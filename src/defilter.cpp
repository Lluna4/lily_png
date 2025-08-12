#include "defilter.h"

std::expected<bool, lily_png::png_error> lily_png::defilter_pixel(unsigned char *dest, unsigned char x,unsigned char a, unsigned char b, unsigned char c, unsigned char type)
{
	std::println("Filter {}", type);
	switch (type)
	{
		case 0:
			*dest = x;
			break;
		case 1:
			*dest = x + a;
			break;
		case 2:
			*dest = x + b;
			break;
		case 3:

			*dest = x + floor((a + b)/2);
			break;
		case 4:
			*dest = x + paeth_predict(a, b, c);
			break;
		default:
			return std::unexpected(png_error::non_standard_filter);
	}
	return true;
}

std::expected<bool, lily_png::png_error> lily_png::defilter(file_reader::buffer<unsigned char> &src, file_reader::buffer<unsigned char> &dest, metadata &meta)
{
	auto pixel_size_ret = get_pixel_bit_size(meta);
	if (!pixel_size_ret)
		return std::unexpected(pixel_size_ret.error());
	size_t pixel_size = pixel_size_ret.value();
	size_t pixel_size_bytes = (pixel_size + 7)/8;
	size_t scanline_size = (meta.width * pixel_size + 7)/8;
	auto uncompress_ret = get_uncompressed_size(meta);
	if (!uncompress_ret)
		return std::unexpected(uncompress_ret.error());
	dest.allocate(uncompress_ret.value());
	int y = 0;
	size_t stride = scanline_size + 1;
	int x = 0;
	int index = 0;
	std::vector<unsigned char> filters;
	for (int yy = 0; yy < meta.height; yy++)
	{
		filters.push_back(src.data[index]);
		index += scanline_size + 1;
	}
	bool end = false;
	while (y <= meta.height)
	{
		if (end == false)
			x = 0;
		int x_before = x;
		for (int y2 = y; y2 >= 0; y2--)
		{
			if (x >= meta.width * pixel_size_bytes)
				break;
			unsigned char a = 0;
			if (x > pixel_size_bytes)
				a = dest.data[(y2 * scanline_size) + (x - pixel_size_bytes)];

			unsigned char b = 0;
			if (y2 > 0)
				b = dest.data[(y2 - 1) * scanline_size + x];
			
			unsigned char c = 0;
			if (y2 > 0 && x > pixel_size_bytes)
				c = dest.data[(y2 - 1) * scanline_size + (x - pixel_size_bytes)];
			auto ret = defilter_pixel(&dest.data[y2 * scanline_size + x], src.data[y2 * stride + x + 1], a, b, c, src.data[y2 * stride]);
			if (!ret)
				return std::unexpected(ret.error());
			x++;
		}
		x = x_before;
		if (end == false)
			y++;
		else
			x++;
		if (y == meta.height)
		{
			end = true;
			y--;
		}
		if (x >= meta.width * pixel_size_bytes)
			break;
	}
	return true;
}
