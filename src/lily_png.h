#pragma once

#include "../file_reader/src/file_read.h"
#include <string>
#include <vector>
#include "utils.h"
#include "filter.h"
#include <zlib.h>

struct color
{
	unsigned char r = 0;
	unsigned char g = 0;
	unsigned char b = 0;
};

void read_png(const std::string &file_path, buffer<unsigned char> &data);