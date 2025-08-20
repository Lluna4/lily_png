#define NS_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION

#include "defilter.h"
#include "Foundation/Foundation.hpp"
#include "Metal/Metal.hpp"

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

static void loop_diag(unsigned char *src, unsigned char *dest, lily_png::metadata &meta, int x, int y, int y_limit)
{
	auto pixel_size_ret = get_pixel_bit_size(meta);
	if (!pixel_size_ret)
		return ;
	size_t pixel_size = pixel_size_ret.value();
	size_t pixel_size_bytes = (pixel_size + 7)/8;
	size_t scanline_size = (meta.width * pixel_size + 7)/8;
	size_t stride = scanline_size + 1;

	for (int y2 = y; y2 >= 0; y2--)
	{
		if (x >= meta.width * pixel_size_bytes)
			break;

		unsigned char a = 0;
		if (x >= pixel_size_bytes)
			a = dest[(y2 * scanline_size) + (x - pixel_size_bytes)];

		unsigned char b = 0;
		if (y2 > 0)
			b = dest[((y2 - 1) * scanline_size) + x];
			
		unsigned char c = 0;
		if (y2 > 0 && x >= pixel_size_bytes)
				c = dest[((y2 - 1) * scanline_size) + (x - pixel_size_bytes)];

		auto ret = lily_png::defilter_pixel(&dest[(y2 * scanline_size) + x], src[(y2 * stride) + x + 1], a, b, c, src[y2 * stride]);
		if (!ret)
			return ;
		x++;
	}
}

std::expected<bool, lily_png::png_error> lily_png::defilter(file_reader::buffer<unsigned char> &src, file_reader::buffer<unsigned char> &dest, metadata &meta)
{
	MTL::Device *dev = MTL::CreateSystemDefaultDevice();

	MTL::Library *lib = dev->newDefaultLibrary();

	MTL::Function *func = lib->newFunction(NS::String::string("defilter", NS::ASCIIStringEncoding));

	NS::Error *error = nullptr;
	MTL::ComputePipelineState *compute_pipeline = dev->newComputePipelineState(func, &error);
	if (error)
		return std::unexpected(png_error::read_failed);


	auto pixel_size_ret = get_pixel_bit_size(meta);
	if (!pixel_size_ret)
		return std::unexpected(pixel_size_ret.error());
	size_t pixel_size = pixel_size_ret.value();
	size_t pixel_size_bytes = (pixel_size + 7)/8;
	size_t scanline_size = (meta.width * pixel_size + 7)/8;
	auto uncompress_ret = get_uncompressed_size(meta);
	if (!uncompress_ret)
		return std::unexpected(uncompress_ret.error());
	dest.allocate(uncompress_ret.value());
	int y = 0;
	size_t stride = scanline_size + 1;
	bool y_end = false;
	int x = 0;
	int x_before = 0;

	while (true)
	{
		if (y == meta.height)
		{
			y_end = true;
			y--;
		}
		if (y_end == false)
			x = 0;
		
		x_before = x;
		MTL::Buffer *buf = dev->newBuffer(y, MTL::ResourceStorageModeShared);

		MTL::CommandQueue *com_queue = dev->newCommandQueue();
		MTL::CommandBuffer *com_buffer = com_queue->commandBuffer();
		MTL::ComputeCommandEncoder *com_encoder = com_buffer->computeCommandEncoder();

		com_encoder->setComputePipelineState(compute_pipeline);
		com_encoder->setBuffer(buf, 0, 0);

		if (y_end == false)
			y++;
		else
			x++;
		
		if (x >= meta.width * pixel_size_bytes)
			break;
	}
	return true;
}
