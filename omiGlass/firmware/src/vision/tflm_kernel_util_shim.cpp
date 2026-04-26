#include <math.h>
#include <stdint.h>

#include "tensorflow/lite/kernels/kernel_util.h"

namespace tflite {

TfLiteStatus GetQuantizedConvolutionMultipler(TfLiteContext *, const TfLiteTensor *input, const TfLiteTensor *filter, const TfLiteTensor *,
                                              TfLiteTensor *output, double *multiplier)
{
    if (input == nullptr || filter == nullptr || output == nullptr || multiplier == nullptr) {
        return kTfLiteError;
    }
    *multiplier = ((double) input->params.scale * (double) filter->params.scale) / (double) output->params.scale;
    return kTfLiteOk;
}

TfLiteStatus GetQuantizedConvolutionMultipler(TfLiteContext *context, const TfLiteTensor *input, const TfLiteTensor *filter, TfLiteTensor *output,
                                              double *multiplier)
{
    return GetQuantizedConvolutionMultipler(context, input, filter, nullptr, output, multiplier);
}

static inline int32_t clamp_i32(int32_t v, int32_t lo, int32_t hi)
{
    if (v < lo) {
        return lo;
    }
    if (v > hi) {
        return hi;
    }
    return v;
}

static TfLiteStatus activation_range_int8(TfLiteFusedActivation activation, TfLiteTensor *output, int32_t *act_min, int32_t *act_max)
{
    const int32_t qmin = -128;
    const int32_t qmax = 127;
    const int32_t zp = (int32_t) output->params.zero_point;
    const double scale = (double) output->params.scale;

    if (activation == kTfLiteActRelu) {
        const int32_t q0 = (int32_t) llround(0.0 / scale) + zp;
        *act_min = clamp_i32(q0, qmin, qmax);
        *act_max = qmax;
        return kTfLiteOk;
    }
    if (activation == kTfLiteActRelu6) {
        const int32_t q0 = (int32_t) llround(0.0 / scale) + zp;
        const int32_t q6 = (int32_t) llround(6.0 / scale) + zp;
        *act_min = clamp_i32(q0, qmin, qmax);
        *act_max = clamp_i32(q6, qmin, qmax);
        return kTfLiteOk;
    }
    if (activation == kTfLiteActReluN1To1) {
        const int32_t qn1 = (int32_t) llround(-1.0 / scale) + zp;
        const int32_t q1 = (int32_t) llround(1.0 / scale) + zp;
        *act_min = clamp_i32(qn1, qmin, qmax);
        *act_max = clamp_i32(q1, qmin, qmax);
        return kTfLiteOk;
    }

    *act_min = qmin;
    *act_max = qmax;
    return kTfLiteOk;
}

TfLiteStatus CalculateActivationRangeQuantized(TfLiteContext *, TfLiteFusedActivation activation, TfLiteTensor *output, int32_t *act_min,
                                               int32_t *act_max)
{
    if (output == nullptr || act_min == nullptr || act_max == nullptr) {
        return kTfLiteError;
    }
    if (output->type == kTfLiteInt8) {
        return activation_range_int8(activation, output, act_min, act_max);
    }
    return kTfLiteError;
}

} // namespace tflite

