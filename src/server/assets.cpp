#include <server/assets.hpp>
#include <util/io.hpp>

namespace {
    // WEBSITE_INDEX is baked in by CMake and points to the installed copy of
    // website/index.html next to the server binary. Sibling files live in the
    // same directory.
    std::string website_sibling(const std::string& filename) {
        std::string index = WEBSITE_INDEX;
        auto slash = index.find_last_of('/');
        if (slash == std::string::npos) return filename;
        return index.substr(0, slash + 1) + filename;
    }
}

Assets Assets::load() {
    return {
        IO::read(WEBSITE_INDEX),
        IO::read(website_sibling("style.css")),
        IO::read(website_sibling("app.js")),
    };
}
