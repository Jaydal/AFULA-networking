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
// #include <WifiNetwork.h>

#include <Constants.h>

//methods
// #define VALIDATE_FIRE_IR [&]() { return fireValidator.ValidateWithIR(FLM_SENSOR); }()
#define VALIDATE_FIRE_AI [&]() { return fireValidator.ValidateWithAI(getHttpClient(),testMode); }()
// #define CONFIG_WIFI_LOCAL []() { wifiNetwork.Config(local_IP,gateway,subnet);}()
// #define CONNECT_WIFI []() { wifiNetwork.Connect(SSID,PASSWORD,SSID_FALLBACK,PASSWORD_FALLBACK);}()

//dynamic vars
bool init_done = false;
bool fireDetected = false;
bool IRValidation = false;
bool fireExtinguishing = false;
int noFireOccurenceIR = 0;
int fireOccurenceIR = 0;
String activeServer;

//settings
bool testMode = TEST_MODE;
bool noLogs = NO_LOGS;
bool motorOff = MOTOR_OFF;
bool manualDriveMode = MANUAL_DRIVE_MODE;
int commandTimer = COMMAND_TIMER;
int commandResetTimer = COMMAND_RESET_TIMER;
int setupCounter = SETUP_COUNTER;
bool musicMode = MUSIC_MODE;

//networking
SoftwareSerial gsmSerial(GSM_IO_1, GSM_IO_2);
// WifiNetwork wifiNetwork;
ESP8266WebServer server(80);

IPAddress local_IP(192, 168, 1, 101);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 0, 0);

//components
const int BLU_LED = BLINKER_BLUE_LED;
const int RED_LED = LED_BUILTIN;
// const int SPK_1 = SIREN_SPEAKER;
const int FLM_SENSOR = FLAME_SENSOR;

//fire validation
FireValidator fireValidator;

//extras
MusicPlayer music;

void initComponents()
{
  pinMode(BLU_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);
  // pinMode(SPK_1, OUTPUT);
}

void sendSMS(String message){
  gsmSerial.println(SMS_MODE);
  delay(1000);
  gsmSerial.println(SMS_RECIPIENT_COMMAND); 
  delay(1000);
  gsmSerial.print(message);
  delay(1000); 
  gsmSerial.println((char)SMS_END);
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
  WiFi.begin(SSID, PASSWORD);
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
    WiFi.begin(SSID_FALLBACK, PASSWORD_FALLBACK);
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

HTTPClient getHttpClient(String url = AI_SERVER, String fallbackUrl = AI_SERVER_FALLBACK){
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

void executeCommand(char c)
{
  // Serial.print("Received Command:");
  // Serial.println(c);

  if(c==FLAME_IR){
    Serial.print("Fire Occuring!!!!!");
    fireOccurenceIR++;
  }
  else if(c=='V'){
    fireOccurenceIR=-3;
    // Serial.print("Validating");
  }
  else if(c=='A'){
    fireOccurenceIR=-1;
    // Serial.print("Manuevering...");
  }
    else if(c=='Z'){
    fireOccurenceIR=-2;
    // Serial.print("Manuevering...");
  }
  else if(c=='N'){
    fireOccurenceIR=0;
  }
}

void wireReceiveEvent(){
    while (Wire.available())
    {
        char c = Wire.read();
        executeCommand(c);
    }
}

void sendMDMCommand(char command)
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
      sendMDMCommand(FORWARD);
    }
    else if (command == "backward")
    {
      sendMDMCommand(BACKWARD);
    }
    else if (command == "left")
    {
      sendMDMCommand(LEFT);
    }
    else if (command == "right")
    {
      sendMDMCommand(RIGHT);
    }
    else if (command == "stop")
    {
      sendMDMCommand(STOP);
    }
    else if (command == "extingush")
    {
      sendMDMCommand(EXTINGUISH);
    }
    else if (command == "reset")
    {
      sendMDMCommand(RESET_MOTOR);
    }
  }
  server.send(200, "text/plain", "Command received: " + command);
}

// void beep(int spk = SPK_1){
//    for( int i = 0; i<500;i++){
//       digitalWrite(spk , HIGH);
//       delayMicroseconds(500);
//       digitalWrite(spk, LOW );
//       delayMicroseconds(500);
//    }
// }

void blinkLED(int freq, int ms, int led = BLU_LED, int led2 = LED_BUILTIN)
{
  for (int i = 0; i < freq; i++)
  {
    digitalWrite(led, HIGH);
    digitalWrite(led2, HIGH);
    // beep();
    delay(ms);
    digitalWrite(led, LOW);
    digitalWrite(led2, LOW);
    delay(ms);
  }
}

void maneuver()
{
  if (fireOccurenceIR == -1)
  {
    Serial.println("Maneuvering on the Right");
  }
  else if (fireOccurenceIR == -2)
  {
    Serial.println("Maneuvering on the Left");
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
  sendMDMCommand(RESET_MOTOR);
  sendSMS("Fire appears to be out! AFULA-bot has stopped responding to the fire.");
  sendLogs("INFO: Executing Reset.");
  digitalWrite(BLU_LED, HIGH);
  Serial.println("Resetting....");
  fireExtinguishing = false;
  fireDetected = false;
  noFireOccurenceIR = 0;
  fireOccurenceIR = 0;
  
  delay(3000);
  digitalWrite(BLU_LED, LOW);
  sendLogs("INFO: Reset Completed.");
  delay(10000);
}

void onFire()
{
  fireDetected = true;
  sendLogs("INFO: Fire is confirmed.");
  Serial.println();
  Serial.println("Fire Confirmed!!!");
  sendAlert();
  fireExtinguishing = true;
  fireOccurenceIR = 0;
  sendMDMCommand(EXTINGUISH);
  delay(1000);
}

void getMDMCommand(){
  Wire.requestFrom(9, 1);
    if (Wire.available()) {
        char c = Wire.read();
        executeCommand(c);
    }
  delay(10);
}

void setup()
{
  initComponents();
  digitalWrite(BLU_LED,HIGH);
  digitalWrite(RED_LED,HIGH);

  gsmSerial.begin(9600);
  Serial.begin(115200);
  Wire.begin();
  // Wire.begin(9);
  // Wire.onReceive(wireReceiveEvent);

  Serial.println();
  
  delay(1000);
  initWifi();
  // CONFIG_WIFI_LOCAL;
  delay(500);
  // CONNECT_WIFI;
  delay(1000);

  server.on("/command", HTTP_POST, handleCommand);
  server.begin();
  Serial.println();
}

void loop()
{
  if(!init_done){
    Serial.print("Initializing (");
    Serial.print(setupCounter);
    Serial.println(")");  
    server.handleClient();
    delay(1);
    setupCounter++;
    
    if(setupCounter<=1000){
      return;
    }

    digitalWrite(BLU_LED,LOW);
    digitalWrite(RED_LED,LOW);
    sendLogs("INFO: Setup Initiliazed.");

    if(testMode){
      sendLogs("INFO: Test mode is on!");
      Serial.println("Test mode is on!");

    }
    init_done = true;
    sendMDMCommand(STOP);
    sendMDMCommand('O');
  }
  else{
    server.handleClient();
  }

  if(manualDriveMode){
    reset();
    commandTimer++;
    Serial.println("Manual mode is on!");
    digitalWrite(RED_LED,HIGH);
    digitalWrite(BLU_LED,HIGH);

    if(commandTimer>=commandResetTimer){
        sendMDMCommand(STOP);
    }
    return;
  }
  else{
    digitalWrite(RED_LED,LOW);
    digitalWrite(BLU_LED,LOW);
  }

  if (fireDetected && !fireExtinguishing)
  {
    // WHEN FIRE IS DETECTED VIA SERVER
    blinkLED(10, 100);
    Serial.println("Fire Warning!!!");
    // sendMDMCommand('W');
    // delay(3000);
    
    Serial.println("Validating");
    int IRCounter = 0;
    // do{
    //   sendMDMCommand(FLAME_IR);
    //   getMDMCommand();
    //   blinkLED(1,50);
    //   if(fireOccurenceIR>0){
    //     onFire();
    //     sendMDMCommand('W');
    //     return;
    //   }
    //   else if(fireOccurenceIR<0){
    //     Serial.println("MDM to maneuver...");
    //     maneuver();
    //   }
    //   else{
    //     // blinkLED(2,100);
    //     // delay(100);
    //     IRCounter++;
    //   }
    // }while (IRCounter<=100);
    sendMDMCommand('O');
    delay(500);
    sendMDMCommand('O');
    delay(500);
    sendMDMCommand('O');
    delay(500);

    sendMDMCommand(FLAME_IR_MOTOR);
    do{
      // sendMDMCommand(FLAME_IR_MOTOR);
      getMDMCommand();
      delay(50);
      if(fireOccurenceIR>0){
        onFire();
        return;
      }
      else if(fireOccurenceIR<0){
        Serial.println("MDM to maneuver...");
        maneuver();
      }
      else{
        Serial.print("IRCounter: ");
        Serial.println(IRCounter);
        IRCounter++;
      }

    }while(IRCounter<=350);
    
    fireDetected = false;
    sendMDMCommand(STOP);
    sendMDMCommand('O');
    delay(100);
    sendMDMCommand('O');
    delay(100);
  }
  else if (fireDetected && fireExtinguishing)
  {
    //WHEN FIRE IS DETECTED AND BEING PUT OUT

    int fireOutCounter = 0;
    sendMDMCommand(FLAME_IR);
    do{
      sendMDMCommand(FLAME_IR);
      getMDMCommand();
      blinkLED(1,50);
      if(fireOccurenceIR>0){
        fireOutCounter = 0;
      }
      else if(fireOccurenceIR<0){
        Serial.println("MDM to maneuver...");
        maneuver();
      }
      else{
        fireOutCounter++;
      }

    }while(fireOutCounter<=1);
    
    Serial.println("Fire is out!");
    reset();
  }
  else
  {
    //WHEN FIRE IS BEING CHECKED IF EXISTS

    blinkLED(3,1000,LED_BUILTIN);
    Serial.print("Starting fire detection...");
    Serial.print("Detecting using IR from Motor Driver Module");

    int IRCounter = 0;
    do{
      sendMDMCommand(FLAME_IR);
      getMDMCommand();
      delay(50);
      if(fireOccurenceIR>0){
        onFire();
        return;
      }
      else if(fireOccurenceIR<0){
        Serial.println("MDM to maneuver...");
        maneuver();
      }
      else{
        Serial.print("IRCounter: ");
        Serial.println(IRCounter);
        IRCounter++;
      }

    }while(IRCounter<=250);

    Serial.print("No fire detected using IR.");

    sendMDMCommand('O');
    blinkLED(5,500,LED_BUILTIN);
    sendMDMCommand('O');

    Serial.println("Starting fire detection with AI.....");
    fireDetected = VALIDATE_FIRE_AI;
    Serial.println("Prediction: ");
    Serial.print("Fire exists, ");
    Serial.println(fireDetected ? "true" : "false");
  }
}
