#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <SoftwareSerial.h>
#include <FireValidator.h>
#include <Pitches.h>
#include <MusicPlayer.h>

#define FORWARD 'F'
#define BACKWARD 'B'
#define RIGHT 'R'
#define LEFT 'L'
#define STOP 'S'
#define EXTINGUISH 'E'
#define RESET_MOTOR 'X'

//methods
#define VALIDATE_FIRE_IR [&]() { return fireValidator.ValidateWithIR(FLM_SENSOR); }()
#define VALIDATE_FIRE_AI [&]() { return fireValidator.ValidateWithAI(getHttpClient(),testMode); }()

//Settings
bool testMode = false;
bool noLogs = true;
bool motorOff = false;
bool manualDriveMode = false;
int commandTimer = 0;
int commandResetTimer = 100;
bool init_done = false;
int setupCounter = 0;
bool musicMode = false;

//GSM
String gsmRecipient =  "+639978037903";
SoftwareSerial gsmSerial(D5, D6);

// Networking
ESP8266WebServer server(80);

IPAddress local_IP(192, 168, 1, 101);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 0, 0);

// const char *ssid_prod = "arduino_net";
// const char *password_prod = "arduino_net";
// const char *serverUrl_prod = "http://192.168.1.110";//change to 110

const char *ssid = "PLDTHOMEFIBRTx4z7";
const char *password = "HindiKoAlamE1Ko!";
const char *serverUrl = "http://192.168.1.216";

const char *ssid_prod = "PLDTHOMEFIBRTx4z7";
const char *password_prod = "HindiKoAlamE1Ko!";
const char *serverUrl_prod = "http://192.168.1.216";

String activeServer;

// fire detection
bool fireDetected = false;
bool fireExtinguishing = false;
int noFireOccurenceIR = 0;

// components
const int BLU_LED = D8;
const int RED_LED = D7;
const int SPK_1 = D2;
const int FLM_SENSOR = A0;


// flame sensor
// const int flameSensorMin = 0;
// const int flameSensorMax = 1024;
FireValidator fireValidator;

void initComponents()
{
  pinMode(BLU_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(SPK_1, OUTPUT);
}

void sendSMS(String message){
  gsmSerial.println("AT+CMGF=1");
  delay(1000);
  gsmSerial.println("AT+CMGS=\""+gsmRecipient+"\"\r"); 
  delay(1000);
  gsmSerial.print(message);
  delay(1000); 
  gsmSerial.println((char)26);
  Serial.print("Sending message: ");
  Serial.println(message);
  delay(1000); 
}

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

  while (WiFi.status() != WL_CONNECTED && wifiConnectionCounter <= 50)
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

  activeServer = url;

  if (httpCode != HTTP_CODE_OK)
  {
    Serial.println("Reconnecting using fallback server...");
    http.end();
    http.begin(client, fallbackUrl);
    activeServer = fallbackUrl;
  }

  return http;
}

void sendLogs(String message)
{
  if(noLogs) return;
  
  HTTPClient http;
  WiFiClient client;
  
  delay(1000);
  
  if (http.begin(client, activeServer + "/logs"))
  {

    http.addHeader("Content-Type", "application/json");

    String jsonData = "{\"message\": \"" + message + "\"}";

    int httpPostCode = http.POST(jsonData);

    if (httpPostCode > 0)
    {
      Serial.println("Unable to send logs.");
      // Serial.println(httpPostCode);
      String response = http.getString();
      // Serial.println("Server response: " + response);
    }
    else
    {
      Serial.println("Error sending HTTP POST request.");
    }

    http.end();
  }
  else
  {
    Serial.println("Unable to connect to server.");
  }
}

void sendMotorCommand(char command)
{
  if(motorOff){
    return;
  }
  Serial.print("Motor Command: ");
  Serial.println(command);
  commandTimer = 0;
  Wire.beginTransmission(9);
  Wire.write(command);
  Wire.endTransmission();
  delay(1);
  sendLogs("INFO: Sending "+ String(command) +" command to motor driver module.");
}

void handleCommand()
{
  String command = server.arg("command");
  
  manualDriveMode = (command == "manualmode_on") ? true : 
                    (command == "manualmode_off") ? false : manualDriveMode;

  testMode = (command == "testmode_on") ? true : 
                  (command == "testmode_off") ? false : testMode;
  
  motorOff = (command == "motor_off") ? true : 
                (command == "motor_on") ? false : motorOff;

  musicMode = (command == "musicmode_off") ? false : 
              (command == "musicmode_on") ? true : musicMode;

  if(manualDriveMode){
    if (command == "forward")
    {
      sendMotorCommand(FORWARD);
    }
    else if (command == "backward")
    {
      sendMotorCommand(BACKWARD);
    }
    else if (command == "left")
    {
      sendMotorCommand(LEFT);
    }
    else if (command == "right")
    {
      sendMotorCommand(RIGHT);
    }
    else if (command == "stop")
    {
      sendMotorCommand(STOP);
    }
    else if (command == "extingush")
    {
      sendMotorCommand(EXTINGUISH);
    }
    else if (command == "reset")
    {
      sendMotorCommand(RESET_MOTOR);
    }
  }
  server.send(200, "text/plain", "Command received: " + command);
}

void beep(int spk = SPK_1){
   for( int i = 0; i<500;i++){
      digitalWrite(spk , HIGH);
      delayMicroseconds(500);
      digitalWrite(spk, LOW );
      delayMicroseconds(500);
   }
}

void blinkLED(int freq, int ms, int led = BLU_LED, int led2 = LED_BUILTIN)
{
  for (int i = 0; i < freq; i++)
  {
    digitalWrite(led, HIGH);
    digitalWrite(led2, HIGH);
    beep();
    delay(ms);
    digitalWrite(led, LOW);
    digitalWrite(led2, LOW);
    delay(ms);
  }
}

void sendAlert()
{
  sendLogs("INFO: Sending alerts to local authorities.");
  Serial.println("Alerting Authorities....");
  sendSMS("AFULA-bot has detected a fire! Please respond to its location.");
}

void reset()
{
  sendSMS("Fire appears to be out! AFULA-bot has stopped responding to the fire.");
  sendLogs("INFO: Executing Reset.");
  digitalWrite(BLU_LED, HIGH);
  Serial.println("Resetting....");

  sendMotorCommand(RESET_MOTOR);
  fireExtinguishing = false;
  fireDetected = false;
  noFireOccurenceIR = 0;

  delay(3000);
  digitalWrite(BLU_LED, LOW);
  sendLogs("INFO: Reset Completed.");
}

void onFire()
{
  fireDetected = true;
  sendLogs("INFO: Fire is confirmed.");
  Serial.println();
  Serial.println("Fire Confirmed!!!");
  sendAlert();
  fireExtinguishing = true;
  sendMotorCommand(EXTINGUISH);
}

void setup()
{
  gsmSerial.begin(9600);
  Serial.begin(115200);
  Wire.begin();

  Serial.println();
  
  delay(1000);
  initComponents();
  initWifi();
  delay(1000);

  server.on("/command", HTTP_POST, handleCommand);
  server.begin();
  Serial.println();
}

MusicPlayer music;

void loop()
{
  // sendMotorCommand(FORWARD);
  // sendMotorCommand(EXTINGUISH);
  // delay(4000);
  // sendMotorCommand(RESET_MOTOR);
  // delay(1000);
  // return;
  if(!init_done){
    digitalWrite(BLU_LED,HIGH);
    Serial.print("Initializing (");
    Serial.print(setupCounter);
    Serial.println(")");  
    server.handleClient();
    delay(1);
    setupCounter++;
    
    if(setupCounter<=5000){
      return;
    }

    digitalWrite(BLU_LED,LOW);
    sendLogs("INFO: Setup Initiliazed.");

    if(testMode){
      sendLogs("INFO: Test mode is on!");
      Serial.println("Test mode is on!");

    }
    init_done = true;
  }
  else{
    server.handleClient();
  }

  if (musicMode)
  {
    music.Play();
    delay(2000);
    return;
  }

  if(manualDriveMode){
    commandTimer++;
    Serial.println("Manual mode is on!");
    digitalWrite(RED_LED,HIGH);
    digitalWrite(BLU_LED,HIGH);

    if(commandTimer>=commandResetTimer){
        sendMotorCommand(STOP);
    }
    return;
  }
  else{
    digitalWrite(RED_LED,LOW);
    digitalWrite(BLU_LED,LOW);
  }

  if (fireDetected && !fireExtinguishing)
  {
    sendLogs("INFO: Image Processing Server detects fire.");
    Serial.println("Fire Warning!!!");
    blinkLED(2, 500 ,BLU_LED,RED_LED);

    Serial.println("Validating");
    sendLogs("INFO: Initializing fire validation using Infrared Sensors.");
    int ctr = 0;
    int resetTimer = 20000;
    int timer = 0;

    digitalWrite(RED_LED,HIGH);
    digitalWrite(BLU_LED,HIGH);

    do
    {
      if(timer%500==0 || timer==0){
        sendMotorCommand(FORWARD);
      }

      timer++;
      ctr += VALIDATE_FIRE_IR ? 1 : 0;
      delay(1);
      if (timer >= resetTimer)
      {
        Serial.println("Sensor did not detect any flames!");
        sendLogs("INFO: Infrared Sensors did not detect fire.");
        break;
      }
    } while (ctr <= 3);

    sendMotorCommand(STOP);

    if (ctr >= 3)
    {
      onFire();
    }
    else
    {
      digitalWrite(RED_LED,LOW);
      digitalWrite(BLU_LED,LOW);
      Serial.println("Fire Not Confirmed!!!");
      sendLogs("INFO: Infrared Sensors were not able to detect fire.");
      blinkLED(5, 100,BLU_LED);
      fireDetected = false;
    }
  }
  else if (fireDetected && fireExtinguishing)
  {
    sendLogs("INFO: Fighting Fire.");
    blinkLED(1, 100,RED_LED,BLU_LED);
    if (!VALIDATE_FIRE_IR)
    {
      Serial.println("Fire seems out");
      sendLogs("INFO: Fire seems out.");
      if (noFireOccurenceIR > 20)
      {
        sendLogs("INFO: Fire is out.");
        Serial.println("Fire is out!");
        blinkLED(1, 2000,BLU_LED);
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
      if (VALIDATE_FIRE_IR)
      {
        onFire();
        return;
      }
      ctr++;
      delay(100);
    } while (ctr <= 50);

    Serial.println("Starting fire detection with AI.....");
    fireDetected = VALIDATE_FIRE_AI;
    Serial.println("Prediction: ");
    Serial.print("Fire exists, ");
    Serial.println(fireDetected ? "true" : "false");
    String result = fireDetected ? "Fire Detected.":"No Fire Detected.";
    String msg = "INFO: Image Processing Result: ";
    msg+=result;
    sendLogs(msg);
  }
}
