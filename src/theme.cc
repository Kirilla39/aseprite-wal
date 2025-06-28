#include <aseprite_wal/theme.hpp>
#include <fstream>
#include <regex>
#include <sstream>
#include <iomanip>

static std::string scale_hex_color(const std::string& hex, int scale) {
    int r = 0, g = 0, b = 0;
    if (hex.size() == 7 && hex[0] == '#') {
        r = std::stoi(hex.substr(1,2), nullptr, 16);
        g = std::stoi(hex.substr(3,2), nullptr, 16);
        b = std::stoi(hex.substr(5,2), nullptr, 16);
    } else {
        return hex;
    }
    int rr = (r * scale) / 256;
    int gg = (g * scale) / 256;
    int bb = (b * scale) / 256;
    std::ostringstream oss;
    oss << '#' << std::hex << std::setw(2) << std::setfill('0') << rr
        << std::setw(2) << std::setfill('0') << gg
        << std::setw(2) << std::setfill('0') << bb;
    return oss.str();
}

void process_theme_xml(const std::string& template_path, const std::map<std::string, std::string>& color_map, const std::string& output_path) {
    std::ifstream in(template_path);
    if (!in) throw std::runtime_error("Cannot open theme template: " + template_path);
    std::ofstream out(output_path);
    if (!out) throw std::runtime_error("Cannot write to output: " + output_path);

    std::string line;
    bool in_colors = false;
    std::regex color_re(R"(value=\"([0-9A-Fa-f]{1,2})?@([a-zA-Z0-9_]+)\")");
    while (std::getline(in, line)) {
        if (line.find("<colors>") != std::string::npos) in_colors = true;
        if (line.find("</colors>") != std::string::npos) in_colors = false;
        if (in_colors) {
            std::smatch m;
            if (std::regex_search(line, m, color_re)) {
                int scale = 255;
                if (m[1].matched) {
                    scale = std::stoi(m[1], nullptr, 16);
                }
                std::string key = m[2];
                auto it = color_map.find(key);
                if (it != color_map.end()) {
                    std::string hex = it->second;
                    std::string scaled = scale_hex_color(hex, scale);
                    line = std::regex_replace(line, color_re, "value=\"" + scaled + "\"");
                }
            }
        }
        out << line << '\n';
    }
} 