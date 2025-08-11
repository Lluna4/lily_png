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

std::expected<bool, lily_png::png_error> lily_png::defilter(file_reader::buffer<unsigned char> &src, image &dest)
{
	unsigned long scanlines = 0;
	auto uncompress_ret = get_uncompressed_size(dest.meta);
	if (!uncompress_ret)
		return std::unexpected(uncompress_ret.error());
	dest.buffer.allocate(uncompress_ret.value());
	
	auto pixel_size_ret = get_pixel_bit_size(dest.meta);
	if (!pixel_size_ret)
		return std::unexpected(pixel_size_ret.error());
	size_t pixel_size = pixel_size_ret.value();
	size_t scanline_size = (dest.meta.width * pixel_size + 7)/8;
	size_t stride = scanline_size + 1;
	for (int y = 0; y < dest.meta.height; y++)
	{
		int x = 0;
		int y2 = y;
		while (y2 >= 0)
		{
			std::println("x{} y{}", x, y2);
			size_t current_row_offset = (size_t)y2 * stride;
			size_t prev_row_offset = (size_t)(y2 - 1) * stride;

			unsigned char a = 0;
			if (x > 0) {
				// Current row, previous pixel column.
				a = dest.buffer.data[current_row_offset + 1 + (x - 1)];
			}

			// 'b' is the pixel above: Recon(x, y-1)
			unsigned char b = 0;
			if (y2 > 0) {
				// Previous row, same pixel column.
				b = dest.buffer.data[prev_row_offset + 1 + x];
			}

			// 'c' is the pixel top-left: Recon(x-1, y-1)
			unsigned char c = 0;
			if (x > 0 && y2 > 0) {
				// Previous row, previous pixel column.
				c = dest.buffer.data[prev_row_offset + 1 + (x - 1)];
			}

			auto ret = defilter_pixel(&dest.buffer.data[y2 * scanline_size + x], src.data[current_row_offset + 1 + x], a, b, c, src.data[current_row_offset]);
			if (!ret)
				return std::unexpected(ret.error());
			y2--;
			x++;
		}
	}
	return true;
}
