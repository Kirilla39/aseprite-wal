#pragma once
#include <string>
#include <vector>
#include <cstdint>

struct Color {
    uint8_t r, g, b;
    static Color from_hex(const std::string& hex);
};

class PNGHandler {
public:
    static std::vector<uint8_t> read_image(const char* filename, int& width, int& height);
    static void write_image(const char* filename, const std::vector<uint8_t>& image_data, int width, int height);
};

void apply_color(std::vector<uint8_t>& image_data, const Color& color, int width = -1);
std::vector<uint8_t> pad_image(const std::vector<uint8_t>& img, int w, int h, int new_w, int new_h);
void alpha_blend(const std::vector<uint8_t>& bottom, const std::vector<uint8_t>& top, std::vector<uint8_t>& out, int w, int h); 