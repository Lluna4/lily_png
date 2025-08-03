
#include "utils.h"

std::expected<size_t, lily_png::png_error> lily_png::get_pixel_bit_size(const metadata &meta)
{
	size_t ret = 0;
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
				return std::unexpected(png_error::invalid_bit_depth);
			break;
		case static_cast<int>(color::indexed):
			if (meta.bit_depth == 1 || meta.bit_depth == 2 || meta.bit_depth == 4 || meta.bit_depth == 8)
			{
				ret = meta.bit_depth;
			}
			else
				return std::unexpected(png_error::invalid_bit_depth);
			break;
		case static_cast<int>(color::grayscale_alpha):
			if (meta.bit_depth == 8 || meta.bit_depth == 16)
			{
				ret = meta.bit_depth * 2;
			}
			else
				return std::unexpected(png_error::invalid_bit_depth);
			break;
		case static_cast<int>(color::rgba):
			if (meta.bit_depth == 8 || meta.bit_depth == 16)
			{
				ret = meta.bit_depth * 4;
			}
			else
				return std::unexpected(png_error::invalid_bit_depth);
			break;
		default:
			return std::unexpected(png_error::invalid_color_type);
	}
	return ret;
}

std::expected<size_t, lily_png::png_error> lily_png::get_uncompressed_size(const metadata &meta)
{
	auto pixel_ret = get_pixel_bit_size(meta);
	if (!pixel_ret)
		return std::unexpected(pixel_ret.error());
	size_t ret = (meta.width * pixel_ret.value() + 7)/8; //ceil() but for bytes
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

float lily_png::get_aspect_ratio(const metadata &meta)
{
	float aspect_ratio = 0.0f;

	if (meta.width > meta.height)
		aspect_ratio = (float)meta.width / (float)meta.height;
	else
		aspect_ratio = (float)meta.height / (float)meta.width;

	return aspect_ratio;
}

std::expected<size_t, lily_png::png_error> lily_png::image::resize_image(image &dest)
{
	size_t new_size = (dest.meta.width * pixel_size_bytes) * dest.meta.height;
	dest.buffer.allocate(new_size);

	//this implementation is bad but its only a test
	int compressed_pixels_width = meta.width / dest.meta.width;
	int compressed_pixels_height = meta.height / dest.meta.height;

	for (int y = 0; y < dest.meta.height; y++)
	{
		for (int x = 0; x < dest.meta.width; x++)
		{
			*(dest[y, x]) = *(operator[](y * compressed_pixels_height, x * compressed_pixels_width));
		}
	}
	return new_size;
}
