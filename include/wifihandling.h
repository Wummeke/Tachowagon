#include <Arduino.h>

bool initWiFi()
{
    if (ssid == "") {
        Serial.println("Undefined SSID.");
        return false;
    }
    if (devicename == "") {
        devicename = "ESP-WIFI-MANAGER";
    }

    WiFi.mode(WIFI_OFF);
    WiFi.mode(WIFI_STA);

    if (ip != "") {
        localIP.fromString(ip.c_str());
    } else {
        localIP = INADDR_NONE;
    }

    if (gateway != "") {
        localGateway.fromString(gateway.c_str());
    } else {
        localGateway = INADDR_NONE;
    }

    if (subnet != "") {
        localSubnet.fromString(subnet.c_str());
    } else {
        localSubnet = INADDR_NONE;
    }

    WiFi.setHostname(devicename.c_str());
    if (!WiFi.config(localIP, localGateway, localSubnet)) {
        Serial.println("STA Failed to configure");
        return false;
    }

    WiFi.begin(ssid.c_str(), pass.c_str());
    Serial.print("Connecting to WiFi as: ");
    Serial.println(WiFi.getHostname());

    unsigned long currentMillis = millis();
    previousMillis = currentMillis;

    while (WiFi.status() != WL_CONNECTED)
    {
        currentMillis = millis();
        if (currentMillis - previousMillis >= WiFiinterval) {
            Serial.println("Failed to connect.");
            return false;
        }
    }

    Serial.println(WiFi.localIP());
    return true;
}

void init_wifi_ap(){
    // Connect to Wi-Fi network with SSID and password
    Serial.println("Setting AP (Access Point)");
    // NULL sets an open Access Point
    WiFi.softAP(devicename.c_str(), NULL);

    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP); 
}