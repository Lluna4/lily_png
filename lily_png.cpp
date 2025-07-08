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

void read_png(const std::string &file_path, buffer_unsigned &data)
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
	metadata meta{};
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
