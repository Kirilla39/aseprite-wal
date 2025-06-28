#include <aseprite_wal/image.hpp>
#include <png.h>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <vector>
#include <string>
#include <cstdint>

Color Color::from_hex(const std::string& hex) {
    if (hex.length() != 6 && (hex.length() != 7 || hex[0] != '#')) {
        throw std::runtime_error("Invalid hex color format. Use RRGGBB or #RRGGBB");
    }
    const char* str = hex.c_str() + (hex[0] == '#' ? 1 : 0);
    char* end;
    long value = strtol(str, &end, 16);
    return Color{
        static_cast<uint8_t>((value >> 16) & 0xFF),
        static_cast<uint8_t>((value >> 8) & 0xFF),
        static_cast<uint8_t>(value & 0xFF)
    };
}

std::vector<uint8_t> PNGHandler::read_image(const char* filename, int& width, int& height) {
    FILE* fp = fopen(filename, "rb");
    if (!fp) throw std::runtime_error("Cannot open input file");
    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png) throw std::runtime_error("Cannot create PNG read struct");
    png_infop info = png_create_info_struct(png);
    if (!info) {
        png_destroy_read_struct(&png, nullptr, nullptr);
        throw std::runtime_error("Cannot create PNG info struct");
    }
    if (setjmp(png_jmpbuf(png))) {
        png_destroy_read_struct(&png, &info, nullptr);
        fclose(fp);
        throw std::runtime_error("Error during PNG read");
    }
    png_init_io(png, fp);
    png_read_info(png, info);
    width = png_get_image_width(png, info);
    height = png_get_image_height(png, info);
    png_byte color_type = png_get_color_type(png, info);
    png_byte bit_depth = png_get_bit_depth(png, info);
    if (bit_depth == 16) png_set_strip_16(png);
    if (color_type == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png);
    if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_PALETTE) png_set_add_alpha(png, 0xFF, PNG_FILLER_AFTER);
    if (color_type == PNG_COLOR_TYPE_GRAY || color_type == PNG_COLOR_TYPE_GRAY_ALPHA) png_set_gray_to_rgb(png);
    png_read_update_info(png, info);
    std::vector<uint8_t> image_data(width * height * 4);
    std::vector<png_bytep> row_pointers(height);
    for (int y = 0; y < height; y++) {
        row_pointers[y] = &image_data[y * width * 4];
    }
    png_read_image(png, row_pointers.data());
    png_destroy_read_struct(&png, &info, nullptr);
    fclose(fp);
    return image_data;
}

void PNGHandler::write_image(const char* filename, const std::vector<uint8_t>& image_data, int width, int height) {
    FILE* fp = fopen(filename, "wb");
    if (!fp) throw std::runtime_error("Cannot open output file");
    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (!png) throw std::runtime_error("Cannot create PNG write struct");
    png_infop info = png_create_info_struct(png);
    if (!info) {
        png_destroy_write_struct(&png, nullptr);
        throw std::runtime_error("Cannot create PNG info struct");
    }
    if (setjmp(png_jmpbuf(png))) {
        png_destroy_write_struct(&png, &info);
        fclose(fp);
        throw std::runtime_error("Error during PNG write");
    }
    png_init_io(png, fp);
    png_set_IHDR(png, info, width, height, 8,
                 PNG_COLOR_TYPE_RGBA,
                 PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);
    std::vector<png_bytep> row_pointers(height);
    for (int y = 0; y < height; y++) {
        row_pointers[y] = const_cast<png_bytep>(&image_data[y * width * 4]);
    }
    png_write_info(png, info);
    png_write_image(png, row_pointers.data());
    png_write_end(png, nullptr);
    png_destroy_write_struct(&png, &info);
    fclose(fp);
}

void apply_color(std::vector<uint8_t>& image_data, const Color& color, int width) {
    for (size_t i = 0; i < image_data.size(); i += 4) {
        float gray = image_data[i] / 255.0f;
        uint8_t r = static_cast<uint8_t>(color.r * gray);
        uint8_t g = static_cast<uint8_t>(color.g * gray);
        uint8_t b = static_cast<uint8_t>(color.b * gray);
        image_data[i] = r;
        image_data[i + 1] = g;
        image_data[i + 2] = b;
    }
}

std::vector<uint8_t> pad_image(const std::vector<uint8_t>& img, int w, int h, int new_w, int new_h) {
    std::vector<uint8_t> out(new_w * new_h * 4, 0);
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            for (int c = 0; c < 4; ++c) {
                out[(y * new_w + x) * 4 + c] = img[(y * w + x) * 4 + c];
            }
        }
    }
    return out;
}

void alpha_blend(const std::vector<uint8_t>& bottom, const std::vector<uint8_t>& top, std::vector<uint8_t>& out, int width, int height) {
    out.resize(width * height * 4);
    for (size_t i = 0; i < out.size(); i += 4) {
        float a1 = top[i + 3] / 255.0f;
        float a2 = bottom[i + 3] / 255.0f;
        float a = a1 + a2 * (1 - a1);
        if (a > 0) {
            for (int c = 0; c < 3; ++c) {
                float c1 = top[i + c] / 255.0f;
                float c2 = bottom[i + c] / 255.0f;
                float blend = (c1 * a1 + c2 * a2 * (1 - a1)) / a;
                out[i + c] = static_cast<uint8_t>(blend * 255);
            }
        }
        out[i + 3] = static_cast<uint8_t>(a * 255);
    }
} 