#pragma once

#include "file_reader/src/file_read.h"
#include <string>

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

buffer read_png(const std::string &file_path);