#pragma once
#include "metadata.h"
#include <expected>
#include <cassert>
#include <math.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>

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

        explicit image(metadata m)
            :meta(m)
        {
            auto pixel_size_ret = get_pixel_bit_size(m).or_else([] (png_error error)
            {
                return std::expected<size_t, png_error>(0);
            });
            pixel_size = pixel_size_ret.value();
            pixel_size_bytes = (pixel_size + 7)/8;
        }
        file_reader::buffer<unsigned char> buffer{};
        metadata meta{};

        void add_metadata(metadata m)
        {
            auto pixel_size_ret = get_pixel_bit_size(m).or_else([] (png_error error)
            {
                return std::expected<size_t, png_error>(0);
            });
            pixel_size = pixel_size_ret.value();
            pixel_size_bytes = (pixel_size + 7)/8;
            this->meta = m;
        }

        constexpr unsigned char *operator[](std::size_t y, std::size_t x) const
        {
            if (x > meta.width || y > meta.height)
                return nullptr;
            const size_t calculate = (y * meta.width + x) * pixel_size_bytes;
            return &buffer.data[calculate];
        }
        std::expected<size_t, png_error> resize_image(image &dest);
        std::expected<bool, png_error> to_ascii(file_reader::buffer<char> &dest);
    private:
        size_t pixel_size;
        size_t pixel_size_bytes;
    };
}

template <>
struct std::formatter<lily_png::png_error>
{

    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const lily_png::png_error& id, std::format_context& ctx) const
    {
        if (id == lily_png::png_error::file_doesnt_exist)
            return std::format_to(ctx.out(), "{}", "File doesn't exist");
        if (id == lily_png::png_error::read_failed)
            return std::format_to(ctx.out(), "{}", "Read failed");
        if (id == lily_png::png_error::file_is_not_a_png)
            return std::format_to(ctx.out(), "{}", "File is not a png");
        if (id == lily_png::png_error::invalid_bit_depth)
            return std::format_to(ctx.out(), "{}", "Invalid bit depth");
        if (id == lily_png::png_error::invalid_color_type)
            return std::format_to(ctx.out(), "{}", "Invalid color type");
        if (id == lily_png::png_error::non_standard_filter)
            return std::format_to(ctx.out(), "{}", "Non standard filter");
        return std::format_to(ctx.out(), "{}", "Unknown error");
    }
};
