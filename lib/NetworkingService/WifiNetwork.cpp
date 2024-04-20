#include "WifiNetwork.h"
#include <ESP8266WiFi.h>

void WifiNetwork::Config(IPAddress local_IP, IPAddress gateway, IPAddress subnet)
{
    if (!WiFi.config(local_IP, gateway, subnet))
    {
        Serial.println("STA Failed to configure");
    }
}

void WifiNetwork::Connect(String ssid, String password, String fallback_ssid, String fallback_password)
{
    WiFi.begin(ssid, password);
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
        WiFi.begin(fallback_ssid, fallback_password);
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