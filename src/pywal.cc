#include <aseprite_wal/pywal.hpp>
#include <nlohmann/json.hpp>
#include <fstream>
#include <stdexcept>

std::map<std::string, std::string> load_pywal_colors(const std::string& json_path) {
    std::ifstream f(json_path);
    if (!f) throw std::runtime_error("Cannot open pywal colors file: " + json_path);
    nlohmann::json j;
    f >> j;
    std::map<std::string, std::string> color_map;
    if (!j.contains("colors")) throw std::runtime_error("No 'colors' key in pywal file");
    const auto& jcolors = j["colors"];
    for (int i = 0; i <= 15; ++i) {
        std::string key = "color" + std::to_string(i);
        if (jcolors.contains(key)) color_map[key] = jcolors[key];
    }
    if (j.contains("special")) {
        const auto& jspecial = j["special"];
        for (const std::string& key : {"foreground", "background", "cursor"}) {
            if (jspecial.contains(key)) color_map[key] = jspecial[key];
        }
    }
    return color_map;
} 