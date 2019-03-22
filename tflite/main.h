#include "tensorflow/lite/interpreter.h"

class tfliteAPI {
public:
    tfliteAPI() {};
    virtual ~tfliteAPI() {};
    int init(const char * tflitePath);
    int run(float * inputValue, int inputCount, float * outputValue, int outputCount, bool isDebugLog);
private:
    const char * mModelPath;
};


