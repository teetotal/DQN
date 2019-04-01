#include "tensorflow/lite/kernels/register.h"
#include "tensorflow/lite/model.h"
#include "tensorflow/lite/op_resolver.h"
#include "tensorflow/lite/string_util.h"

class tfliteAPI {
public:
    tfliteAPI() {};
    virtual ~tfliteAPI() {};
    int init(const char * tflitePath);
    int run(float * inputValue, int inputCount, float * outputValue, int outputCount, bool isDebugLog);
private:
    const char * mModelPath;

    std::unique_ptr<tflite::FlatBufferModel> model;
    #ifdef TFLITE_CUSTOM_OPS_HEADER
    tflite::MutableOpResolver resolver;
    RegisterSelectedOps(&resolver);
    #else
    tflite::ops::builtin::BuiltinOpResolver resolver;
    #endif
    
    //tflite::Interpreter * interpreter;
};