#pragma once
#include "utils.h"
#include <cmath>

namespace lily_png
{
    void filter_scanline(unsigned char *scanline, unsigned char *previous_scanline, unsigned char *dest, metadata &meta, unsigned char filter_type);
    void filter(file_reader::buffer<unsigned char> &data, file_reader::buffer<unsigned char> &dest ,metadata &meta);
}

