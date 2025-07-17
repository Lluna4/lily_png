#pragma once
#include "metadata.h"

size_t get_pixel_bit_size(const metadata &meta);
size_t get_uncompressed_size(const metadata meta);
int paeth_predict(int a, int b, int c);
