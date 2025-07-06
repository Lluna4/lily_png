#include "lily_png.h"

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

buffer_unsigned read_png(const std::string &file_path)
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
	buffer_unsigned image_data_concat{0};
	buffer_unsigned image_data{0};
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
		auto data = std::make_tuple(data_body, (unsigned int)0);
		auto ree = reader.read_from_tuple(data);
		unsigned long crc = crc32(0, reinterpret_cast<unsigned char *>(std::get<1>(chunk_head).data), std::get<1>(chunk_head).size);
		crc = crc32(crc, reinterpret_cast<unsigned char *>(std::get<0>(data).data), std::get<0>(data).size);
		if (crc != std::get<1>(data))
			throw std::runtime_error("Crc check failed");
		std::println("Size received {}", ree.first);
		if (strcmp("IHDR", std::get<1>(chunk_head).data) == 0)
			meta = parse_metadata(std::get<0>(data));
		else if (strcmp("IDAT", std::get<1>(chunk_head).data) == 0)
		{
			buffer &temp_buf = std::get<0>(data);
			image_data_concat.write(reinterpret_cast<unsigned char *>(temp_buf.data), temp_buf.size);
		}
		else if (strcmp("IEND",std::get<1>(chunk_head).data) == 0)
			break;
	}
	std::println("Image data size {}", image_data_concat.size);
	std::println("Image data allocated {}", image_data_concat.allocated);
	std::println("Allocations {}", image_data_concat.allocations);
	image_data.allocate(image_data_concat.allocated);
	uncompress(image_data.data, &image_data.allocated, image_data_concat.data, image_data_concat.size);
	std::println("Filter {}", (int)meta.filter);

	switch (meta.filter)
	{
		case 0:
			break;
		case 1:
			std::println("sub filter now implemented");
			break;
		case 2:
			std::println("up filter now implemented");
			break;
		case 3:
			std::println("average filter now implemented");
			break;
		case 4:
			std::println("paeth filter now implemented");
			break;
		default:
			std::println("Filter not recognised");
			break;
	}
	return image_data;
}
