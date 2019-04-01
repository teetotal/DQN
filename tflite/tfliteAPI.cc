#include "tfliteAPI.h"

int tfliteAPI::init(const char * tflitePath) {
  mModelPath = tflitePath;
  
  std::unique_ptr<tflite::Allocation> allocation(new tflite::FileCopyAllocation(mModelPath, tflite::DefaultErrorReporter()));
  model.reset(new tflite::FlatBufferModel(std::move(allocation), tflite::DefaultErrorReporter()));
  
  if (!model) {
    //printf("%s", model->error_reporter()->to_string().c_str();
    return -1;
  }
  
  return 0;
}

int tfliteAPI::run(float * inputValue, int inputCount, float * outputValue, int outputCount, bool isDebugLog)
{
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