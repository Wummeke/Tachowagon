#include <Arduino.h>

PubSubClient mqtt(tachoClient);


void pub2mqtt(String topic, const char* payload){
    
    mqtt.setServer(settings.mqttserver, settings.mqttport);
    int stop_connecting = millis() + mqttreconnect;
    while (!mqtt.connected())
    {
        if (millis() < stop_connecting) {
            Serial.println(settings.mqttuser);
            Serial.println(settings.mqttpassword);
            Serial.println(hostName);
            Serial.println(settings.mqttport);
            if (mqtt.connect(hostName, settings.mqttuser, settings.mqttpassword)) {
                mqtt.publish(topic.c_str(), payload, settings.mqttretain);
                // Serial.print("Message published to broker");
                // Serial.println(settings.mqttserver);
            }
        }
        else{
            Serial.print("Error Connecting to MQTTBroker: ");
            Serial.println(settings.mqttserver);
            break;
        }
    }
    mqtt.disconnect();

}