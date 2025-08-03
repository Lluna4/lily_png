#pragma once
#include "metadata.h"
#include <expected>
#include <cassert>

namespace lily_png
{
    enum class png_error
    {
        file_doesnt_exist,
        read_failed,
        file_is_not_a_png,
        non_standard_filter,
        invalid_bit_depth,
        invalid_color_type
    };

    std::expected<size_t, png_error> get_pixel_bit_size(const metadata &meta);
    std::expected<size_t, png_error> get_uncompressed_size(const metadata &meta);
    int paeth_predict(const int a, const int b, const int c);
    float get_aspect_ratio(const metadata &meta);

    struct image
    {
        image()
        {
            auto pixel_size_ret = get_pixel_bit_size(meta).or_else([] (png_error error)
            {
                return std::expected<size_t, png_error>(0);
            });
            pixel_size = pixel_size_ret.value();
            pixel_size_bytes = (pixel_size + 7)/8;
        }
        file_reader::buffer<unsigned char> buffer{};
        metadata meta{};


        constexpr unsigned char *operator[](std::size_t y, std::size_t x) const
        {
            if (x > meta.width || y > meta.height)
                return nullptr;
            const size_t calculate = (y * meta.width + x) * pixel_size_bytes;
            return &buffer.data[calculate];
        }
        std::expected<size_t, png_error> resize_image(image &dest);

    private:
        size_t pixel_size;
        size_t pixel_size_bytes;
    };
}
