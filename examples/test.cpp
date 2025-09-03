#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../src/lily_png.h"
#include <iostream>
#include "stb_write.h"
#include <cstdlib>
#include <chrono>

int main(int argc, char *argv[])
{
    if (argc == 1)
    {
        std::println("Not enough arguments");
        return -1;
    }
    using clock = std::chrono::system_clock;
    using ms = std::chrono::duration<double, std::milli>;
    lily_png::image data{};
    const auto before = clock::now();
    auto ret = lily_png::read_png(argv[1], data);
    const ms duration = clock::now() - before;
    std::println("{}ms", duration.count());
    if (!ret)
    {
        std::println("Failed {}", ret.error());
        return -1;
    }
        auto pixel_size_ret = get_pixel_bit_size(data.meta);
    if (!pixel_size_ret)
        return -1;
    size_t pixel_size = pixel_size_ret.value();
    size_t pixel_size_bytes = (pixel_size + 7)/8;
    int components = pixel_size_bytes;

    int width = data.meta.width;
    int height = data.meta.height;
    
    int stride_in_bytes = data.meta.width * pixel_size_bytes;

    if (!stbi_write_png("data.png", width, height, components, data.buffer.data, stride_in_bytes)) 
    {
        std::cerr << "STB Error: Could not write image to " << "data.png" << std::endl;
    }
    else 
    {
        std::cout << "Image saved as " << "data.png" << std::endl;
    }
}
