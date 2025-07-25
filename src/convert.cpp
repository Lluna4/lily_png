#include "convert.h"

std::expected<bool, lily_png::CONVERT_ERROR> lily_png::convert_to_R32G32B32A32(file_reader::buffer<unsigned char> &data, file_reader::buffer<unsigned char> &dest, metadata &meta)
{
    if (meta.color_type != static_cast<char>(color::rgba))
        return std::unexpected(CONVERT_ERROR::color_type_mismatch);
    if (!(meta.bit_depth == 8 || meta.bit_depth == 16))
        return std::unexpected(CONVERT_ERROR::non_standard_bit_depth);

    size_t size = get_uncompressed_size(meta);
    size_t dest_size = 0;
    if (meta.bit_depth == 8)
    {
        dest_size = size * 4;
        std::tuple<unsigned char, unsigned char, unsigned char, unsigned char> pixel;
        constexpr std::size_t size_tup = std::tuple_size_v<decltype(pixel)>;
        dest.allocate(dest_size);
        size_t dest_index = 0;
        for (int i = 0; i < size; i += 4)
        {
            file_reader::parsing_buffer par_buf(data);
            par_buf.point = &par_buf.buf.data[i];
            par_buf.consumed_size = 0;
            read_comp(size_tup, par_buf, pixel);
            color_rgba32 tmp{0};
            tmp.r = std::get<0>(pixel);
            tmp.g = std::get<1>(pixel);
            tmp.b = std::get<2>(pixel);
            tmp.a = std::get<3>(pixel);
            memcpy(&dest.data[dest_index], &tmp.r, sizeof(unsigned int));
            dest_index += sizeof(unsigned int);
            memcpy(&dest.data[dest_index], &tmp.g, sizeof(unsigned int));
            dest_index += sizeof(unsigned int);
            memcpy(&dest.data[dest_index], &tmp.b, sizeof(unsigned int));
            dest_index += sizeof(unsigned int);
            memcpy(&dest.data[dest_index], &tmp.a, sizeof(unsigned int));
            dest_index += sizeof(unsigned int);
        }
    }

    else if (meta.bit_depth == 16)
    {
        dest_size = size * 2;
        std::tuple<unsigned short, unsigned short, unsigned short, unsigned short> pixel;
        constexpr std::size_t size_tup = std::tuple_size_v<decltype(pixel)>;
        dest.allocate(dest_size);
        size_t dest_index = 0;
        for (int i = 0; i < size; i += 4 * sizeof(unsigned short))
        {
            file_reader::parsing_buffer par_buf(data);
            par_buf.point = &par_buf.buf.data[i];
            par_buf.consumed_size = 0;
            read_comp(size_tup, par_buf, pixel);
            color_rgba32 tmp{0};
            tmp.r = std::get<0>(pixel);
            tmp.g = std::get<1>(pixel);
            tmp.b = std::get<2>(pixel);
            tmp.a = std::get<3>(pixel);
            memcpy(&dest.data[dest_index], &tmp.r, sizeof(unsigned int));
            dest_index += sizeof(unsigned int);
            memcpy(&dest.data[dest_index], &tmp.g, sizeof(unsigned int));
            dest_index += sizeof(unsigned int);
            memcpy(&dest.data[dest_index], &tmp.b, sizeof(unsigned int));
            dest_index += sizeof(unsigned int);
            memcpy(&dest.data[dest_index], &tmp.a, sizeof(unsigned int));
            dest_index += sizeof(unsigned int);
        }
    }

    return true;
}
