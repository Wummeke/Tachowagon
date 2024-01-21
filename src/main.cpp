#include <Arduino.h>
#include "config.h"
#include "filesystem.h"
#include "screen.h"
#include "sensors.h"
#include "wifihandling.h"
#include "connectivity.h"
#include "tachowebserver.h"
#include "mqtthandling.h"
#include "led_blink.h"


void measure_speed() {            // interrupt routine, counts time from last change in opto
  dtime = micros();
  itime = dtime - prevtime;
  if (itime > 3999) {                // min. 4ms between falling edges
    ticks++;
    speedticks++;
    timetaken = itime;
    prevtime = dtime;
  }
  lastInt = dtime;
}

void setup()
{
  // put your setup code here, to run once:
  btStop();
  Serial.begin(115200);
  Serial.println("Starting");
  pinMode(senspin, INPUT);
  attachInterrupt(digitalPinToInterrupt(senspin), measure_speed, FALLING);
  
  initFileSystem();
  screen_setup();
  mpu6050Init();
  init_leds();

  Serial.println(ssid);
  Serial.println(pass);
  Serial.println(devicename);

  if(initWiFi()) {
    if (!MDNS.begin(devicename)) {
        Serial.println("Error setting up MDNS responder!");
        while(1) {
            delay(1000);
        }
    }
    Serial.println("mDNS responder started");
  
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.println(getMAC());

    if (!checkForUpdates()) {
      Serial.println("no updates");
    }

    startWebserver();
    Serial.println("HTTP server started");
    delay(100);
    screen_print("Tachowagon", 0 , 24); 
  }
  else{
    wifi_manager_page();
  }

}

void loop()
{
  currTick = ticks;
  float L_cm = (2 * PI * (diameter / 2)) * 100;
  travelcm = L_cm * currTick;
  travelmeter = travelcm / 100;    // m
  travelh0 = travelcm * 0.00087; // km (H0)   km = 87 * (1/100000) km/cm * travelcm

  msTime = timetaken;
  currTick = speedticks;
  if (currTick != lastTick) {
    if (msTime) {
      lastTick = currTick;
      media = sumatotal / 4;        // medium value from 4 last readings
      media = sumatotal - media + msTime;
      velcms = (1000000 * L_cm)  / media;
    }
  }

  speed = velcms;
  speedh0 = velcms * 3.132;      // km/h (H0)   km/h = 87 * (1/100000) km/cm * 3600 s/h * vel cm/s
  if (speed > vmax){
    vmax = speed;
    vmaxh0 = speedh0;
  }


  if ((micros() - lastInt) > 4000000) {  // 4s from last interrupt? -> Wagon stopped
    speed = 0;
    timetaken=0;
    velcms=0;
    lastInt = micros();
  }

  if ((showip = true) && (millis() - iponlcd < showiponLCD)) {
    String LocalIP = String() + WiFi.localIP()[0] + "." + WiFi.localIP()[1] + "." + WiFi.localIP()[2] + "." + WiFi.localIP()[3];
    
    screen_print(LocalIP.c_str(), 0, 24);
    
  } else {
    showip = false;
    print_speed(speedh0);
  }
  
if (millis() - mputread > 5) { 
    calc_mpu_angle();
    mputread = millis();
   }

  if ((millis() - lastwebupdate) > webrefreshrate) {
    mpuMeasurement();
    
    // Serial.print("X : ");
    // Serial.print(roll);
    // Serial.print("\tY : ");
    // Serial.println(pitch);

    events.send("ping",NULL,millis());
    events.send(String(JsonSensorData(true)).c_str(),"sensordata",millis());
    notifyClients(JsonSensorData(true));
    lastwebupdate = millis(); // reset update timeout
    // Serial.println("Web updated");
  }

  if (MQTT_PUBLISH)
  {
    if ((millis() - lastmqttupdate) > mqttupdaterate)
    {
      const String pubTopic = mqttBaseTopic + "measurement";
      pub2mqtt(pubTopic, String(JsonSensorData(true)).c_str());
      lastmqttupdate = millis();
    }
  }

  if (blinking){
  ledblink();
  }
}