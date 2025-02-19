#ifndef MBGL_STYLE_STYLE_PARSER
#define MBGL_STYLE_STYLE_PARSER

#include <mbgl/map/source.hpp>
#include <mbgl/util/ptr.hpp>

#include <rapidjson/document.h>

#include <vector>
#include <memory>
#include <string>
#include <unordered_map>
#include <forward_list>

namespace mbgl {

class StyleLayer;
class Source;

using JSVal = rapidjson::Value;

class StyleParser {
public:
    void parse(const JSVal&);

    std::vector<std::unique_ptr<Source>>&& getSources() {
        return std::move(sources);
    }

    std::vector<util::ptr<StyleLayer>> getLayers() {
        return layers;
    }

    std::string getSpriteURL() const {
        return spriteURL;
    }

    std::string getGlyphURL() const {
        return glyphURL;
    }

private:
    void parseSources(const JSVal&);
    void parseLayers(const JSVal&);
    void parseLayer(const std::string& id, const JSVal&, util::ptr<StyleLayer>&);
    void parseVisibility(StyleLayer&, const JSVal& value);

    std::uint8_t version;

    std::vector<std::unique_ptr<Source>> sources;
    std::vector<util::ptr<StyleLayer>> layers;

    std::unordered_map<std::string, const Source*> sourcesMap;
    std::unordered_map<std::string, std::pair<const JSVal&, util::ptr<StyleLayer>>> layersMap;

    // Store a stack of layer IDs we're parsing right now. This is to prevent reference cycles.
    std::forward_list<std::string> stack;

    std::string spriteURL;
    std::string glyphURL;
};

}

#endif
