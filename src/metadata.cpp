#include "metadata.h"


metadata parse_metadata(buffer &data)
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