#pragma once

#include "../file_reader/src/file_read.h"
#include <string>
#include <vector>
#include <format>
#include <functional>
#include "utils.h"
#include "filter.h"
#include "ascii.h"
#include "convert.h"
#include <zlib.h>

namespace lily_png
{
	std::expected<bool, png_error> apply_to_pixel(file_reader::buffer<unsigned char> &src, metadata &meta, std::function<void(unsigned char *, int, size_t)> func);
	std::expected<bool, png_error> read_png(const std::string &file_path, image &data);
}