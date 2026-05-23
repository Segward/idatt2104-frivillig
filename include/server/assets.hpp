#ifndef ASSETS_HPP
#define ASSETS_HPP

// Snapshot of the static website files. Loaded once at server startup so the
// IO loop never touches disk during a request. Does not include uWS headers
// on purpose, so it can be reused outside the server target.
struct Assets {
    std::string index_html;
    std::string style_css;
    std::string app_js;
    static Assets load();
};

#endif
