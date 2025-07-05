#include "library.h"

#include <iostream>

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

char * read_png(const std::string &file_path)
{
	unsigned char magic[9] = {137, 80, 78, 71, 13, 10, 26, 10};
	file_reader reader(file_path);
	buffer head = {0};
	head.size = 8;
	auto head_tup = std::make_tuple(head);
	auto re = reader.read_from_tuple(head_tup);
	if (re.second == READ_INCOMPLETE || re.second == READ_FILE_ENDED)
		throw std::runtime_error("Not enough size to read header");
	if (memcmp(magic, std::get<0>(head_tup).data, 8) != 0)
		throw std::runtime_error("File is not a png");
	metadata meta{0};
	while (true)
	{
		buffer chunk_type{0};
		chunk_type.allocated = 0;
		chunk_type.data = nullptr;
		chunk_type.size = 4;
		auto chunk_head = std::make_tuple((unsigned int)0, chunk_type);
		auto ret = reader.read_from_tuple(chunk_head);
		if (ret.second == READ_INCOMPLETE || ret.second == READ_FILE_ENDED)
			throw std::runtime_error("Incomplete chunk");
		std::println("Header stats size {} type {}", std::get<0>(chunk_head), std::get<1>(chunk_head).data);
		buffer data_body{0};
		data_body.size = std::get<0>(chunk_head);
		auto data = std::make_tuple(data_body, chunk_type);
		auto ree = reader.read_from_tuple(data);
		std::println("Size received {}", ree.first);
		if (strcmp("IHDR", std::get<1>(chunk_head).data) == 0)
			meta = parse_metadata(std::get<0>(data));
	}
}
