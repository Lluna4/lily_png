#pragma once
#include "utils.h"

void filter_scanline(unsigned char *scanline, unsigned char *previous_scanline, unsigned char *dest, metadata &meta, unsigned char filter_type);
void filter(buffer<unsigned char> &data, buffer<unsigned char> &dest ,metadata &meta);

