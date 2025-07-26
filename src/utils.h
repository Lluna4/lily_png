#pragma once
#include "metadata.h"
#include <expected>

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
}
