#include <mbgl/map/map.hpp>
#include <mbgl/map/still_image.hpp>
#include <mbgl/util/image.hpp>
#include <mbgl/util/io.hpp>
#include <mbgl/util/run_loop.hpp>

#include <mbgl/platform/default/headless_view.hpp>
#include <mbgl/platform/default/headless_display.hpp>
#include <mbgl/storage/default_file_source.hpp>
#include <mbgl/storage/sqlite_cache.hpp>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"
#pragma GCC diagnostic ignored "-Wunused-local-typedefs"
#pragma GCC diagnostic ignored "-Wshadow"
#include <boost/program_options.hpp>
#pragma GCC diagnostic pop

namespace po = boost::program_options;

#include <cstdlib>
#include <iostream>

int main(int argc, char *argv[]) {
    std::string style_path;
    double lat = 0, lon = 0;
    double zoom = 0;
    double bearing = 0;

    int width = 512;
    int height = 512;
    double pixelRatio = 1.0;
    static std::string output = "out.png";
    std::string cache_file = "cache.sqlite";
    std::vector<std::string> classes;
    std::string token;
    bool debug = false;

    po::options_description desc("Allowed options");
    desc.add_options()
        ("style,s", po::value(&style_path)->required()->value_name("json"), "Map stylesheet")
        ("lon,x", po::value(&lon)->value_name("degrees")->default_value(lon), "Longitude")
        ("lat,y", po::value(&lat)->value_name("degrees")->default_value(lat), "Latitude in degrees")
        ("zoom,z", po::value(&zoom)->value_name("number")->default_value(zoom), "Zoom level")
        ("bearing,b", po::value(&bearing)->value_name("degrees")->default_value(bearing), "Bearing")
        ("width,w", po::value(&width)->value_name("pixels")->default_value(width), "Image width")
        ("height,h", po::value(&height)->value_name("pixels")->default_value(height), "Image height")
        ("class,c", po::value(&classes)->value_name("name"), "Class name")
        ("token,t", po::value(&token)->value_name("key")->default_value(token), "Mapbox access token")
        ("debug", po::bool_switch(&debug)->default_value(debug), "Debug mode")
        ("output,o", po::value(&output)->value_name("file")->default_value(output), "Output file name")
        ("cache,d", po::value(&cache_file)->value_name("file")->default_value(cache_file), "Cache database file name")
    ;

    try {
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
    } catch(std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl << desc;
        exit(1);
    }

    std::string style = mbgl::util::read_file(style_path);

    using namespace mbgl;

    util::RunLoop loop;
    SQLiteCache cache(cache_file);
    DefaultFileSource fileSource(&cache);

    // Try to load the token from the environment.
    if (!token.size()) {
        const char *token_ptr = getenv("MAPBOX_ACCESS_TOKEN");
        if (token_ptr) {
            token = token_ptr;
        }
    }

    // Set access token if present
    if (token.size()) {
        fileSource.setAccessToken(std::string(token));
    }

    HeadlessView view(pixelRatio, width, height);
    Map map(view, fileSource, MapMode::Still);

    map.setStyleJSON(style, ".");
    map.setClasses(classes);

    map.setLatLngZoom({ lat, lon }, zoom);
    map.setBearing(bearing);

    if (debug) {
        map.setDebug(debug);
    }

    std::unique_ptr<const StillImage> image;
    map.renderStill([&](std::exception_ptr error, std::unique_ptr<const StillImage> image_) {
        try {
            if (error) {
                std::rethrow_exception(error);
            }
        } catch(std::exception& e) {
            std::cout << "Error: " << e.what() << std::endl;
            exit(1);
        }

        image = std::move(image_);
        loop.stop();
    });

    loop.run();

    const std::string png = util::compress_png(image->width, image->height, image->pixels.get());
    util::write_file(output, png);

    return 0;
}
