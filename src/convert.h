#pragma once
#include "../file_reader/src/buffer.h"
#include "metadata.h"
#include "utils.h"
#include <expected>
#include <vector>

namespace lily_png
{
    struct color_rgb
    {
        unsigned char r = 0;
        unsigned char g = 0;
        unsigned char b = 0;
    };

    struct color_rgba
    {
        unsigned char r = 0;
        unsigned char g = 0;
        unsigned char b = 0;
        unsigned char a = 0;
    };

    struct color_rgba32
    {
        unsigned int r = 0;
        unsigned int g = 0;
        unsigned int b = 0;
        unsigned int a = 0;
    };

    enum class CONVERT_ERROR
    {
        color_type_mismatch,
        non_standard_bit_depth
    };

    std::expected<bool, CONVERT_ERROR> convert_to_R32G32B32A32(file_reader::buffer<unsigned char> &data, file_reader::buffer<unsigned char> &dest, metadata &meta);
}
