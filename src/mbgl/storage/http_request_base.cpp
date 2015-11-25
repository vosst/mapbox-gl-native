#include <mbgl/storage/http_request_base.hpp>

#include <mbgl/util/http_header.hpp>
#include <mbgl/util/chrono.hpp>

namespace mbgl {

time_t HTTPRequestBase::parseCacheControl(const char *value) {
    if (value) {
        const auto cacheControl = http::CacheControl::parse(value);
        if (cacheControl.maxAge) {
            return SystemClock::to_time_t(SystemClock::now()) + *cacheControl.maxAge;
        }
    }

    return 0;
}

}
