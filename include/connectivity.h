HTTPClient WebClient;
WiFiClient tachoClient;
AsyncWebServer tachoserver(80);
DNSServer dns;

String getMAC()
{
  uint8_t mac[6];
  char result[14];
  WiFi.macAddress(mac);

  snprintf(result, sizeof(result), "%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  return String(result);
}

void configModeCallback (AsyncWiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  
  Serial.println(WiFi.softAPIP());
  
  String APIP = String() + WiFi.softAPIP()[0] + "." + WiFi.softAPIP()[1] + "." + WiFi.softAPIP()[2] + "." + WiFi.softAPIP()[3];
  screen_print(APIP.c_str(), 0, 24);
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

void execOTA(String OTAurl, int updateType)
{
  // ### updateType 0 = Firwmare, updateType 100 = Data
  screen_print("Install update", 0 , 24);
  WebClient.begin(OTAurl);

  int httpCode = WebClient.GET();
  if (httpCode != HTTP_CODE_OK)
  {
    Serial.println("HTTP response should be 200");
    return;
  }
  int contentLength = WebClient.getSize();
  if (contentLength <= 0)
  {
    Serial.println("Content-Length not defined");
    return;
  }

  if (updateType > 0 ){
    LittleFS.end();
    delay(100);
  }
  
  bool canBegin = Update.begin(contentLength, updateType);
  if (!canBegin)
  {
    Serial.println("Not enough space to begin OTA");
    return;
  }
  else
  {
    Update.onProgress([](unsigned int progress, unsigned int total) {
    });
  }

  Client &client = WebClient.getStream();
  int written = Update.writeStream(client);
  if (written != contentLength)
  {
    Serial.println(String("OTA written ") + written + " / " + contentLength + " bytes");
    return;
  }

  if (!Update.end())
  {
    Serial.println("Error #" + String(Update.getError()));
    return;
  }

  if (!Update.isFinished())
  {
    Serial.println("Update failed.");
    return;
  }

  Serial.println("Update successfully completed. Rebooting.");
  // delay(1000);
  ESP.restart();
}

bool checkUpdate(String VersionURL, int CurrentVersion){

  WebClient.begin(VersionURL);
  int httpCode = WebClient.GET();
  Serial.println(httpCode);
  String httpresponse = "Got HTTP response: " + String(httpCode);
  Serial.println(httpresponse);

  // Code OTA update check
  if (httpCode == 200)
  {
    String newVersionString = WebClient.getString();
    Serial.println(newVersionString);
    int newVersion = newVersionString.toInt();

    if (newVersion > CurrentVersion)
    {
      Serial.print("Found update: ");
      return true;
    }
    else
    {
      return false;
    }
  }
  else
  {
    return false;
  }
}

bool checkForUpdates(){
  screen_print("Check update", 0 , 24);
  String mac = getMAC();
  String fwURL = String(fwUrlBase);
  fwURL.concat(mac);

  // ### FW Update ###
  String fwVersionURL = fwURL;
  fwVersionURL.concat(".version");
  Serial.println(fwVersionURL);
  String fwImageURL = fwURL;
  fwImageURL.concat(".bin");
  if (checkUpdate(fwVersionURL, FW_VERSION)){
    String fwImageURL = fwURL;
    fwImageURL.concat(".bin");
    Serial.println(fwImageURL);
    execOTA(fwImageURL, 0);
    return true;
  } else {
    Serial.println("No Firmware update");
  }

  // ### Data Update ###
  int currentver = readVersionFile(LittleFS, "/version.txt");
  String dataVersionURL = fwURL;
  dataVersionURL.concat(".dataver");
  Serial.println(dataVersionURL);
  if(checkUpdate(dataVersionURL, currentver))
  {
    String dataImageURL = fwURL;
    dataImageURL.concat("_data.bin");
    Serial.println(dataImageURL);
    execOTA(dataImageURL, 100);
    return true;
  } else {
    Serial.println("No data update");
  }

  return false;
}