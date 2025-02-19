#include <mbgl/style/style.hpp>
#include <mbgl/map/map_data.hpp>
#include <mbgl/map/source.hpp>
#include <mbgl/map/transform_state.hpp>
#include <mbgl/sprite/sprite_store.hpp>
#include <mbgl/sprite/sprite_atlas.hpp>
#include <mbgl/style/style_layer.hpp>
#include <mbgl/style/style_parser.hpp>
#include <mbgl/style/property_transition.hpp>
#include <mbgl/style/class_dictionary.hpp>
#include <mbgl/style/style_cascade_parameters.hpp>
#include <mbgl/style/style_calculation_parameters.hpp>
#include <mbgl/geometry/glyph_atlas.hpp>
#include <mbgl/geometry/line_atlas.hpp>
#include <mbgl/util/constants.hpp>
#include <mbgl/platform/log.hpp>
#include <csscolorparser/csscolorparser.hpp>

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

#include <algorithm>

namespace mbgl {

Style::Style(MapData& data_)
    : data(data_),
      glyphStore(std::make_unique<GlyphStore>()),
      glyphAtlas(std::make_unique<GlyphAtlas>(1024, 1024)),
      spriteStore(std::make_unique<SpriteStore>(data.pixelRatio)),
      spriteAtlas(std::make_unique<SpriteAtlas>(512, 512, data.pixelRatio, *spriteStore)),
      lineAtlas(std::make_unique<LineAtlas>(512, 512)),
      mtx(std::make_unique<uv::rwlock>()),
      workers(4) {
    glyphStore->setObserver(this);
    spriteStore->setObserver(this);
}

void Style::setJSON(const std::string& json, const std::string&) {
    rapidjson::Document doc;
    doc.Parse<0>((const char *const)json.c_str());
    if (doc.HasParseError()) {
        Log::Error(Event::ParseStyle, "Error parsing style JSON at %i: %s", doc.GetErrorOffset(), rapidjson::GetParseError_En(doc.GetParseError()));
        return;
    }

    StyleParser parser;
    parser.parse(doc);

    for (auto& source : parser.getSources()) {
        addSource(std::move(source));
    }

    for (auto& layer : parser.getLayers()) {
        addLayer(std::move(layer));
    }

    glyphStore->setURL(parser.getGlyphURL());
    spriteStore->setURL(parser.getSpriteURL());

    loaded = true;
}

Style::~Style() {
    for (const auto& source : sources) {
        source->setObserver(nullptr);
    }

    glyphStore->setObserver(nullptr);
    spriteStore->setObserver(nullptr);
}

void Style::addSource(std::unique_ptr<Source> source) {
    source->setObserver(this);
    source->load();
    sources.emplace_back(std::move(source));
}

std::vector<util::ptr<StyleLayer>>::const_iterator Style::findLayer(const std::string& id) const {
    return std::find_if(layers.begin(), layers.end(), [&](const auto& layer) {
        return layer->id == id;
    });
}

StyleLayer* Style::getLayer(const std::string& id) const {
    auto it = findLayer(id);
    return it != layers.end() ? it->get() : nullptr;
}

void Style::addLayer(util::ptr<StyleLayer> layer) {
    layers.emplace_back(std::move(layer));
}

void Style::addLayer(util::ptr<StyleLayer> layer, const std::string& before) {
    layers.emplace(findLayer(before), std::move(layer));
}

void Style::removeLayer(const std::string& id) {
    auto it = findLayer(id);
    if (it == layers.end())
        throw std::runtime_error("no such layer");
    layers.erase(it);
}

void Style::update(const TransformState& transform,
                   TexturePool& texturePool) {
    bool allTilesUpdated = true;
    for (const auto& source : sources) {
        if (!source->update(data, transform, *this, texturePool, shouldReparsePartialTiles)) {
            allTilesUpdated = false;
        }
    }

    // We can only stop updating "partial" tiles when all of them
    // were notified of the arrival of the new resources.
    if (allTilesUpdated) {
        shouldReparsePartialTiles = false;
    }
}

void Style::cascade() {
    std::vector<ClassID> classes;

    std::vector<std::string> classNames = data.getClasses();
    for (auto it = classNames.rbegin(); it != classNames.rend(); it++) {
        classes.push_back(ClassDictionary::Get().lookup(*it));
    }
    classes.push_back(ClassID::Default);
    classes.push_back(ClassID::Fallback);

    StyleCascadeParameters parameters(classes,
                                      data.getAnimationTime(),
                                      PropertyTransition { data.getDefaultTransitionDuration(),
                                                           data.getDefaultTransitionDelay() });

    for (const auto& layer : layers) {
        layer->cascade(parameters);
    }
}

void Style::recalculate(float z) {
    uv::writelock lock(mtx);

    for (const auto& source : sources) {
        source->enabled = false;
    }

    zoomHistory.update(z, data.getAnimationTime());

    StyleCalculationParameters parameters(z,
                                          data.getAnimationTime(),
                                          zoomHistory,
                                          data.getDefaultFadeDuration());

    for (const auto& layer : layers) {
        hasPendingTransitions |= layer->recalculate(parameters);

        Source* source = getSource(layer->source);
        if (!source) {
            continue;
        }

        source->enabled = true;
    }
}

Source* Style::getSource(const std::string& id) const {
    const auto it = std::find_if(sources.begin(), sources.end(), [&](const auto& source) {
        return source->info.source_id == id;
    });

    return it != sources.end() ? it->get() : nullptr;
}

bool Style::hasTransitions() const {
    return hasPendingTransitions;
}

bool Style::isLoaded() const {
    if (!loaded) {
        return false;
    }

    for (const auto& source : sources) {
        if (!source->isLoaded()) {
            return false;
        }
    }

    if (!spriteStore->isLoaded()) {
        return false;
    }

    return true;
}

void Style::setObserver(Observer* observer_) {
    assert(util::ThreadContext::currentlyOn(util::ThreadType::Map));
    assert(!observer);

    observer = observer_;
}

void Style::onGlyphRangeLoaded() {
    shouldReparsePartialTiles = true;

    emitTileDataChanged();
}

void Style::onGlyphRangeLoadingFailed(std::exception_ptr error) {
    emitResourceLoadingFailed(error);
}

void Style::onSourceLoaded() {
    emitTileDataChanged();
}

void Style::onSourceLoadingFailed(std::exception_ptr error) {
    emitResourceLoadingFailed(error);
}

void Style::onTileLoaded(bool isNewTile) {
    if (isNewTile) {
        shouldReparsePartialTiles = true;
    }

    emitTileDataChanged();
}

void Style::onTileLoadingFailed(std::exception_ptr error) {
    emitResourceLoadingFailed(error);
}

void Style::onSpriteLoaded() {
    shouldReparsePartialTiles = true;
    emitTileDataChanged();
}

void Style::onSpriteLoadingFailed(std::exception_ptr error) {
    emitResourceLoadingFailed(error);
}

void Style::emitTileDataChanged() {
    assert(util::ThreadContext::currentlyOn(util::ThreadType::Map));

    if (observer) {
        observer->onTileDataChanged();
    }
}

void Style::emitResourceLoadingFailed(std::exception_ptr error) {
    assert(util::ThreadContext::currentlyOn(util::ThreadType::Map));

    try {
        if (error) {
            lastError = error;
            std::rethrow_exception(error);
        }
    } catch(const std::exception& e) {
        Log::Error(Event::Style, "%s", e.what());
    }

    if (observer) {
        observer->onResourceLoadingFailed(error);
    }
}

void Style::dumpDebugLogs() const {
    for (const auto& source : sources) {
        source->dumpDebugLogs();
    }

    spriteStore->dumpDebugLogs();
}

}
