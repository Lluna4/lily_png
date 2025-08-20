#include <metal_stdlib>
using namespace metal;

kernel void defilter(device uint8_t *dest, device uint8_t *src, device uint8_t *a, device uint8_t *b, device uint8_t *c, device uint8_t *type)
{
    switch (type[0])
    {
        case 0:
            dest[0] = src[0];
            break;
        case 1:
            dest[0] = src[0] + a[0];
            break;
        case 2:
            dest[0] = src[0] + b[0];
            break;
        case 3:
            dest[0] = src[0] + floor((a[0] + b[0])/2.0f);
        case 4:
            uint8_t pred = a[0]+b[0]-c[0];
            uint8_t pred1 = abs(pred-a[0]);
            uint8_t pred2 = abs(pred-b[0]);
            uint8_t pred3 = abs(pred-c[0]);
            if (pred1 <= pred2 && pred1 <= pred3)
                dest[0] = a[0];
            else if (pred2 <= pred3)
                dest[0] = b[0];
            else
                dest[0] = c[0];
    }
}
