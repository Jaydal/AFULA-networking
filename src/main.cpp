#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <base64.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

const char *ssid = "PLDTHOMEFIBRTx4z7";
const char *password = "HindiKoAlamE1Ko!";
const char *serverUrl = "http://192.168.1.216:3000";
bool fireDetected = false;
int LED = D8;
bool testMode = true;

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
  String base64data = getImageBase64();

  if (base64data == "")
  {
    return false;
  }

  // Set up HTTP client
  HTTPClient http;
  WiFiClient client;
  http.begin(client, serverUrl);
  http.addHeader("Content-Type", "application/json");

  // Create JSON object with Base64 image data
  String jsonData = "{\"testMode\": " + 
    String(testMode ? "true" : "false") + ", \"image\": \"" + 
      base64data + "\"}";

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
  pinMode(LED,OUTPUT);
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
  blinkLED(3,2000);
  fireDetected= predictFireFromImage();
  Serial.println("Prediction: ");
  Serial.print("Fire exists, ");
  Serial.println( fireDetected ? "true" : "false");

  if(fireDetected){
    blinkLED(20,100);
    //try get distance
  }

  //delay for 30secs before initiating new request
  delay(30000);
}


