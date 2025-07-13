#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "lily_png.h"
#include <math.h>

static metadata parse_metadata(buffer &data)
{
	std::tuple<unsigned int, unsigned int, char, char, char, char, char> meta;
	constexpr std::size_t size = std::tuple_size_v<decltype(meta)>;
	parsing_buffer par_buf(data);
	par_buf.point = par_buf.buf.data;
	par_buf.consumed_size = 0;
	read_comp(size, par_buf, meta);
	std::println("Width {} height {}", std::get<0>(meta), std::get<1>(meta));
	metadata m{0};
	m.width = std::get<0>(meta);
	m.height = std::get<1>(meta);
	m.bit_depth = std::get<2>(meta);
	m.color_type = std::get<3>(meta);
	m.compression = std::get<4>(meta);
	m.filter = std::get<5>(meta);
	m.interface = std::get<6>(meta);
	std::println("metadata{{ width: {}, height: {}, bit_depth: {}, color_type: {}, compression: {}, filter: {}, interface: {} }}", m.width, m.height, (int)m.bit_depth, (int)m.color_type, (int)m.compression, (int)m.filter, (int)m.interface);
	return m;
}

static size_t get_pixel_bit_size(const metadata &meta)
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

static size_t get_uncompressed_size(const metadata meta)
{
	size_t ret = 0;
	switch (meta.color_type)
	{
		case 0:
			if (meta.bit_depth == 1 || meta.bit_depth == 2 || meta.bit_depth == 4 || meta.bit_depth == 8 || meta.bit_depth == 16)
			{
				size_t size_per_row = ceil((meta.width * (meta.bit_depth))/8) + 1;
				ret = size_per_row * meta.height;
			}
			else
				throw std::runtime_error("Invalid bit depht");
			break;
		case 2:
			if (meta.bit_depth == 8 || meta.bit_depth == 16)
			{
				size_t size_per_row = ceil((meta.width * (meta.bit_depth * 3))/8) + 1;
				ret = size_per_row * meta.height;
			}
			else
				throw std::runtime_error("Invalid bit depht");
			break;
		case 3:
			if (meta.bit_depth == 1 || meta.bit_depth == 2 || meta.bit_depth == 4 || meta.bit_depth == 8)
			{
				size_t size_per_row = ceil((meta.width * (meta.bit_depth))/8) + 1;
				ret = size_per_row * meta.height;
			}
			else
				throw std::runtime_error("Invalid bit depht");
			break;
		case 4:
			if (meta.bit_depth == 8 || meta.bit_depth == 16)
			{
				size_t size_per_row = ceil((meta.width * (meta.bit_depth * 2))/8) + 1;
				ret = size_per_row * meta.height;
			}
			else
				throw std::runtime_error("Invalid bit depht");
			break;
		case 6:
			if (meta.bit_depth == 8 || meta.bit_depth == 16)
			{
				size_t size_per_row = ceil((meta.width * (meta.bit_depth * 4))/8) + 1;
				ret = size_per_row * meta.height;
			}
			else
				throw std::runtime_error("Invalid bit depht");
			break;
		default:
			throw std::runtime_error("Invalid color type");
	}
	std::println("Uncompressed size {}", ret);
	return ret;
}

static void read_raw_data(const std::string &file_path, buffer_unsigned &data, metadata &meta)
{
	unsigned char magic[9] = {137, 80, 78, 71, 13, 10, 26, 10};
	file_reader reader(file_path);
	char file_magic[9] = {0};
	reader.read_buffer(file_magic, 8);
	if (memcmp(magic, file_magic, 8) != 0)
	{
		std::println("File is not a png!");
		return ;
	}
	buffer_unsigned raw_dat{};
	while (true)
	{
		std::tuple<unsigned int, buffer> chunk_header;
		std::get<1>(chunk_header).size = 4;
		auto ret = reader.read_from_tuple(chunk_header);
		if (ret.second == READ_FILE_ENDED || ret.second == READ_INCOMPLETE)
		{
			std::println("Chunk incomplete!");
			return ;
		}

		std::println("Chunk type {} Size {}", std::get<1>(chunk_header).data, std::get<0>(chunk_header));

		buffer_unsigned raw_data{};
		raw_data.size = std::get<0>(chunk_header);
		std::tuple<buffer, unsigned> dat;
		std::get<0>(dat).size = std::get<0>(chunk_header);
		ret = reader.read_from_tuple(dat);
		if (ret.second == READ_FILE_ENDED || ret.second == READ_INCOMPLETE)
		{
			std::println("Chunk incomplete!");
			return ;
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
			meta = parse_metadata(std::get<0>(dat));
		}
		else if (strncmp(std::get<1>(chunk_header).data, "IEND", 4) == 0)
			break;
	}

	data.allocate(get_uncompressed_size(meta));
	size_t prev_allocated = data.allocated;
	int r = uncompress(data.data, &prev_allocated, raw_dat.data, raw_dat.allocated);
	if (r != Z_OK)
	{
		std::println("Uncompress failed {}", r);
		throw std::runtime_error("Uncompress fail");
	}
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

static void filter_scanline(unsigned char *scanline, unsigned char *previous_scanline, unsigned char *dest, metadata &meta, unsigned char filter_type)
{
	size_t pixel_size = get_pixel_bit_size(meta);
	for (int i = 0; i < ceil(pixel_size/8.0f) * meta.width; i++)
	{
		long index_before = 0;
		switch (filter_type)
		{
			case 0:
				dest[i] = scanline[i];
				break;
			case 1:
				if (pixel_size < 8)
					index_before = i - 1;
				else
					index_before = i - ceil(pixel_size/8.0f);
				if (index_before <= 0)
					dest[i] = scanline[i] + 0;
				else
					dest[i] = scanline[i] + dest[index_before];
				break;
			case 2:
				dest[i] = scanline[i] + previous_scanline[i];
				break;
			case 3:
				if (pixel_size < 8)
					index_before = i - 1;
				else
					index_before = i - ceil(pixel_size/8.0f);
				if (index_before <= 0)
					dest[i] = scanline[i] + floor((0 + previous_scanline[i])/2);
				else
					dest[i] = scanline[i] + floor((dest[index_before] + previous_scanline[i])/2);
				break;
			case 4:
				if (pixel_size < 8)
					index_before = i - 1;
				else
					index_before = i - ceil(pixel_size/8.0f);
				if (index_before <= 0)
					dest[i] = scanline[i] + paeth_predict(0, previous_scanline[i], 0);
				else
					dest[i] = scanline[i] + paeth_predict(dest[index_before], previous_scanline[i], previous_scanline[index_before]);
				break;
			default:
				std::println("Non standard filter");
				break;
		}
	}
}

static void filter(buffer_unsigned &data, buffer_unsigned &dest ,metadata &meta)
{
	unsigned long index = 0;
	unsigned long index_dest = 0;
	size_t pixel_size = get_pixel_bit_size(meta);
	unsigned long scanlines = 0;
	dest.allocate(get_uncompressed_size(meta));
	unsigned char *previous_scanline = nullptr;
	while (scanlines < meta.height)
	{
		unsigned char filter = data.data[index];
		std::println("Filter {}", filter);
		filter_scanline(&data.data[index++], previous_scanline, &dest.data[index_dest], meta, filter);
		previous_scanline = &dest.data[index_dest];
		index += ceil(pixel_size/8.0f) * meta.width;
		index_dest += ceil(pixel_size/8.0f) * meta.width;
		scanlines++;
	}
}


void read_png(const std::string &file_path, buffer_unsigned &data)
{
	buffer_unsigned tmp_data{};
	metadata meta{0};
	read_raw_data(file_path, tmp_data, meta);
	filter(tmp_data, data, meta);
}
