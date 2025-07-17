#pragma once
#include "utils.h"

void filter_scanline(unsigned char *scanline, unsigned char *previous_scanline, unsigned char *dest, metadata &meta, unsigned char filter_type);
void filter(buffer_unsigned &data, buffer_unsigned &dest ,metadata &meta);

