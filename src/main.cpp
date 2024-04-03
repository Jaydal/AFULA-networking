#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>

#define FORWARD 'F'
#define BACKWARD 'B'
#define RIGHT 'R'
#define LEFT 'L'
#define STOP 'S'
#define EXTINGUISH 'E'
#define RESET_MOTOR 'X'

bool testMode = false;

// Networking
IPAddress local_IP(192, 168, 1, 101);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 0, 0);

const char *ssid_prod = "arduino_net";
const char *password_prod = "arduino_net";
const char *serverUrl_prod = "http://192.168.1.110";

const char *ssid = "PLDTHOMEFIBRTx4z7";
const char *password = "HindiKoAlamE1Ko!";
const char *serverUrl = "http://jdal.local";

// fire detection
bool fireDetected = false;
bool fireExtinguishing = false;
int noFireOccurenceIR = 0;

// components
int GRN_LED = D8;
int RED_LED = D7;
int FLM_SENSOR = A0;

// flame sensor
const int flameSensorMin = 0;
const int flameSensorMax = 1024;

void initWifi()
{
  if (!WiFi.config(local_IP, gateway, subnet))
  {
    Serial.println("STA Failed to configure");
  }

  delay(500);
  WiFi.begin(ssid_prod, password_prod);
  delay(500);

  Serial.print("Connecting");
  int wifiConnectionCounter = 0;

  while (WiFi.status() != WL_CONNECTED && wifiConnectionCounter <= 30)
  {
    delay(500);
    Serial.print(".");
    wifiConnectionCounter++;
  }
  Serial.println();
  if (WiFi.status() != WL_CONNECTED)
  {
    WiFi.begin(ssid, password);
    Serial.print("Failed to connect to primary WIFI, connecting using fallback credentials.");
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(500);
      Serial.print(".");
    }
  }

  Serial.println();
  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());
}


HTTPClient getHttpClient(String url = serverUrl_prod, String fallbackUrl = serverUrl){
  HTTPClient http;
  WiFiClient client;
  http.begin(client, url);

  int httpCode = http.GET();

  if (httpCode != HTTP_CODE_OK)
  {
    Serial.println("Reconnecting using fallback server...");
    http.end();
    http.begin(client, fallbackUrl);
  }

  return http;
}

void sendLogs(String message){
  HTTPClient http = getHttpClient(String(serverUrl_prod)+"/log",String(serverUrl)+"/log");
  
  int httpCode = http.GET();

  if (httpCode != HTTP_CODE_OK)
  {
    Serial.println("Unable to get logs!");
    return;
  }

  http.addHeader("Content-Type", "application/json");

  String jsonData = "{\"message\": " + message + "}";

  int httpPostCode = http.POST(jsonData);

  if(httpPostCode != HTTP_CODE_OK){
    Serial.println("Unable to post logs!");
  }
}

void sendMotorCommand(char command)
{
  sendLogs("INFO: Sending "+ String(command) +" command to motor driver module.");
  Wire.beginTransmission(9);
  Wire.write(command);
  Wire.endTransmission();
  delay(1);
}

bool validateFirewithIR()
{
  sendLogs("INFO: Detecting fire using Infrared Sensors.");
  bool fireDetected = false;
  int sensorReading = analogRead(FLM_SENSOR);
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

bool predictFireFromImage()
{
  sendLogs("INFO: Starting fire detection using Image Processing Service.");
  Serial.println("Fire Prediction Started...");
  Serial.print("Server: ");
  Serial.println(serverUrl_prod);

  bool firePredicted = false;

  HTTPClient http = getHttpClient();
  int httpCode = http.GET();

  if (httpCode != HTTP_CODE_OK)
  {
    Serial.println("Unable to connect!");
    sendLogs("WARN: Service is not available.");
    sendLogs("WARN: Unable to connect.");
    return false;
  }

  http.addHeader("Content-Type", "application/json");

  String jsonData = "{\"testMode\": " +
                    String(testMode ? "true" : "false") + "}";

  sendLogs("INFO: Sending HTTP Request.");
  int httpResponseCode = http.POST(jsonData);

  if (httpResponseCode > 0)
  {
    String response = http.getString();
    Serial.println("HTTP Response code: " + String(httpResponseCode));
    Serial.println("Response: " + response);

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);

    if (error)
    {
      sendLogs("ERROR: Deserialize failed.");
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return false;
    }

    String nasnet = doc["nasnet"];
    String shufflenet = doc["shufflenet"];

    Serial.println("nasnet model: " + nasnet);
    Serial.println("shufflenet model: " + shufflenet);
    sendLogs("INFO: Fire Detection Results:");
    sendLogs("- - - Nasnet model: " + nasnet);
    sendLogs("- - - Shufflenet model: " + shufflenet);

    firePredicted = (shufflenet != "1.0" || nasnet != "1.0") ? true : false;
  }
  else
  {
    sendLogs("ERROR: Error on HTTP request.");
    Serial.print("Error on HTTP request: ");
    Serial.println(httpResponseCode);
  }

  http.end();
  sendLogs("INFO: Terminating HTTP Request.");

  return firePredicted;
}

void blinkLED(int freq, int ms, int led = GRN_LED)
{
  for (int i = 0; i < freq; i++)
  {
    digitalWrite(led, HIGH);
    delay(ms);
    digitalWrite(led, LOW);
    delay(ms);
  }
}

void sendAlert()
{
  sendLogs("INFO: Sending alerts to local authorities.");
  Serial.println("Alerting Authorities....");
}

void reset()
{
  sendLogs("INFO: Executing Reset.");
  digitalWrite(GRN_LED, HIGH);
  Serial.println("Resetting....");

  sendMotorCommand(RESET_MOTOR);
  fireExtinguishing = false;
  fireDetected = false;
  noFireOccurenceIR = 0;

  delay(3000);
  digitalWrite(GRN_LED, LOW);
  sendLogs("INFO: Reset Completed.");
}

void onFire()
{
  sendLogs("INFO: Fire is confirmed.");
  Serial.println();
  Serial.println("Fire Confirmed!!!");
  sendAlert();
  fireExtinguishing = true;
  sendMotorCommand(EXTINGUISH);
}

void setup()
{
  Wire.begin();

  Serial.begin(9600);
  pinMode(GRN_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  delay(1000);
  Serial.begin(115200);
  Serial.println();

  initWifi();
  sendLogs("INFO: Setup Initiliazed.");
}

void loop()
{
  if (fireDetected && !fireExtinguishing)
  {
    sendLogs("INFO: Image Processing Server detects fire.");
    Serial.println("Fire Warning!!!");
    blinkLED(20, 100);

    Serial.println("Validating");
    sendLogs("INFO: Initializing fire validation using Infrared Sensors.");
    int ctr = 0;
    int resetTimer = 30000;
    int timer = 0;

    do
    {
      timer++;
      ctr += validateFirewithIR() ? 1 : 0;
      delay(1);
      if (timer >= resetTimer)
      {
        Serial.println("Sensor did not detect any flames!");
        sendLogs("INFO: Infrared Sensors did not detect fire.");
        break;
      }
      sendMotorCommand(FORWARD);
    } while (ctr <= 3);

    if (ctr >= 3)
    {
      onFire();
    }
    else
    {
      Serial.println("Fire Not Confirmed!!!");
      sendLogs("INFO: Infrared Sensors were not able to detect fire.");
      blinkLED(10, 500);
      fireDetected = false;
    }
  }
  else if (fireDetected && fireExtinguishing)
  {
    sendLogs("INFO: Fighting Fire.");
    Serial.println("WANG WANG!!");
    blinkLED(1, 100, RED_LED);
    if (!validateFirewithIR())
    {
      sendLogs("INFO: Fire seems out.");
      if (noFireOccurenceIR > 10)
      {
        sendLogs("INFO: Fire is out.");
        reset();
      }
      noFireOccurenceIR++;
    }
  }
  else
  {
    blinkLED(3, 1000);
    Serial.print("Starting fire detection in 5000ms");
    sendLogs("INFO: Starting fire detection.");

    int ctr = 0;
    do
    {
      Serial.print(".");
      if (validateFirewithIR())
      {
        fireDetected = true;
        onFire();
        return;
      }
      ctr++;
      delay(1);
    } while (ctr <= 5000);

    Serial.println("Starting fire detection with AI.....");

    fireDetected = predictFireFromImage();
    Serial.println("Prediction: ");
    Serial.print("Fire exists, ");
    Serial.println(fireDetected ? "true" : "false");
    sendLogs("INFO: Image Processing Result: "+fireDetected ? "FIRE EXISTS" : "NO FIRE");
  }
}
