#pragma once

#include "../file_reader/src/file_read.h"
#include <string>
#include <vector>
#include "utils.h"
#include "filter.h"
#include <zlib.h>

namespace lily_png
{
	struct color_rgb
	{
		unsigned char r = 0;
		unsigned char g = 0;
		unsigned char b = 0;
	};

	enum class png_error
	{
		file_doesnt_exist,
		read_failed,
		file_is_not_a_png
	};


	std::expected<metadata, png_error> read_png(const std::string &file_path, file_reader::buffer<unsigned char> &data);
}