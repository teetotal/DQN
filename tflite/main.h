/*
class tfliteAPI {
public:
    tfliteAPI() {};
    virtual ~tfliteAPI() {};
    void init(const char * tflitePath);
    int run(float * inputValue, int inputCount, float * outputValue, int outputCount, bool isDebugLog);
private:
    const char * mModelPath;
    void * mTfliteAPI;
};
*/
class main {
public:
    main();
    virtual ~main() {};
    void init(const char * tflitePath);
    int run(float * inputValue, int inputCount, float * outputValue, int outputCount, bool isDebugLog);
private:
    void * mTfliteAPI;
};

