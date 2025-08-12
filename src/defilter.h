#pragma once

#include "utils.h"
#include <expected>
#include <vector>

namespace lily_png
{
	std::expected<bool, png_error> defilter_pixel(unsigned char *dest, unsigned char x, unsigned char a, unsigned char b, unsigned char c, unsigned char type);
	std::expected<bool, png_error> defilter(file_reader::buffer<unsigned char> &src, file_reader::buffer<unsigned char> &dest, metadata &meta);
}
