#include <png.h>
#include <cstdlib>
#include <cstdint>
#include <string>
#include <stdexcept>
#include <iostream>
#include <vector>
#include <nlohmann/json.hpp>
#include <filesystem>
#include <map>
#include <aseprite_wal/image.hpp>
#include <aseprite_wal/theme.hpp>
#include <aseprite_wal/pywal.hpp>

using json = nlohmann::json;
namespace fs = std::filesystem;

void help(const char* program_name) {
    std::cerr << "Usage: " << program_name << " [options]\n"
              << "Options:\n"
              << "  --colors PATH      Path to pywal colors.json (default: ~/.cache/wal/colors.json)\n"
              << "  --template PATH    Path to template directory (default: ~/.config/aseprite-wal/templates/light)\n"
              << "  --theme-path PATH  Path to Aseprite extensions directory (default: ~/.local/share/Steam/steamapps/common/Aseprite/data/extensions/)\n"
              << "  --help            Show this help message\n\n"
              << "Example:\n"
              << "  " << program_name << " --colors ~/.cache/wal/colors.json --template ~/.config/aseprite-wal/templates/light\n";
}

bool has_arg(int argc, char* argv[], const std::string& name) {
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == name) return true;
    }
    return false;
}

std::string get_arg(int argc, char* argv[], const std::string& name, const std::string& def) {
    for (int i = 1; i < argc - 1; ++i) {
        if (std::string(argv[i]) == name) return argv[i + 1];
    }
    return def;
}

std::string expand_tilde(const std::string& path) {
    if (!path.empty() && path[0] == '~') {
        const char* home = getenv("HOME");
        if (home) return std::string(home) + path.substr(1);
    }
    return path;
}

const std::vector<std::string> pywal_keys = {
    "color0", "color1", "color2", "color3", "color4", "color5", "color6", "color7",
    "color8", "color9", "color10", "color11", "color12", "color13", "color14", "color15",
    "foreground", "background", "cursor"
};

int main(int argc, char* argv[]) {
    try {
        if (has_arg(argc, argv, "--help")) {
            help(argv[0]);
            return 0;
        }
        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];
            if (arg.substr(0, 2) == "--") {
                if (arg != "--colors" && arg != "--template" && arg != "--theme-path") {
                    std::cerr << "Error: Unknown parameter '" << arg << "'\n"
                            << "Use --help to see available options.\n";
                    return 1;
                }
                if (i + 1 >= argc) {
                    std::cerr << "Error: Missing value for parameter '" << arg << "'\n"
                            << "Use --help to see available options.\n";
                    return 1;
                }
                i++;
            }
        }
        std::string colors_path = get_arg(argc, argv, "--colors", "~/.cache/wal/colors.json");
        std::string template_path = get_arg(argc, argv, "--template", "~/.config/aseprite-wal/templates/light");
        std::string theme_path = get_arg(argc, argv, "--theme-path", "~/.local/share/Steam/steamapps/common/Aseprite/data/extensions/");
        colors_path = expand_tilde(colors_path);
        template_path = expand_tilde(template_path);
        theme_path = expand_tilde(theme_path);
        if (!fs::exists(colors_path)) {
            std::cerr << "Error: Colors file not found: " << colors_path << "\n"
                     << "Use --help to see available options.\n";
            return 1;
        }
        if (!fs::exists(template_path)) {
            std::cerr << "Error: Template directory not found: " << template_path << "\n"
                     << "Use --help to see available options.\n";
            return 1;
        }
        if (!fs::exists(theme_path)) {
            std::cerr << "Error: Theme path not found: " << theme_path << "\n"
                     << "Use --help to see available options.\n";
            return 1;
        }
        std::string out_dir = (fs::path(theme_path) / "aseprite-wal").string();
        std::string out_path = (fs::path(out_dir) / "sheet.png").string();
        fs::create_directories(out_dir);
        std::map<std::string, std::string> color_map = load_pywal_colors(colors_path);
        int width = -1, height = -1;
        std::map<std::string, std::vector<uint8_t>> images;
        for (const auto& key : pywal_keys) {
            std::string img_path = (fs::path(template_path) / (key + ".png")).string();
            if (!fs::exists(img_path)) continue;
            auto it = color_map.find(key);
            if (it == color_map.end()) continue;
            Color color = Color::from_hex(it->second);
            int w, h;
            std::vector<uint8_t> img = PNGHandler::read_image(img_path.c_str(), w, h);
            apply_color(img, color, w);
            if (width == -1 && height == -1) { width = w; height = h; }
            if (w != width || h != height) img = pad_image(img, w, h, width, height);
            images[key] = std::move(img);
        }
        std::string nocolor_path = (fs::path(template_path) / "nocolor.png").string();
        if (fs::exists(nocolor_path)) {
            int w, h;
            std::vector<uint8_t> img = PNGHandler::read_image(nocolor_path.c_str(), w, h);
            if (width == -1 && height == -1) { width = w; height = h; }
            if (w != width || h != height) img = pad_image(img, w, h, width, height);
            images["nocolor"] = std::move(img);
        }
        if (width == -1 || height == -1) throw std::runtime_error("No valid images found in template folder");
        std::vector<std::string> blend_order = pywal_keys;
        if (images.count("nocolor")) {
            blend_order.push_back("nocolor");
        }
        std::vector<uint8_t> out_img(width * height * 4, 0);
        bool first = true;
        for (const auto& key : blend_order) {
            auto it = images.find(key);
            if (it == images.end()) continue;
            if (first) {
                out_img = it->second;
                first = false;
            } else {
                std::vector<uint8_t> blended;
                alpha_blend(out_img, it->second, blended, width, height);
                out_img = std::move(blended);
            }
        }
        PNGHandler::write_image(out_path.c_str(), out_img, width, height);
        std::string theme_template = (fs::path(template_path) / "theme.xml").string();
        std::string theme_xml_out = (fs::path(out_dir) / "theme.xml").string();
        process_theme_xml(theme_template, color_map, theme_xml_out);
        for (const char* fname : {"sheet.aseprite-data", "package.json"}) {
            fs::path src = fs::path(template_path) / fname;
            fs::path dst = fs::path(out_dir) / fname;
            if (fs::exists(src)) {
                fs::copy_file(src, dst, fs::copy_options::overwrite_existing);
            }
        }
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n"
                 << "Use --help to see available options.\n";
        return 1;
    }
}
