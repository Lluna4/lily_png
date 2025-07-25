#pragma once
#include "utils.h"
#include <cmath>

namespace lily_png
{
    std::expected<bool, png_error> lily_png::filter_scanline(unsigned char *scanline, unsigned char *previous_scanline, unsigned char *dest, metadata &meta, unsigned char filter_type);
    std::expected<bool, png_error> filter(file_reader::buffer<unsigned char> &data, file_reader::buffer<unsigned char> &dest ,metadata &meta);
}

