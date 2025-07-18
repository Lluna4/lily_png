#pragma once
#include "../file_reader/src/file_read.h"

namespace lily_png
{
	enum class color
	{
		grayscale = 0,
		rgb = 2,
		indexed = 3,
		grayscale_alpha = 4,
		rgba = 6
	};
	struct metadata
	{
		unsigned int width;
		unsigned int height;
		char bit_depth;
		char color_type;
		char compression;
		char filter;
		char interface;
	};

	metadata parse_metadata(file_reader::buffer<char> &data);
}
