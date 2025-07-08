#pragma once

#include "file_reader/src/file_read.h"
#include <string>
#include <zlib.h>

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

void read_png(const std::string &file_path, buffer_unsigned &data);