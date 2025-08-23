#include <metal_stdlib>
using namespace metal;

struct parameters
{
    int32_t y_start;
    int32_t x_start;
    int32_t scanline_size;
    int32_t bytes_per_pixel;
};

kernel void defilter(device uint8_t *img_data [[buffer(0)]], device uint8_t *dest [[buffer(1)]],
    device parameters *p [[buffer(2)]]
    ,uint32_t index [[thread_position_in_grid]])
{
    const int stride = p->scanline_size + 1;
    int32_t y = p->y_start - index;
    int32_t x = p->x_start + index;

    uint8_t a = 0;
    if (x >= p->bytes_per_pixel)
        a = dest[(y * p->scanline_size) + (x - p->bytes_per_pixel)];

    uint8_t b = 0;
    if (y > 0)
        b = dest[((y - 1) * p->scanline_size) + x];

    uint8_t c = 0;

    if (x >= p->bytes_per_pixel && y > 0)
        c = dest[((y - 1) * p->scanline_size) + (x - p->bytes_per_pixel)];

    uint8_t type = img_data[y * stride];

    switch (type)
    {
        case 0:
            dest[(y * p->scanline_size) + x] = img_data[(y * stride) + x + 1];
            break;
        case 1:
            dest[(y * p->scanline_size) + x] = img_data[(y * stride) + x + 1] + a;
            break;
        case 2:
            dest[(y * p->scanline_size) + x] = img_data[(y * stride) + x + 1] + b;
            break;
        case 3:

            dest[(y * p->scanline_size) + x] = img_data[(y * stride) + x + 1] + ((a + b)/2);
            break;
        case 4:
            int32_t pred = a+b-c;
            int32_t pred1 = abs(pred-a);
            int32_t pred2 = abs(pred-b);
            int32_t pred3 = abs(pred-c);
            if (pred1 <= pred2 && pred1 <= pred3)
                dest[(y * p->scanline_size) + x] = img_data[(y * stride) + x + 1] + a;
            else if (pred2 <= pred3)
                dest[(y * p->scanline_size) + x] = img_data[(y * stride) + x + 1] + b;
            else
                dest[(y * p->scanline_size) + x] = img_data[(y * stride) + x + 1] + c;
            break;
    }
}
