#include "FireValidator.h"
#include <Arduino.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

const int flameSensorMin = 0;
const int flameSensorMax = 1024;

bool FireValidator::ValidateWithIR(int sensor, int min, int max)
{
    int sensorReading = analogRead(sensor);
    int range = map(sensorReading, min, max, 0, 3);
    switch (range)
    {
    case 0:
        return true;
        break;
    case 1:
        return true;
        break;
    }
    return false;
}

bool FireValidator::ValidateWithAI(HTTPClient client, bool testMode)
{
    bool firePredicted = false;

    client.addHeader("Content-Type", "application/json");

    String jsonData = "{\"testMode\": " +
                      String(testMode ? "true" : "false") + "}";

    //   sendLogs("INFO: Sending HTTP Request.");
    Serial.println("Sending request to Image Processing Server.");
    int httpResponseCode = client.POST(jsonData);

    if (httpResponseCode > 0)
    {
        String response = client.getString();
        Serial.println("HTTP Response code: " + String(httpResponseCode));
        Serial.println("Response: " + response);

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, response);

        if (error)
        {
            //   sendLogs("ERROR: Deserialize failed.");
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.f_str());
            return false;
        }

        String nasnet = doc["nasnet"];
        String shufflenet = doc["shufflenet"];

        Serial.println("nasnet model: " + nasnet);
        Serial.println("shufflenet model: " + shufflenet);
        // sendLogs("- - - Nasnet model: " + nasnet);
        // sendLogs("- - - Shufflenet model: " + shufflenet);
        // sendLogs("INFO: Fire Detection Results:");

        firePredicted = (shufflenet != "1.0" || nasnet != "1.0") ? true : false;
    }
    else
    {
        // sendLogs("ERROR: Error on HTTP request.");
        Serial.print("Error on HTTP request: ");
        Serial.println(httpResponseCode);
        firePredicted = false;
    }

    client.end();
    // sendLogs("INFO: Terminating HTTP Request.");

    return firePredicted;
}
