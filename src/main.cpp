#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <base64.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

bool testMode = false;

// Networking
const char *ssid = "PLDTHOMEFIBRTx4z7";
const char *password = "HindiKoAlamE1Ko!";
const char *serverUrl = "http://192.168.1.216:3000";

// fire detection
bool fireDetected = false;

// components
int LED = D8;

// flame sensor
const int flameSensorMin = 0;
const int flameSensorMax = 1024;

bool validateFire()
{
  bool fireDetected = false;
  int sensorReading = analogRead(A0);
  int range = map(sensorReading, flameSensorMin, flameSensorMax, 0, 3);

  switch (range)
  {
  case 0:
    fireDetected = true;
    break;
  case 1:
    fireDetected = true;
    break;
  }
  return fireDetected;

}

String getImageBase64()
{
  File imageFile = LittleFS.open("/blank.png", "r");
  if (!imageFile)
  {
    Serial.println("Failed to open file for reading");
    return "";
  }

  // Get the file size
  size_t fileSize = imageFile.size();

  // Allocate a buffer to store the image
  uint8_t *imageData = (uint8_t *)malloc(fileSize);
  if (!imageData)
  {
    Serial.println("Failed to allocate memory");
    imageFile.close();
    return "";
  }

  // Read the image data into the buffer
  size_t bytesRead = imageFile.read(imageData, fileSize);
  if (bytesRead != fileSize)
  {
    Serial.println("Failed to read image data");
    imageFile.close();
    free(imageData);
    return "";
  }

  // Close the file
  imageFile.close();

  // Encode the image data to Base64
  String base64Image = base64::encode(imageData, fileSize);

  // Free the allocated memory
  free(imageData);

  return base64Image;
}

bool predictFireFromImage()
{
  Serial.println("Fire Prediction Started...");
  Serial.print("Server: ");
  Serial.println(serverUrl);

  bool firePredicted = false;

  HTTPClient http;
  WiFiClient client;
  http.begin(client, serverUrl);
  http.addHeader("Content-Type", "application/json");

  // Create JSON object with Base64 image data
  String jsonData = "{\"testMode\": " +
                    String(testMode ? "true" : "false")+"}";

  // Send JSON data as the request body
  int httpResponseCode = http.POST(jsonData);

  // Check for response
  if (httpResponseCode > 0)
  {
    String response = http.getString();
    Serial.println("HTTP Response code: " + String(httpResponseCode));
    Serial.println("Response: " + response);

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);

    // Test if parsing succeeds.
    if (error)
    {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return false;
    }

    String nasnet = doc["nasnet"];
    String shufflenet = doc["shufflenet"];

    Serial.println("nasnet model: " + nasnet);
    Serial.println("shufflenet model: " + shufflenet);

    firePredicted = (shufflenet != "1.0" || nasnet != "1.0") ? true : false;
  }
  else
  {
    Serial.print("Error on HTTP request: ");
    Serial.println(httpResponseCode);
  }

  // Disconnect
  http.end();

  return firePredicted;
}

void blinkLED(int freq, int ms)
{
  for (int i = 0; i < freq; i++)
  {
    digitalWrite(LED, HIGH);
    delay(ms);
    digitalWrite(LED, LOW);
    delay(ms);
  }
}

void setup()
{
  Serial.begin(9600);
  pinMode(LED, OUTPUT);
  delay(1000);
  Serial.begin(115200);
  Serial.println();

  delay(500);
  WiFi.begin(ssid, password);
  delay(500);

  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());

  if (!LittleFS.begin())
  {
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }
}

void loop()
{

  if (fireDetected)
  {
    Serial.println("Fire Warning!!!");
    blinkLED(20, 100);

    //DRIVE MODULE

    //Start flame detection
    Serial.println("Validating");

    int ctr = 0;
    int resetTimer = 100000;
    int timer = 0;

    do {
      timer++;
      ctr+=validateFire()?1:0;
      delay(1);
      if(timer>=resetTimer){
        Serial.println("Sensor did not detect any flames!");
        break;
      }
    }
    while (ctr<=3);

    if(ctr>=3){
        //FIRE VALIDATED
        //gsm module
        // trigger extiguisher
        Serial.println("Fire Confirmed!!!");
        blinkLED(100, 200);
    }
    else{
        Serial.println("Fire Not Confirmed!!!");
        blinkLED(10, 500);
    }

    fireDetected=false;//set to false to reset
  }
  else
  {
    Serial.println("Starting fire detection in 5000ms");
    delay(5000);
    Serial.println("Starting fire detection.....");
    blinkLED(3, 2000);
    fireDetected = predictFireFromImage();
    Serial.println("Prediction: ");
    Serial.print("Fire exists, ");
    Serial.println(fireDetected ? "true" : "false");
  }
}
