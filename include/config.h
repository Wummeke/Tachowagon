const int FW_VERSION = 23092309;

// ### Included Libraries ###
#include <Arduino.h>
#include <FS.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPmDNS.h>
#include <ESPAsyncWebServer.h>
#include <ESPAsyncWiFiManager.h>  
#include <HTTPClient.h>
#include <Update.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <U8g2lib.h>
#include <PubSubClient.h>

// ### Variables ###
const char *hostName = "tachowagon";
// #define LOCAL_SSID "Tachowagon"
// #define LOCAL_PASS "tacho123"


//MQTT settings
bool MQTT_PUBLISH;
String mqttBaseTopic;

// bool shouldSaveConfig = false;

//OTA settings
const char* fwUrlBase = "http://192.168.0.2/ESP/";
size_t content_len;
// int spiffs_version; OLD!

// ### FILESYSTEM STUFF ###
int otafs_version;
const char *configfile = "/settings.json";
#define FORMAT_LITTLEFS_IF_FAILED true

// ### FLASH CONFIG ###
struct Config {
  char hostname[64];
  char accesspoint[64];
  char password[128];
  bool screenrotated;
  bool flashing;
  char mqttserver[64];
  uint16_t mqttport;
  char mqttuser[64];
  char mqttpassword[64];
  bool mqttenabled;
  bool mqttretain;
  char mqtttopic[128];

};
Config settings;


//### Variable Definitions ###
//## Hardware
#define senspin         19
#define I2C_SDA         21
#define I2C_SCL         22

#define SSD1306_ADDRESS 0x3C

// ## calculation variables
float diameter = 0.0115; //wheel diameter in meter

float speed=0, speedh0=0, travelh0=0, travelmeter=0, vmax = 0, vmaxh0 = 0;
int veltype=0, resetvalues = 0;    // default: km/h (H0) & m
float travelcm, velcms;
volatile unsigned long ticks=0, speedticks=0, timetaken=0, lastInt=0, lastwebupdate=0, iponlcd=0, lastmqttupdate=0; // used in ISR and loop
unsigned long dtime, prevtime=0, itime;
unsigned long msTime, media, sumatotal=0;
unsigned long currTick, lastTick=0;
bool reset_clicked = false;
bool showip = true;

// Variables for MPU6050
unsigned long mputread=0;
float pitch_offset, roll_offset;
float temperature, pitch, roll;
unsigned long last_read_time;
float last_x_angle, last_y_angle, last_z_angle;                 // These are the filtered angles
float last_gyro_x_angle, last_gyro_y_angle, last_gyro_z_angle;  // Store the gyro angles to compare drift
float base_x_gyro = 0, base_y_gyro = 0, base_z_gyro = 0;        //  to calibrate the gyroscope sensor and accelerometer readings
float base_x_accel = 0, base_y_accel = 0, base_z_accel = 0;
int16_t ax, ay, az;                                             // Variables to store the values from the sensor readings
int16_t gx, gy, gz;
int16_t rawTemp;
unsigned long t_now;
#define GYRO_FACTOR 65.536                  // 32768 / 500dps
#define ACCEL_FACTOR 4096.0                 // 32768 / 8g
const float RADIANS_TO_DEGREES = 57.2958;   // 180 / 3.14159
const float DEGREES_TO_RADIANS = 0.01745;   // 3.14159 / 180

//BlinkingLEDs
bool blinking;
#define LED1   16
#define LED2   17
// old config stuff
int ledState1 = LOW, ledState2 = LOW;
long blinkMillis1 = 0, blinkMillis2 = 0; 
long interval1 = 525, interval2 = 550, OnTime = 50;
//new config stuff
byte leds[2] = {LED1, LED2};
byte currentLed;
#define FLASH_DELAY 40
#define PAUSE_DELAY 500
#define FLASH_TIMES 4

// Program variables
float BatteryVoltage;

#define webrefreshrate 500 //update sensordata every x milliseconds
#define mqttupdaterate 30000
#define mqttreconnect 10000 // How long to try to reconnect?
#define showiponLCD 10000 //duration in MS how long the local IP is shown on the LCD


// ### WIFI STUFF ###
// Search for parameter in HTTP POST request
const char* PARAM_INPUT_1 = "ssid";
const char* PARAM_INPUT_2 = "pass";

//Variables to save values from HTML form
String ssid;
String pass;
String ip;
String gateway;
String subnet;
String devicename;
IPAddress localIP;
IPAddress localGateway;
IPAddress localSubnet;
bool screenrotation;
bool mqttenabled;
String message;

// Timer variables
unsigned long previousMillis = 0;
const long WiFiinterval = 10000;  // interval to wait for Wi-Fi connection (milliseconds)
