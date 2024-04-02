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
#define RESET 'RST'

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

void sendMotorCommand(char command)
{
  Wire.beginTransmission(9);
  Wire.write(command);
  Wire.endTransmission();
  delay(1);
}

bool validateFirewithIR()
{
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
  Serial.println("Fire Prediction Started...");
  Serial.print("Server: ");
  Serial.println(serverUrl_prod);

  bool firePredicted = false;

  HTTPClient http;
  WiFiClient client;
  http.begin(client, serverUrl_prod);

  int httpCode = http.GET();

  if (httpCode != HTTP_CODE_OK)
  {

    Serial.println("Reconnecting using fallback server...");
    http.end();

    http.begin(client, serverUrl);
    httpCode = http.GET();

    if (httpCode != HTTP_CODE_OK)
    {
      http.end();
      Serial.println("Unable to connect!");
      return false;
    }
  }

  http.addHeader("Content-Type", "application/json");

  String jsonData = "{\"testMode\": " +
                    String(testMode ? "true" : "false") + "}";

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

  http.end();

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
  Serial.println("Alerting Authorities....");
}

void reset()
{
  digitalWrite(GRN_LED, HIGH);
  Serial.println("Resetting....");

  sendMotorCommand(RESET);
  fireExtinguishing = false;
  fireDetected = false;
  noFireOccurenceIR = 0;

  delay(3000);
  digitalWrite(GRN_LED, LOW);
}

void onFire()
{
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
}

void loop()
{
  if (fireDetected && !fireExtinguishing)
  {
    Serial.println("Fire Warning!!!");
    blinkLED(20, 100);

    Serial.println("Validating");

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
      blinkLED(10, 500);
      fireDetected = false;
    }
  }
  else if (fireDetected && fireExtinguishing)
  {
    Serial.println("WANG WANG!!");
    blinkLED(1, 100, RED_LED);
    if (!validateFirewithIR())
    {
      if (noFireOccurenceIR > 10)
      {
        reset();
      }
      noFireOccurenceIR++;
    }
  }
  else
  {
    blinkLED(3, 1000);
    Serial.print("Starting fire detection in 5000ms");

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
  }
}
