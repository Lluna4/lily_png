#pragma once
#include "../file_reader/src/file_read.h"

namespace lily_png
{
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
