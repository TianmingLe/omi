#include "camera_capture.h"

static inline uint8_t rgb565_to_gray_u8(uint16_t p)
{
    const uint32_t r5 = (uint32_t) ((p >> 11) & 0x1F);
    const uint32_t g6 = (uint32_t) ((p >> 5) & 0x3F);
    const uint32_t b5 = (uint32_t) (p & 0x1F);

    const uint32_t r8 = (r5 * 255U) / 31U;
    const uint32_t g8 = (g6 * 255U) / 63U;
    const uint32_t b8 = (b5 * 255U) / 31U;

    const uint32_t y = (r8 * 77U + g8 * 150U + b8 * 29U) >> 8;
    return (uint8_t) y;
}

void preprocess_rgb565_to_96x96_int8(const uint16_t *rgb565, int src_w, int src_h, int8_t *out_96x96)
{
    if (rgb565 == nullptr || out_96x96 == nullptr) {
        return;
    }
    if (src_w <= 1 || src_h <= 1) {
        for (int i = 0; i < 96 * 96; i++) {
            out_96x96[i] = 0;
        }
        return;
    }

    const uint32_t x_scale_q16 = (uint32_t) (((uint64_t) (src_w - 1) << 16) / (uint64_t) (96 - 1));
    const uint32_t y_scale_q16 = (uint32_t) (((uint64_t) (src_h - 1) << 16) / (uint64_t) (96 - 1));

    for (int dy = 0; dy < 96; dy++) {
        const uint32_t sy_q16 = (uint32_t) dy * y_scale_q16;
        const int y0 = (int) (sy_q16 >> 16);
        const int y1 = (y0 + 1 < src_h) ? (y0 + 1) : y0;
        const uint32_t wy = sy_q16 & 0xFFFFU;

        for (int dx = 0; dx < 96; dx++) {
            const uint32_t sx_q16 = (uint32_t) dx * x_scale_q16;
            const int x0 = (int) (sx_q16 >> 16);
            const int x1 = (x0 + 1 < src_w) ? (x0 + 1) : x0;
            const uint32_t wx = sx_q16 & 0xFFFFU;

            const uint8_t g00 = rgb565_to_gray_u8(rgb565[y0 * src_w + x0]);
            const uint8_t g01 = rgb565_to_gray_u8(rgb565[y0 * src_w + x1]);
            const uint8_t g10 = rgb565_to_gray_u8(rgb565[y1 * src_w + x0]);
            const uint8_t g11 = rgb565_to_gray_u8(rgb565[y1 * src_w + x1]);

            const uint32_t top = ((uint32_t) g00 * (65536U - wx) + (uint32_t) g01 * wx) >> 16;
            const uint32_t bot = ((uint32_t) g10 * (65536U - wx) + (uint32_t) g11 * wx) >> 16;
            const uint32_t g = (top * (65536U - wy) + bot * wy) >> 16;

            out_96x96[dy * 96 + dx] = (int8_t) ((int) g - 128);
        }
    }
}

