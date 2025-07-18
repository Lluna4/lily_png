#pragma once
#include "../file_reader/src/buffer.h"
#include "metadata.h"

namespace lily_png
{
    void convert_to_R32G32B32A32(file_reader::buffer<unsigned char> &data, metadata &meta);
}
