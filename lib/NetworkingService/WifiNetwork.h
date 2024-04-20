#ifndef WifiNetwork_h
#define WifiNetwork_h

class WifiNetwork{
    public:
        void Config(IPAddress local_IP, IPAddress gateway, IPAddress subnet);
        void Connect(String ssid, String password, String fallback_ssid, String fallback_password);
};

#endif