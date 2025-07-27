//
// Created by Luna on 27/7/25.
//

#include "ascii.h"

#include "convert.h"

void lily_png::convert_to_ascii(file_reader::buffer<unsigned char> &src, file_reader::buffer<char> &dest, metadata &meta)
{
	auto pixel_size_ret = get_pixel_bit_size(meta);
	if (!pixel_size_ret)
		return ;
	size_t pixel_size = pixel_size_ret.value();
	size_t pixel_size_bytes = (pixel_size + 7)/8;
	float aspect_ratio = get_aspect_ratio(meta);
	metadata new_meta = meta;
	new_meta.width = 40;
	new_meta.height = 40;
	file_reader::buffer<unsigned char> intermediate_resize{};
	auto resize_ret = resize_image(src, intermediate_resize, meta, new_meta);
	if (!resize_ret)
		return ;
	size_t intermediate_size = resize_ret.value();
	size_t char_size = strlen("\033[38;2;255;255;255ma\033[0m ");
	size_t width_char  = char_size * new_meta.width;
	size_t full_size = width_char * new_meta.height + new_meta.width;

	dest.allocate(full_size);
	size_t dest_index = 0;
	size_t line_done = 0;

	for (int i = 0; i < intermediate_size; i += pixel_size_bytes)
	{
		color_rgb tmp_color{};
		tmp_color.r = intermediate_resize.data[i];
		tmp_color.g = intermediate_resize.data[i + 1];
		tmp_color.b = intermediate_resize.data[i + 2];
		std::string format = std::format("\033[38;2;{};{};{}ma\033[0m ", tmp_color.r, tmp_color.g, tmp_color.b);
		memcpy(&dest.data[dest_index], format.c_str(), format.size());
		dest_index += format.size();
		line_done++;
		if (line_done == new_meta.width)
		{
			dest.data[dest_index] = '\n';
			dest_index++;
			line_done = 0;
		}
		if (dest_index >= full_size)
		{
			dest.data[dest_index] = '\0';
			break;
		}
	}
}
