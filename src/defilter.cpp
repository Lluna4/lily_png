#define NS_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION

#include "defilter.h"
#include <vulkan/vulkan.hpp>
#include <fstream>
#include <sstream>
#include <filesystem>

struct parameters
{
	int y_start;
	int x_start;
	int scanline_size;
	int bytes_per_pixel;
	int stride;
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

enum class Error
{
	NO_SUITABLE_QUEUE_FAMILY_FOUND, 
	NO_SUITABLE_MEMORY_FOUND
};


template <>
struct std::formatter<Error>
{

    constexpr auto parse(std::format_parse_context& ctx) {
        return ctx.begin();
    }

    auto format(const Error& id, std::format_context& ctx) const
    {
        if (id == Error::NO_SUITABLE_QUEUE_FAMILY_FOUND)
            return std::format_to(ctx.out(), "{}", "A suitable queue family has not been found");
		if (id == Error::NO_SUITABLE_MEMORY_FOUND)
			return std::format_to(ctx.out(), "{}", "A suitable memory type has not been found");
        return std::format_to(ctx.out(), "{}", "Unknown error");
    }
};


vk::PhysicalDevice get_physical_device(vk::Instance &instance)
{
	std::vector<vk::PhysicalDevice> devices = instance.enumeratePhysicalDevices();
	std::vector<int> scores;
	for (vk::PhysicalDevice &device: devices) // score is only representative for compute
	{
		vk::PhysicalDeviceProperties device_propierties = device.getProperties();

		scores.push_back((device_propierties.limits.maxComputeSharedMemorySize + device_propierties.limits.maxComputeWorkGroupCount.front() + device_propierties.limits.maxComputeWorkGroupInvocations + device_propierties.limits.maxComputeWorkGroupSize.front())/4);
	}

	int max_index = 0;
	for (int i = 1; i < scores.size(); i++)
	{
		if (scores[i] > scores[max_index])
			max_index = i;
	}
	
	std::println("Max score {}", scores[max_index]);
	return devices[max_index];
}

std::expected<int,Error> get_compute_queue_family_index(vk::PhysicalDevice &physical_device)
{
	std::vector<vk::QueueFamilyProperties> queue_propierties = physical_device.getQueueFamilyProperties();
	int index = 0;

	for (int i = 0; i < queue_propierties.size(); i++)
	{
		if (queue_propierties[i].queueFlags & vk::QueueFlagBits::eCompute)
		{
			return i;
		}
	}
	return std::unexpected(Error::NO_SUITABLE_QUEUE_FAMILY_FOUND);
}

std::expected<std::pair<vk::DeviceMemory, vk::Buffer>, Error> create_buffer(const vk::Device &device, vk::PhysicalDevice selected_physical_device, vk::BufferUsageFlagBits usage, size_t size)
{
    vk::BufferCreateInfo buffer_info = vk::BufferCreateInfo(vk::BufferCreateFlags(), size, usage, vk::SharingMode::eExclusive);
    vk::Buffer vertex_buffer = device.createBuffer(buffer_info);
    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(device, vertex_buffer, &memory_requirements);
    vk::PhysicalDeviceMemoryProperties memory_properties = selected_physical_device.getMemoryProperties();

    int propierty_index = -1;
    for (int i = 0; i < memory_properties.memoryTypeCount; i++)
    {
        if (memory_properties.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlags(vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostCached))
        {
            propierty_index = i;
            break;
        }
    }
    if (propierty_index == -1)
    {
		return std::unexpected(Error::NO_SUITABLE_MEMORY_FOUND);
    }

    vk::MemoryAllocateInfo alloc_info = vk::MemoryAllocateInfo(memory_requirements.size, propierty_index);

    vk::DeviceMemory vertex_buffer_memory = device.allocateMemory(alloc_info);
    device.bindBufferMemory(vertex_buffer, vertex_buffer_memory, 0);
    return std::make_pair(vertex_buffer_memory, vertex_buffer);
}

std::string get_file_contents(std::string filename)
{
	std::ifstream file;
	file.open(filename);
	std::stringstream reader;
	reader << file.rdbuf();
	return reader.str();
}

std::expected<bool, lily_png::png_error> lily_png::defilter(file_reader::buffer<unsigned char> &src, file_reader::buffer<unsigned char> &dest, metadata &meta)
{
	using clock = std::chrono::system_clock;
    using ms = std::chrono::duration<double, std::milli>;
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

	vk::ApplicationInfo app_info("Test_compute", 1, nullptr, 0, VK_API_VERSION_1_4);
	
	#ifdef NDEBUG
	std::vector<const char *> layers = {};
	#else
	std::vector<const char *> layers = {"VK_LAYER_KHRONOS_validation"};
	std::println("Debug mode");
	#endif
	#ifdef __APPLE__
	std::vector<const char *> extensions = {"VK_KHR_portability_enumeration", VK_EXT_DEBUG_UTILS_EXTENSION_NAME};
	#else
	std::vector<const char *> extensions = {};
	#endif
	vk::InstanceCreateInfo instance_info(vk::InstanceCreateFlags(VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR), &app_info, layers.size(), layers.data(), extensions.size(), extensions.data());

	vk::Instance instance = vk::createInstance(instance_info);

	vk::PhysicalDevice physical_device = get_physical_device(instance);

	std::println("Selected device: {}", physical_device.getProperties().deviceName.data());

	auto queue_family_index_ret = get_compute_queue_family_index(physical_device);

	if (!queue_family_index_ret)
	{
		std::println("{}", queue_family_index_ret.error());
		return -1;
	}
	int queue_family_index = queue_family_index_ret.value();

	std::println("queue family index is {}", queue_family_index);

	float queue_priority = 1.0f;
	vk::DeviceQueueCreateInfo device_queue_info(vk::DeviceQueueCreateFlags(), queue_family_index, 1, &queue_priority);

	#ifdef __APPLE__
	std::vector<const char *> device_extensions = {"VK_KHR_portability_subset", "VK_KHR_8bit_storage", "VK_KHR_shader_float16_int8"};
	#else
	std::vector<const char *> device_extensions = {"VK_KHR_8bit_storage", "VK_KHR_shader_float16_int8"};
	#endif
	vk::PhysicalDeviceFeatures device_features = vk::PhysicalDeviceFeatures();
	vk::PhysicalDeviceVulkan12Features vulkan_1_2_features = vk::PhysicalDeviceVulkan12Features();
	vulkan_1_2_features.shaderInt8 = VK_TRUE;
	vulkan_1_2_features.uniformAndStorageBuffer8BitAccess = VK_TRUE;
	vk::DeviceCreateInfo device_info(vk::DeviceCreateFlags(), 1, &device_queue_info, 0, nullptr, device_extensions.size(), device_extensions.data(), &device_features);
	device_info.setPNext(&vulkan_1_2_features);
	vk::Device device = physical_device.createDevice(device_info);


	size_t size = uncompress_ret.value();
	auto buffer_in_ret_ret = create_buffer(device, physical_device, vk::BufferUsageFlagBits::eStorageBuffer, size);
	if (!buffer_in_ret_ret)
	{
		std::println("{}", buffer_in_ret_ret.error());
		return -1;
	}
	auto buffer_in_ret = buffer_in_ret_ret.value();
	vk::DeviceMemory buffer_in_memory = buffer_in_ret.first;
	vk::Buffer buffer_in = buffer_in_ret.second;

	
	float *in_data = (float *)device.mapMemory(buffer_in_memory, 0, size);
	std::memcpy(in_data, src.data, size);

	auto buffer_out_ret_ret = create_buffer(device, physical_device, vk::BufferUsageFlagBits::eStorageBuffer, size);
	if (!buffer_out_ret_ret)
	{
		std::println("{}", buffer_out_ret_ret.error());
		return -1;
	}
	
	auto buffer_out_ret = buffer_out_ret_ret.value();
	vk::DeviceMemory buffer_out_memory = buffer_out_ret.first;
	vk::Buffer buffer_out = buffer_out_ret.second;

	parameters param{};

	if (!std::filesystem::exists("/Users/luna/lily_png/shaders/a.spv"))
	{
		std::println("Shader file not found");
		return -1;
	}
	std::string compiled_code = get_file_contents("/Users/luna/lily_png/shaders/a.spv");
	vk::ShaderModuleCreateInfo shader_info(vk::ShaderModuleCreateFlags(), compiled_code.size(), reinterpret_cast<const uint32_t*>(compiled_code.c_str()));
	vk::ShaderModule shader_module = device.createShaderModule(shader_info);


	const std::vector<vk::DescriptorSetLayoutBinding> descriptor_set_layout_binding = 
	{
		{0, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute},
		{1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute}
	};
	vk::DescriptorSetLayoutCreateInfo descriptor_layout_info(vk::DescriptorSetLayoutCreateFlags(), descriptor_set_layout_binding);
	vk::DescriptorSetLayout descriptor_set_layout = device.createDescriptorSetLayout(descriptor_layout_info);

	vk::PushConstantRange push_range(vk::ShaderStageFlagBits::eCompute, 0, sizeof(parameters));

	vk::PipelineLayoutCreateInfo pipeline_layout_info(vk::PipelineLayoutCreateFlags(), descriptor_set_layout, push_range);
	vk::PipelineLayout pipeline_layout = device.createPipelineLayout(pipeline_layout_info);
	vk::PipelineCache pipeline_cache = device.createPipelineCache(vk::PipelineCacheCreateInfo());

	vk::PipelineShaderStageCreateInfo pipeline_shader_info(vk::PipelineShaderStageCreateFlags(), vk::ShaderStageFlagBits::eCompute, shader_module, "main");
	vk::ComputePipelineCreateInfo compute_pipeline_info(vk::PipelineCreateFlags(), pipeline_shader_info, pipeline_layout);
	vk::Pipeline pipeline = device.createComputePipeline(pipeline_cache, compute_pipeline_info).value;

	
	vk::DescriptorPoolSize descriptor_pool_size(vk::DescriptorType::eStorageBuffer, 2);
	vk::DescriptorPoolCreateInfo descriptor_pool_info(vk::DescriptorPoolCreateFlags(), 1, descriptor_pool_size);
	vk::DescriptorPool descriptor_pool = device.createDescriptorPool(descriptor_pool_info);

	vk::DescriptorSetAllocateInfo descriptor_alloc_info(descriptor_pool, 1, &descriptor_set_layout);
	std::vector<vk::DescriptorSet> descriptor_sets = device.allocateDescriptorSets(descriptor_alloc_info);
	vk::DescriptorBufferInfo buffer_in_info(buffer_in, 0, size);
	vk::DescriptorBufferInfo buffer_out_info(buffer_out, 0, size);
	std::vector<vk::WriteDescriptorSet> write_descriptor_sets =
	{
		{descriptor_sets[0], 0, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &buffer_in_info},
		{descriptor_sets[0], 1, 0, 1, vk::DescriptorType::eStorageBuffer, nullptr, &buffer_out_info}
	};
	device.updateDescriptorSets(write_descriptor_sets, {});

	param.bytes_per_pixel = pixel_size_bytes;
	param.scanline_size = scanline_size;
	param.stride = stride;
	vk::CommandPoolCreateInfo command_pool_info(vk::CommandPoolCreateFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer), queue_family_index);
	vk::CommandPool command_pool = device.createCommandPool(command_pool_info);

	vk::CommandBufferAllocateInfo command_buffer_alloc_info(command_pool, vk::CommandBufferLevel::ePrimary, 1);
	std::vector<vk::CommandBuffer> command_buffers = device.allocateCommandBuffers(command_buffer_alloc_info);

	vk::CommandBuffer command_buffer = command_buffers.front();
	vk::Fence fence = device.createFence(vk::FenceCreateInfo());
	vk::Queue queue = device.getQueue(queue_family_index, 0);
	vk::MemoryBarrier memory_barrier(
		vk::AccessFlagBits::eShaderWrite,
		vk::AccessFlagBits::eShaderRead
	);
	vk::CommandBufferBeginInfo command_buffer_begin_info(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
	command_buffer.begin(command_buffer_begin_info);
	command_buffer.bindPipeline(vk::PipelineBindPoint::eCompute, pipeline);
	command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipeline_layout, 0, {descriptor_sets[0]}, {});
	while (true)
	{
		command_buffer.pushConstants(pipeline_layout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(parameters), &param);
		command_buffer.dispatch(y + 1, 1, 1);
        command_buffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eComputeShader,
            vk::PipelineStageFlagBits::eComputeShader,
            vk::DependencyFlags(),
            {memory_barrier},
            {},
            {}
        );
		if (!y_end)
			y++;
		else
			x++;
		if (y >= meta.height)
		{
			y_end = true;
			y--;
		}
		if (x >= scanline_size)
			break;
		param.x_start = x;
		param.y_start = y;
	}
	command_buffer.end();
	vk::SubmitInfo submit_info(0, nullptr, nullptr, 1, &command_buffer);
	queue.submit({submit_info}, fence);
	auto res = device.waitForFences({fence}, true, -1);
	char *out_data = (char *)device.mapMemory(buffer_out_memory, 0, size);
	std::memcpy(dest.data, out_data, size);


	device.unmapMemory(buffer_in_memory);
	device.unmapMemory(buffer_out_memory);
	device.destroyFence(fence);
	device.destroyCommandPool(command_pool);
	device.destroyPipelineLayout(pipeline_layout);
	device.destroyPipelineCache(pipeline_cache);
	device.destroyShaderModule(shader_module);
	device.destroyPipeline(pipeline);
	device.destroyDescriptorPool(descriptor_pool);
	device.destroyDescriptorSetLayout(descriptor_set_layout);
	device.freeMemory(buffer_in_memory);
	device.freeMemory(buffer_out_memory);
	device.destroyBuffer(buffer_in);
	device.destroyBuffer(buffer_out);
	device.destroy();
	instance.destroy();
	return true;
}
