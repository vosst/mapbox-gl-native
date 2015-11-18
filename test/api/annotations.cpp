#include "../fixtures/util.hpp"

#include <mbgl/annotation/point_annotation.hpp>
#include <mbgl/annotation/shape_annotation.hpp>
#include <mbgl/sprite/sprite_image.hpp>
#include <mbgl/map/map.hpp>
#include <mbgl/map/still_image.hpp>
#include <mbgl/platform/default/headless_display.hpp>
#include <mbgl/platform/default/headless_view.hpp>
#include <mbgl/storage/default_file_source.hpp>
#include <mbgl/util/image.hpp>
#include <mbgl/util/io.hpp>
#include <mbgl/util/run_loop.hpp>

#include <future>
#include <vector>

using namespace mbgl;

std::string renderPNG(Map& map) {
    std::unique_ptr<const StillImage> image;
    map.renderStill([&](std::exception_ptr, std::unique_ptr<const StillImage> image_) {
        image = std::move(image_);
    });

    while (!image) {
        mbgl::util::RunLoop::Get()->runOnce();
    }

    return util::compress_png(image->width, image->height, image->pixels.get());
}

TEST(Annotations, PointAnnotation) {
    util::RunLoop loop;

    auto display = std::make_shared<mbgl::HeadlessDisplay>();
    HeadlessView view(display, 1);
    DefaultFileSource fileSource(nullptr);

    Map map(view, fileSource, MapMode::Still);
    map.setStyleJSON(util::read_file("test/fixtures/api/empty.json"), "");
    map.addPointAnnotation(PointAnnotation({ 0, 0 }, "default_marker"));

    util::write_file("test/output/point_annotation.png", renderPNG(map));
}

TEST(Annotations, LineAnnotation) {
    util::RunLoop loop;

    auto display = std::make_shared<mbgl::HeadlessDisplay>();
    HeadlessView view(display, 1);
    DefaultFileSource fileSource(nullptr);

    Map map(view, fileSource, MapMode::Still);
    map.setStyleJSON(util::read_file("test/fixtures/api/empty.json"), "");

    AnnotationSegments segments = {{ {{ { 0, 0 }, { 45, 45 } }} }};

    LineAnnotationProperties properties;
    properties.color = {{ 255, 0, 0, 1 }};
    properties.width = 5;

    map.addShapeAnnotation(ShapeAnnotation(segments, properties));

    util::write_file("test/output/line_annotation.png", renderPNG(map));
}

TEST(Annotations, FillAnnotation) {
    util::RunLoop loop;

    auto display = std::make_shared<mbgl::HeadlessDisplay>();
    HeadlessView view(display, 1);
    DefaultFileSource fileSource(nullptr);

    Map map(view, fileSource, MapMode::Still);
    map.setStyleJSON(util::read_file("test/fixtures/api/empty.json"), "");

    AnnotationSegments segments = {{ {{ { 0, 0 }, { 0, 45 }, { 45, 45 }, { 45, 0 } }} }};

    FillAnnotationProperties properties;
    properties.color = {{ 255, 0, 0, 1 }};

    map.addShapeAnnotation(ShapeAnnotation(segments, properties));

    util::write_file("test/output/fill_annotation.png", renderPNG(map));
}

TEST(Annotations, StyleSourcedShapeAnnotation) {
    util::RunLoop loop;

    auto display = std::make_shared<mbgl::HeadlessDisplay>();
    HeadlessView view(display, 1);
    DefaultFileSource fileSource(nullptr);

    Map map(view, fileSource, MapMode::Still);
    map.setStyleJSON(util::read_file("test/fixtures/api/annotation.json"), "");

    AnnotationSegments segments = {{ {{ { 0, 0 }, { 0, 45 }, { 45, 45 }, { 45, 0 } }} }};

    map.addShapeAnnotation(ShapeAnnotation(segments, "annotation"));

    util::write_file("test/output/style_sourced_shape_annotation.png", renderPNG(map));
}

TEST(Annotations, AddMultiple) {
    util::RunLoop loop;

    auto display = std::make_shared<mbgl::HeadlessDisplay>();
    HeadlessView view(display, 1);
    DefaultFileSource fileSource(nullptr);

    Map map(view, fileSource, MapMode::Still);
    map.setStyleJSON(util::read_file("test/fixtures/api/empty.json"), "");
    map.addPointAnnotation(PointAnnotation({ 0, -20 }, "default_marker"));

    renderPNG(map);

    map.addPointAnnotation(PointAnnotation({ 0, 20 }, "default_marker"));

    util::write_file("test/output/add_multiple.png", renderPNG(map));
}

TEST(Annotations, NonImmediateAdd) {
    util::RunLoop loop;

    auto display = std::make_shared<mbgl::HeadlessDisplay>();
    HeadlessView view(display, 1);
    DefaultFileSource fileSource(nullptr);

    Map map(view, fileSource, MapMode::Still);
    map.setStyleJSON(util::read_file("test/fixtures/api/empty.json"), "");

    renderPNG(map);

    AnnotationSegments segments = {{ {{ { 0, 0 }, { 0, 45 }, { 45, 45 }, { 45, 0 } }} }};

    FillAnnotationProperties properties;
    properties.color = {{ 255, 0, 0, 1 }};

    map.addShapeAnnotation(ShapeAnnotation(segments, properties));

    util::write_file("test/output/non_immediate_add.png", renderPNG(map));
}

TEST(Annotations, RemovePoint) {
    util::RunLoop loop;

    auto display = std::make_shared<mbgl::HeadlessDisplay>();
    HeadlessView view(display, 1);
    DefaultFileSource fileSource(nullptr);

    Map map(view, fileSource, MapMode::Still);
    map.setStyleJSON(util::read_file("test/fixtures/api/empty.json"), "");
    uint32_t point = map.addPointAnnotation(PointAnnotation({ 0, 0 }, "default_marker"));

    renderPNG(map);

    map.removeAnnotation(point);

    util::write_file("test/output/remove_point.png", renderPNG(map));
}

TEST(Annotations, RemoveShape) {
    util::RunLoop loop;

    auto display = std::make_shared<mbgl::HeadlessDisplay>();
    HeadlessView view(display, 1);
    DefaultFileSource fileSource(nullptr);

    AnnotationSegments segments = {{ {{ { 0, 0 }, { 45, 45 } }} }};

    LineAnnotationProperties properties;
    properties.color = {{ 255, 0, 0, 1 }};
    properties.width = 5;

    Map map(view, fileSource, MapMode::Still);
    map.setStyleJSON(util::read_file("test/fixtures/api/empty.json"), "");
    uint32_t shape = map.addShapeAnnotation(ShapeAnnotation(segments, properties));

    renderPNG(map);

    map.removeAnnotation(shape);

    util::write_file("test/output/remove_shape.png", renderPNG(map));
}

TEST(Annotations, ImmediateRemoveShape) {
    util::RunLoop loop;

    auto display = std::make_shared<mbgl::HeadlessDisplay>();
    HeadlessView view(display, 1);
    DefaultFileSource fileSource(nullptr);
    Map map(view, fileSource, MapMode::Still);

    map.removeAnnotation(map.addShapeAnnotation(ShapeAnnotation({}, {})));
    map.setStyleJSON(util::read_file("test/fixtures/api/empty.json"), "");

    renderPNG(map);
}

TEST(Annotations, SwitchStyle) {
    util::RunLoop loop;

    auto display = std::make_shared<mbgl::HeadlessDisplay>();
    HeadlessView view(display, 1);
    DefaultFileSource fileSource(nullptr);

    Map map(view, fileSource, MapMode::Still);
    map.setStyleJSON(util::read_file("test/fixtures/api/empty.json"), "");
    map.addPointAnnotation(PointAnnotation({ 0, 0 }, "default_marker"));

    renderPNG(map);

    map.setStyleJSON(util::read_file("test/fixtures/api/empty.json"), "");

    util::write_file("test/output/switch_style.png", renderPNG(map));
}

TEST(Annotations, CustomIcon) {
    util::RunLoop loop;

    auto display = std::make_shared<mbgl::HeadlessDisplay>();
    HeadlessView view(display, 1);
    DefaultFileSource fileSource(nullptr);

    Map map(view, fileSource, MapMode::Still);
    map.setStyleJSON(util::read_file("test/fixtures/api/empty.json"), "");
    map.setSprite("cafe", std::make_shared<SpriteImage>(12, 12, 1, std::string(12 * 12 * 4, '\xFF')));
    map.addPointAnnotation(PointAnnotation({ 0, 0 }, "cafe"));

    util::write_file("test/output/custom_icon.png", renderPNG(map));
}
