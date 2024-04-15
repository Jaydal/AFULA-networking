#include <ESP8266HTTPClient.h>
#ifndef FireValidator_h
#define FireValidator_h

class FireValidator{
    public:
        bool ValidateWithIR(int sensor, int min = 0, int max = 1024);
        bool ValidateWithAI(HTTPClient client, bool testMode = false);
};

#endif