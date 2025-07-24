
#include "utils.h"

size_t lily_png::get_pixel_bit_size(const metadata &meta)
{
	size_t ret = 0;
	std::println("Bit depth {} color {}", (int)meta.bit_depth, (int)meta.color_type);
	switch (meta.color_type)
	{
		case static_cast<int>(color::grayscale):
			if (meta.bit_depth == 1 || meta.bit_depth == 2 || meta.bit_depth == 4 || meta.bit_depth == 8 || meta.bit_depth == 16)
			{
				ret = meta.bit_depth;
			}
			else
				throw std::runtime_error("Invalid bit depht");
			break;
		case static_cast<int>(color::rgb):
			if (meta.bit_depth == 8 || meta.bit_depth == 16)
			{
				ret = meta.bit_depth * 3;
			}
			else
				throw std::runtime_error("Invalid bit depht");
			break;
		case static_cast<int>(color::indexed):
			if (meta.bit_depth == 1 || meta.bit_depth == 2 || meta.bit_depth == 4 || meta.bit_depth == 8)
			{
				ret = meta.bit_depth;
			}
			else
				throw std::runtime_error("Invalid bit depht");
			break;
		case static_cast<int>(color::grayscale_alpha):
			if (meta.bit_depth == 8 || meta.bit_depth == 16)
			{
				ret = meta.bit_depth * 2;
			}
			else
				throw std::runtime_error("Invalid bit depht");
			break;
		case static_cast<int>(color::rgba):
			if (meta.bit_depth == 8 || meta.bit_depth == 16)
			{
				ret = meta.bit_depth * 4;
			}
			else
				throw std::runtime_error("Invalid bit depht");
			break;
		default:
			throw std::runtime_error("Invalid color type");
	}
	return ret;
}

size_t lily_png::get_uncompressed_size(const metadata &meta)
{
	std::println("metadata{{ width: {}, height: {}, bit_depth: {}, color_type: {}, compression: {}, filter: {}, interface: {} }}", meta.width, meta.height, (int)meta.bit_depth, (int)meta.color_type, (int)meta.compression, (int)meta.filter, (int)meta.interface);
	size_t ret = (meta.width * get_pixel_bit_size(meta) + 7)/8; //ceil() but for bytes
	ret = (ret + 1) * meta.height;
	std::println("Uncompressed size {}", ret);
	return ret;
}

int lily_png::paeth_predict(const int a, const int b, const int c)
{
	const int pred = a+b-c;
	const int pred1 = abs(pred - a);
	const int pred2 = abs(pred - b);
	const int pred3 = abs(pred - c);
	if (pred1 <= pred2 && pred1 <= pred3)
		return a;
	if (pred2 <= pred3)
		return b;
	return c;
}
