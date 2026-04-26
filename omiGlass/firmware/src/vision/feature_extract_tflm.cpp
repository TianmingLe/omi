#include "feature_extract_tflm.h"

#include <string.h>

#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_op_resolver.h"
#include "tensorflow/lite/micro/kernels/fully_connected.h"

#include "vision/model_data.h"

static uint8_t s_tflm_arena[30720];
static const tflite::Model *s_model = nullptr;

alignas(tflite::MicroInterpreter) static uint8_t s_interpreter_buf[sizeof(tflite::MicroInterpreter)];
static tflite::MicroInterpreter *s_interpreter = nullptr;

class FeatureOpResolver final : public tflite::MicroOpResolver {
public:
    const TFLMRegistration *FindOp(tflite::BuiltinOperator op) const override
    {
        if (op == tflite::BuiltinOperator_FULLY_CONNECTED) {
            static TFLMRegistration reg = tflite::Register_FULLY_CONNECTED();
            return &reg;
        }
        return nullptr;
    }

    const TFLMRegistration *FindOp(const char *) const override
    {
        return nullptr;
    }

    tflite::TfLiteBridgeBuiltinParseFunction GetOpDataParser(tflite::BuiltinOperator) const override
    {
        return nullptr;
    }
};

static FeatureOpResolver s_resolver;

void feature_extract_tflm_init()
{
    if (s_interpreter != nullptr) {
        return;
    }

    s_model = tflite::GetModel(g_feature_model);

    s_interpreter =
        new (s_interpreter_buf) tflite::MicroInterpreter(s_model, s_resolver, s_tflm_arena, sizeof(s_tflm_arena));
    s_interpreter->AllocateTensors();
}

bool feature_extract_tflm_run(const int8_t *input_96x96, int8_t *out_128, size_t out_len)
{
    feature_extract_tflm_init();
    if (s_interpreter == nullptr) {
        return false;
    }

    if (out_128 == nullptr || out_len < 128) {
        return false;
    }

    memset(out_128, 0, 128);

    TfLiteTensor *input = s_interpreter->input(0);
    if (input == nullptr || input->type != kTfLiteInt8 || input->data.int8 == nullptr) {
        return false;
    }

    const size_t n = (input_96x96 == nullptr) ? 0 : (size_t) input->bytes;
    if (n > 0) {
        memcpy(input->data.int8, input_96x96, n);
    }

    if (s_interpreter->Invoke() != kTfLiteOk) {
        return false;
    }

    TfLiteTensor *output = s_interpreter->output(0);
    if (output == nullptr || output->type != kTfLiteInt8 || output->data.int8 == nullptr) {
        return false;
    }

    const size_t copy_n = (output->bytes < 128) ? (size_t) output->bytes : (size_t) 128;
    memcpy(out_128, output->data.int8, copy_n);
    return true;
}
