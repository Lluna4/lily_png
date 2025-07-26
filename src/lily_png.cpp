#include "lily_png.h"

std::vector<lily_png::color_rgb> palette;
bool palette_found = false;

static std::expected<bool, lily_png::png_error> read_raw_data(const std::string &file_path, file_reader::buffer<unsigned char> &data, lily_png::metadata &meta)
{
	std::println("Zlib version is {}", zlibVersion());
	unsigned char magic[9] = {137, 80, 78, 71, 13, 10, 26, 10};
	file_reader::file_reader reader{};
	auto result = reader.open_file(file_path);
	if (!result)
		return std::unexpected(lily_png::png_error::file_doesnt_exist);
	char file_magic[9] = {0};
	auto res = reader.read_buffer(file_magic, 8).or_else([](const file_reader::RESULT &res)
	{
		std::println("Reading magic failed!");
		return std::expected<size_t, file_reader::RESULT>{0};
	});
	if (res.value() == 0)
		return std::unexpected(lily_png::png_error::read_failed);
	if (memcmp(magic, file_magic, 8) != 0)
	{
		std::println("File is not a png!");
		return std::unexpected(lily_png::png_error::file_is_not_a_png);
	}
	file_reader::buffer<unsigned char> raw_dat{};
	while (true)
	{
		std::tuple<unsigned int, file_reader::buffer<char>> chunk_header;
		std::get<1>(chunk_header).size = 4;
		auto ret = reader.read_from_tuple(chunk_header).or_else([](const file_reader::RESULT &res)
		{
			std::println("Reading header failed!");
			return std::expected<size_t, file_reader::RESULT>{0};
		});
		if (ret.value() != 8)
		{
			std::println("Chunk incomplete!");
			return std::unexpected(lily_png::png_error::read_failed);
		}
		unsigned int size = std::get<0>(chunk_header);
		//std::println("Chunk type {} Size {}", std::get<1>(chunk_header).data, std::get<0>(chunk_header));

		file_reader::buffer<unsigned char> raw_data{};
		raw_data.size = std::get<0>(chunk_header);
		std::tuple<file_reader::buffer<char>, unsigned> dat;
		std::get<0>(dat).size = std::get<0>(chunk_header);
		ret = reader.read_from_tuple(dat).or_else([](const file_reader::RESULT &res)
		{
			std::println("Reading data failed!");
			return std::expected<size_t, file_reader::RESULT>{0};
		});
		if (ret.value() != raw_data.size + 4)
		{
			std::println("Chunk incomplete!");
			return std::unexpected(lily_png::png_error::read_failed);
		}
		unsigned long crc = crc32(0, reinterpret_cast<unsigned char *>(std::get<1>(chunk_header).data), 4);
		crc = crc32(crc, reinterpret_cast<unsigned char *>(std::get<0>(dat).data), std::get<0>(chunk_header));
		if (crc != std::get<1>(dat))
			throw std::runtime_error("Crc check failed");


		if (strncmp(std::get<1>(chunk_header).data, "IDAT", 4) == 0)
		{
			raw_dat.write(reinterpret_cast<unsigned char *>(std::get<0>(dat).data), std::get<0>(chunk_header));
		}
		else if (strncmp(std::get<1>(chunk_header).data, "IHDR", 4) == 0)
		{
			meta = lily_png::parse_metadata(std::get<0>(dat));
		}
		else if (strncmp(std::get<1>(chunk_header).data, "PLTE", 4) == 0)
		{
			palette_found = true;
			if (size%3 != 0)
			{
				std::println("Palette size is not divisible by 3");
				return std::unexpected(lily_png::png_error::read_failed);
			}
			for (int i = 0; i < size; i += 3)
			{
				lily_png::color_rgb tmp_color;
				tmp_color.r = std::get<0>(dat).data[i];
				tmp_color.g = std::get<0>(dat).data[i + 1];
				tmp_color.b = std::get<0>(dat).data[i + 2];
				palette.push_back(tmp_color);
			}
		}
		else if (strncmp(std::get<1>(chunk_header).data, "IEND", 4) == 0)
			break;
	}

	auto uncompress_ret = get_uncompressed_size(meta);
	if (!uncompress_ret)
		return std::unexpected(uncompress_ret.error());
	data.allocate(uncompress_ret.value());
	size_t prev_allocated = data.allocated;
	int r = uncompress(data.data, &prev_allocated, raw_dat.data, raw_dat.allocated);
	if (r != Z_OK)
	{
		std::println("Uncompress failed {}", r);
		throw std::runtime_error("Uncompress fail");
	}
	return true;
}


static std::expected<bool, lily_png::png_error> apply_palette_scanline(unsigned char *scanline, unsigned char *dest, lily_png::metadata &meta)
{
	auto pixel_size_ret = get_pixel_bit_size(meta);
	if (!pixel_size_ret)
		return std::unexpected(pixel_size_ret.error());
	size_t pixel_size = pixel_size_ret.value();
	size_t pixel_size_bytes = (pixel_size + 7)/8;
	size_t scanline_size = (meta.width * pixel_size + 7)/8;
	unsigned long dest_index = 0;
	for (int i = 0; i < scanline_size; i++)
	{
		lily_png::color_rgb tmp_color = palette[scanline[i]];
		dest[dest_index] = tmp_color.r;
		dest[dest_index++] = tmp_color.g;
		dest[dest_index++] = tmp_color.b;
	}
	return true;
}

static std::expected<bool, lily_png::png_error> apply_palette(file_reader::buffer<unsigned char> &data, file_reader::buffer<unsigned char> &dest, lily_png::metadata &meta)
{
	unsigned long index = 0;
	unsigned long index_dest = 0;
	auto pixel_size_ret = get_pixel_bit_size(meta);
	if (!pixel_size_ret)
		return std::unexpected(pixel_size_ret.error());
	size_t pixel_size = pixel_size_ret.value();
	size_t pixel_size_bytes = (pixel_size + 7)/8;
	size_t scanline_size = (meta.width * pixel_size + 7)/8;
	unsigned long scanlines = 0;
	auto uncompress_ret = get_uncompressed_size(meta);
	if (!uncompress_ret)
		return std::unexpected(uncompress_ret.error());
	dest.allocate(uncompress_ret.value() * 3);
	while (scanlines < meta.height)
	{
		unsigned char filter = data.data[index];
		std::println("Filter {}", filter);
		auto ret = apply_palette_scanline(&data.data[index + 1], &dest.data[index_dest], meta);
		if (!ret)
			return std::unexpected(ret.error());
		index += scanline_size + 1;
		index_dest += scanline_size;
		scanlines++;
	}
	return true;
}

std::expected<lily_png::metadata, lily_png::png_error> lily_png::read_png(const std::string &file_path, file_reader::buffer<unsigned char> &data)
{
	file_reader::buffer<unsigned char> tmp_data{};
	metadata meta{0};
	auto ret = read_raw_data(file_path, tmp_data, meta);
	if (!ret)
	{
		return std::unexpected(ret.error());
	}
	if (palette_found == true)
	{
		file_reader::buffer<unsigned char> dest_palette{};
		auto apply_ret = apply_palette(tmp_data, dest_palette, meta);
		if (!apply_ret)
			return std::unexpected(apply_ret.error());
		tmp_data = dest_palette;
	}
	auto res = filter(tmp_data, data, meta);
	if (!res)
		return std::unexpected(res.error());
	return meta;
}
