#include "main.h"
#include "./kernels/register.h"
#include "./model.h"
#include "./op_resolver.h"
#include "./string_util.h"

int tfliteAPI::init(const char * tflitePath) {
  mModelPath = tflitePath;
  return 0;
}

int tfliteAPI::run(float * inputValue, int inputCount, float * outputValue, int outputCount, bool isDebugLog)
{
  
  std::unique_ptr<tflite::FlatBufferModel> model(
      tflite::FlatBufferModel::BuildFromFile(mModelPath));

  if (!model) {
    return -1;
  }
  
  model->error_reporter();
  
#ifdef TFLITE_CUSTOM_OPS_HEADER
  tflite::MutableOpResolver resolver;
  RegisterSelectedOps(&resolver);
#else
  tflite::ops::builtin::BuiltinOpResolver resolver;
#endif

  std::unique_ptr<tflite::Interpreter> interpreter;
  tflite::InterpreterBuilder(*model, resolver)(&interpreter);
  if (!interpreter) {
    return -2;
  }

  int num_threads = 1;
  if (num_threads != -1) {
    interpreter->SetNumThreads(num_threads);
  }

  if (interpreter->AllocateTensors() != kTfLiteOk) {
    return -3;
  }
  
  float* input = interpreter->typed_input_tensor<float>(0);
  
  // Fill `input`.
  memcpy(input, inputValue, sizeof(float) * inputCount);
  /*
    val = *(inputValue + n);
    *(input + n) = val;
  */

  if(isDebugLog) {
    for(int n=0; n< inputCount; n++){  
      printf("%d-%f\n", n, *(input + n));
    }
  }

  if(interpreter->Invoke() != kTfLiteOk) {
    printf("Invoke error! \n");
    return -4;
  } else if(isDebugLog) printf("Invoked \n");
  
  float * out = interpreter->typed_output_tensor<float>(0);
  memcpy(outputValue, out, sizeof(float) * outputCount);

  if(isDebugLog) {
    for(int n=0; n < outputCount; n++) {
      //*(outputValue + n) = *(out + n);
        printf("%d-%f (%f)\n", n, *(outputValue + n), *(out + n));
    }
  }
  
  return 0;
}