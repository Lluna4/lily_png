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

std::expected<bool, lily_png::png_error> lily_png::defilter(image &src, image &dest)
{
	unsigned long scanlines = 0;
	auto uncompress_ret = get_uncompressed_size(src.meta);
	if (!uncompress_ret)
		return std::unexpected(uncompress_ret.error());
	dest.buffer.allocate(uncompress_ret.value());
	for (int y = 0; y < src.meta.height; y++)
	{
		int x = 0;
		int y2 = y;
		while (y2 >= 0)
		{
			std::println("x{} y{}", x, y2);
			unsigned char *a_ptr = dest[y2, x - 1];
			unsigned char a = 0;
			if (a_ptr != nullptr)
				a = *a_ptr;

			unsigned char *b_ptr = dest[y2 - 1, x];
			unsigned char b = 0;
			if (b_ptr != nullptr)
				b = *b_ptr;

			unsigned char *c_ptr = dest[y2 - 1, x - 1];
			unsigned char c = 0;
			if (c_ptr != nullptr)
				c = *c_ptr;
			auto ret = defilter_pixel(dest[y2, x], *src[y2, x], a, b, c, src.buffer.data[(y2 * (src.meta.width + 1))+ x]);
			if (!ret)
				return std::unexpected(ret.error());
			y2--;
			x++;
		}
	}
	return true;
}
