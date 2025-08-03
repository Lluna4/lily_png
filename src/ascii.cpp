//
// Created by Luna on 27/7/25.
//

#include "ascii.h"

#include "convert.h"

void lily_png::convert_to_ascii(image &src, file_reader::buffer<char> &dest)
{
	image intermediate_img(src.meta);
	intermediate_img.meta.width = 40;
	intermediate_img.meta.height = 40;

	auto ret = src.resize_image(intermediate_img);
	if (!ret)
	{
		std::println("Error! {}", ret.error());
		return ;
	}
	size_t resized_size = ret.value();
	size_t reference_size = strlen("\033[38;2;255;255;255ma\033[0m ");
	dest.allocate(resized_size + (resized_size * intermediate_img.meta.width * intermediate_img.meta.height));
	size_t dest_index = 0;
	size_t byte_size = (intermediate_img.meta.bit_depth + 7)/8;
	size_t lines = 0;
	for (int y = 0; y < intermediate_img.meta.height; y++)
	{
		for (int x = 0; x < intermediate_img.meta.width; x++)
		{
			unsigned char *pxl = intermediate_img[y, x];
			color_rgb tmp{};
			tmp.r = pxl[0];
			tmp.g = pxl[byte_size];
			tmp.b = pxl[byte_size * 2];
			std::string format = std::format("\033[38;2;{};{};{}ma\033[0m ", tmp.r, tmp.g, tmp.b);
			memcpy(&dest.data[dest_index], format.c_str(), format.size());
			dest_index += format.size();
			lines++;
			if (lines == intermediate_img.meta.width)
			{
				dest.data[dest_index] = '\n';
				dest_index++;
				lines = 0;
			}
		}
	}
}
