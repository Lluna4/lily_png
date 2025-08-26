#define NS_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION

#include "defilter.h"
#include "Foundation/Foundation.hpp"
#include "Metal/Metal.hpp"
#include <fstream>

struct parameters
{
	int y_start;
	int x_start;
	int scanline_size;
	int bytes_per_pixel;
};

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

	std::ifstream file;
	file.open("shaders/compute.metal");
	std::stringstream reader;
	reader << file.rdbuf();
	std::string str = reader.str();

	NS::String *shader_code = NS::String::string(str.c_str(), NS::StringEncoding::UTF8StringEncoding);

	NS::Error *err = nullptr;
	MTL::CompileOptions *options = nullptr;

	MTL::Library *lib = dev->newLibrary(shader_code, options, &err);
	if (!lib)
	{
		std::println("{}", err->localizedDescription()->cString(NS::StringEncoding::UTF8StringEncoding));
		return std::unexpected(png_error::read_failed);
	}
	NS::String *comp_func_name = NS::String::string("defilter", NS::StringEncoding::UTF8StringEncoding);
	MTL::Function *func = lib->newFunction(comp_func_name);

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

	MTL::Buffer *buf = dev->newBuffer(uncompress_ret.value() + 1, MTL::ResourceStorageModeShared);
	MTL::Buffer *gpu_ret = dev->newBuffer(uncompress_ret.value() + 1, MTL::ResourceStorageModeShared);


	MTL::Buffer *p = dev->newBuffer(sizeof(parameters), MTL::ResourceStorageModeShared);
	parameters *p_ptr = static_cast<parameters *>(p->contents());

	memcpy(buf->contents(), src.data, uncompress_ret.value());
	MTL::CommandQueue *com_queue = dev->newCommandQueue();
	p_ptr->bytes_per_pixel = pixel_size_bytes;
	p_ptr->scanline_size = scanline_size;

	MTL::SharedEvent *shared_event = dev->newSharedEvent();
	MTL::SharedEventListener *list;
	shared_event->setSignaledValue(1);
	while (true)
	{

		if (y == meta.height)
		{
			y_end = true;
			y--;
		}

		MTL::CommandBuffer *com_buffer = com_queue->commandBuffer();
		MTL::ComputeCommandEncoder *com_encoder = com_buffer->computeCommandEncoder();
		p_ptr->x_start = x;
		p_ptr->y_start = y;
		com_encoder->setComputePipelineState(compute_pipeline);
		com_encoder->setBuffer(buf, 0, 0);
		com_encoder->setBuffer(gpu_ret, 0, 1);
		com_encoder->setBuffer(p, 0, 2);
		MTL::Size grid_size = MTL::Size(y + 1, 1, 1);
		MTL::Size threadgroupsize;
		threadgroupsize = MTL::Size(1, 1, 1);
		com_encoder->dispatchThreadgroups(grid_size, threadgroupsize);
		com_encoder->endEncoding();
		com_buffer->encodeSignalEvent(shared_event, 1);
		com_buffer->encodeWait(shared_event, 0);
		shared_event->waitUntilSignaledValue(1, -1);
		shared_event->setSignaledValue(0);
		com_buffer->commit();
		//com_buffer->waitUntilCompleted();
		com_encoder->release();
		com_buffer->release();

		if (y_end == false)
			y++;
		else
			x++;

		if (x >= meta.width * pixel_size_bytes)
			break;
	}
	memcpy(dest.data, gpu_ret->contents(), uncompress_ret.value());
	com_queue->release();
	buf->release();
	gpu_ret->release();
	compute_pipeline->release();
	func->release();
	lib->release();
	dev->release();
	return true;
}
