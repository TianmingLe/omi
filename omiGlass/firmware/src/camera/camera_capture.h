#pragma once

#include <stdint.h>

void preprocess_rgb565_to_96x96_int8(const uint16_t *rgb565, int src_w, int src_h, int8_t *out_96x96);

