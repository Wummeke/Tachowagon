String JsonSensorData(bool rounded){
    const size_t capacity = JSON_OBJECT_SIZE(10);
    DynamicJsonDocument doc(capacity);
    // float vmaxr, vmaxh0r;
    if (rounded){
        travelcm = round2(travelcm);
        travelh0 = round2(travelh0);
        speed = round2(speed);
        speedh0 = round2(speedh0);
        // vmaxr = round2(vmax);
        // vmaxh0r = round2(vmaxh0);
    }

    doc["battery"] = round2(ReadBatteryVoltage());
    doc["distance_cm"] = travelcm;
    doc["distance_h0"] = travelh0;
    doc["speed_cms"] = speed;
    doc["speed_h0"] = speedh0;
    if (rounded){
        doc["max_speed_cms"] = round2(vmax);
        doc["max_speed_h0"] = round2(vmaxh0);
        doc["pitch"] = round(pitch);
        doc["roll"] = round(roll);
        doc["temperature"] = round2(temperature);
    } else {
        doc["max_speed_cms"] = vmax;
        doc["max_speed_h0"] = vmaxh0;
        doc["pitch"] = pitch;
        doc["roll"] = roll;
        doc["temperature"] = temperature;
    }
    

    // serializeJson(doc, Serial);
    String response;
    serializeJson(doc, response);

    return response;
}

AsyncEventSource events("/events");
AsyncWebSocket ws("/ws");

void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
}

void handleUpdate(AsyncWebServerRequest *request) {
    request->send(LittleFS, "/getupdate.html", "text/html");
    delay(2000);
    ESP.restart();
}

void handle_sensor_values(AsyncWebServerRequest *request) {
    request->send(200, "application/json", JsonSensorData(false));
}
// void handle_resetvalues(AsyncWebServerRequest *request)
// {
//     String FeedbackMessage = "";
//     const char *PARAM_INPUT = "value";
//     if (request->hasParam(PARAM_INPUT))
//     {
//         resetvalues = (request->getParam(PARAM_INPUT)->value()).toInt();
//         // resetvalues = 0: don't reset anything
//         // resetvalues = 1: reset distance
//         // resetvalues = 2: reset speed
//         // resetvalues = 3 : reset pitch&roll
//         if (resetvalues > 0){
//             reset_clicked = true;
//         }
//         FeedbackMessage = "received reset value = " + String(resetvalues);

//     }
//     else
//     {
//         FeedbackMessage = "No message sent";
//     }
//     Serial.println(FeedbackMessage);
//     request->send(200, "text/plain", "OK");
// }

void handle_switchstate(AsyncWebServerRequest *request)
{
    String inputMessage1;
    String inputMessage2;
    const char *PARAM_INPUT_1 = "output";
    const char *PARAM_INPUT_2 = "state";
    // GET input1 value on <ESP_IP>/update?output=<inputMessage1>&state=<inputMessage2>
    if (request->hasParam(PARAM_INPUT_1) && request->hasParam(PARAM_INPUT_2))
    {
        // inputMessage1 = request->getParam(PARAM_INPUT_1)->value();
        inputMessage2 = request->getParam(PARAM_INPUT_2)->value();
        blinking = inputMessage2.toInt();
    }
    else
    {
        Serial.println("invalid input option");
    }
    request->send(200, "text/plain","ok");
}

String outputState(){
  if(blinking){
    return "checked";
  }
  else {
    return "";
  }
}

String GetSettingsJson(){
    StaticJsonDocument<512> doc;

    // Set the values in the document
    doc["hostname"] = settings.hostname;
    doc["accesspoint"] = settings.accesspoint;
    doc["password"] = settings.password;
    doc["screenrotated"] = settings.screenrotated;
    doc["flashing"] = settings.flashing;
    doc["mqttenabled"] = settings.mqttenabled;
    doc["mqttserver"] = settings.mqttserver;
    doc["mqttport"] = settings.mqttport;
    doc["mqttuser"] = settings.mqttuser;
    doc["mqttpassword"] = settings.mqttpassword;
    doc["mqttretain"] = settings.mqttretain;
    doc["mqtttopic"] = settings.mqtttopic;
    doc["mac_address"] = getMAC();
    doc["FW_VERSION"] = FW_VERSION;

    String response;
    serializeJson(doc, response);
    return response;
}

void notifyClients(String SettingsJson) {
  ws.textAll(SettingsJson);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    message = (char*)data;

    if (message.indexOf("sr") >= 0) {
        Serial.println(message);
      settings.screenrotated = message.substring(2);
      Serial.print(settings.screenrotated);
      notifyClients(GetSettingsJson());
    }
    else if (message.indexOf("ld") >= 0) {
      blinking = message.substring(2).toInt();
      // Serial.println(blinking);
    }
    else if (message.indexOf("rb") >= 0) {
      Serial.println(message);
      String configJson = message.substring(2);
      setConfig(configJson);
      notifyClients(GetSettingsJson());
      savesettings(LittleFS, configfile, settings);
      esp_restart();
    }
    else if (message.indexOf("getData") >= 0){
      Serial.println(message);
      notifyClients(JsonSensorData(false));
    }
    else if (message.indexOf("rst") >=0){
      Serial.println(message);
      // String FeedbackMessage = "";
      int resetvalues = message.substring(3).toInt();
      // resetvalues = 0: don't reset anything
      // resetvalues = 1: reset distance
      // resetvalues = 2: reset speed
      // resetvalues = 3 : reset pitch&roll
      if (resetvalues > 0){
          reset_clicked = true;
      }
      // FeedbackMessage = "received reset value = " + String(resetvalues);
      Serial.println("received reset value = " + String(resetvalues));

      switch (resetvalues){
        case 1:
          ticks = 0;
          travelcm = 0;
          Serial.println("Reset distances");
          break;
        case 2:
          vmax = 0;
          vmaxh0 = 0;
          speed = 0;
          velcms=0;
          Serial.println("Reset speeds");
          break;
        case 3:
          calibrate_sensors();          // Repeat calibration
          for (int i=0; i<300; i++) {
            delay (10);
            calc_mpu_angle ();          // Some readings
          }
          roll_offset = last_x_angle;    // set offsets
          pitch_offset = last_y_angle;
          Serial.println("Recalibrated angles");
          break;
      } 

    }
  }
}

// Websocket event handling
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      notifyClients(GetSettingsJson());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}


void startWebserver()
{
    tachoserver.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/index.html", "text/html");
    });
    tachoserver.serveStatic("/", LittleFS, "/");

    ws.onEvent(onEvent);
    tachoserver.addHandler(&ws);

    tachoserver.on("/check_update", HTTP_GET, [](AsyncWebServerRequest *request){handleUpdate(request);});
    tachoserver.on("/get-json", HTTP_GET, [](AsyncWebServerRequest *request) {handle_sensor_values(request);}); 
    // tachoserver.on("/resetvalues", HTTP_GET, [](AsyncWebServerRequest *request) {handle_resetvalues(request);});
    tachoserver.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) {handle_switchstate(request);});

    tachoserver.begin();
    MDNS.addService("http", "tcp", 80);
}

void wifi_manager_page(){
    init_wifi_ap();

    // Web Server Root URL
    tachoserver.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(LittleFS, "/wifimanager.html", "text/html");
    });
    
    tachoserver.serveStatic("/", LittleFS, "/");
    
    tachoserver.on("/", HTTP_POST, [](AsyncWebServerRequest *request) {
      int params = request->params();
      for(int i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isPost()){
          // HTTP POST ssid value
          if (p->name() == PARAM_INPUT_1) {
            ssid = p->value().c_str();
            Serial.print("SSID set to: ");
            Serial.println(ssid);
            // Write file to save value
            // writeFile(LittleFS, ssidPath, ssid.c_str());
            strlcpy(settings.accesspoint, ssid.c_str(), sizeof(settings.accesspoint));
          }
          // HTTP POST pass value
          if (p->name() == PARAM_INPUT_2) {
            pass = p->value().c_str();
            Serial.print("Password set to: ");
            Serial.println(pass);
            // Write file to save value
            // writeFile(LittleFS, passPath, pass.c_str());
            strlcpy(settings.password, pass.c_str(), sizeof(settings.password));
          }
        }
      }
      savesettings(LittleFS, configfile, settings);
      request->send(200, "text/plain", "Done. ESP will restart, connect to your router and go to IP address: " + ip);
      delay(3000);
      ESP.restart();
    });
    tachoserver.begin();
}
