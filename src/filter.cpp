#include "filter.h"

void lily_png::filter_scanline(unsigned char *scanline, unsigned char *previous_scanline, unsigned char *dest, metadata &meta, unsigned char filter_type)
{
	size_t pixel_size = get_pixel_bit_size(meta);
	size_t pixel_size_bytes = (pixel_size + 7)/8;
	size_t scanline_size = (meta.width * pixel_size + 7)/8;
	for (int i = 0; i < scanline_size; i++)
	{
		long index_before = 0;
		unsigned char a = 0;
		unsigned char b = 0;
		unsigned char c = 0;

		if (i >= pixel_size_bytes)
			a = dest[i - pixel_size_bytes];
		if (previous_scanline != nullptr)
			b = previous_scanline[i];
		if (previous_scanline != nullptr && i >= pixel_size_bytes)
			c = previous_scanline[i - pixel_size_bytes];
		switch (filter_type)
		{
			case 0:
				dest[i] = scanline[i];
				break;
			case 1:
				dest[i] = scanline[i] + a;
				break;
			case 2:
				dest[i] = scanline[i] + b;
				break;
			case 3:

				dest[i] = scanline[i] + floor((a + b)/2);
				break;
			case 4:
				dest[i] = scanline[i] + paeth_predict(a, b, c);
				break;
			default:
				std::println("Non standard filter");
				break;
		}
	}
}

void lily_png::filter(file_reader::buffer<unsigned char> &data, file_reader::buffer<unsigned char> &dest , metadata &meta)
{
	unsigned long index = 0;
	unsigned long index_dest = 0;
	size_t pixel_size = get_pixel_bit_size(meta);
	size_t pixel_size_bytes = (pixel_size + 7)/8;
	size_t scanline_size = (meta.width * pixel_size + 7)/8;
	unsigned long scanlines = 0;
	dest.allocate(get_uncompressed_size(meta));
	unsigned char *previous_scanline = nullptr;
	while (scanlines < meta.height)
	{
		unsigned char filter = data.data[index];
		std::println("Filter {}", filter);
		filter_scanline(&data.data[index + 1], previous_scanline, &dest.data[index_dest], meta, filter);
		previous_scanline = &dest.data[index_dest];
		index += scanline_size + 1;
		index_dest += scanline_size;
		scanlines++;
	}
}