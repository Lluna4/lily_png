
#include "utils.h"

size_t get_pixel_bit_size(const metadata &meta)
{
	size_t ret = 0;
	switch (meta.color_type)
	{
		case 0:
			if (meta.bit_depth == 1 || meta.bit_depth == 2 || meta.bit_depth == 4 || meta.bit_depth == 8 || meta.bit_depth == 16)
			{
				ret = meta.bit_depth;
			}
			else
				throw std::runtime_error("Invalid bit depht");
			break;
		case 2:
			if (meta.bit_depth == 8 || meta.bit_depth == 16)
			{
				ret = meta.bit_depth * 3;
			}
			else
				throw std::runtime_error("Invalid bit depht");
			break;
		case 3:
			if (meta.bit_depth == 1 || meta.bit_depth == 2 || meta.bit_depth == 4 || meta.bit_depth == 8)
			{
				ret = meta.bit_depth;
			}
			else
				throw std::runtime_error("Invalid bit depht");
			break;
		case 4:
			if (meta.bit_depth == 8 || meta.bit_depth == 16)
			{
				ret = meta.bit_depth * 2;
			}
			else
				throw std::runtime_error("Invalid bit depht");
			break;
		case 6:
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

size_t get_uncompressed_size(const metadata meta)
{
	size_t ret = (meta.width * get_pixel_bit_size(meta) + 7)/8; //ceil() but for bytes
	ret = (ret + 1) * meta.height;
	std::println("Uncompressed size {}", ret);
	return ret;
}

int paeth_predict(int a, int b, int c)
{
	int pred = a+b-c;
	int pred1 = abs(pred - a);
	int pred2 = abs(pred - b);
	int pred3 = abs(pred - c);
	if (pred1 <= pred2 && pred1 <= pred3)
		return a;
	if (pred2 <= pred3)
		return b;
	return c;
}
