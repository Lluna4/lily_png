#pragma once
#include "metadata.h"

namespace lily_png
{
    size_t get_pixel_bit_size(const metadata &meta);
    size_t get_uncompressed_size(const metadata meta);
    int paeth_predict(const int a, const int b, const int c);
}
