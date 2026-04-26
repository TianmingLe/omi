#pragma once

#include <stddef.h>
#include <stdint.h>

void feature_extract_tflm_init();
bool feature_extract_tflm_run(const int8_t *input_96x96, int8_t *out_128, size_t out_len);

