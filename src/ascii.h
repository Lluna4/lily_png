#pragma once

#include "../file_reader/src/buffer.h"
#include "utils.h"
#include "metadata.h"
#include <format>

namespace lily_png
{
	void convert_to_ascii(image &src, image &dest);
}
