#include <Arduino.h>

int readVersionFile(fs::FS &fs, const char * path){
    File file = fs.open(path);
    if(!file){
        otafs_version = 0;
    }
    else {
        while(file.available()){
        String line = file.readStringUntil('\n');
        otafs_version = line.toInt();
        }
        file.close();
    }
    Serial.print("OTA FS version: ");
    Serial.println(otafs_version);
    return otafs_version;
}

void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listing directory: %s\r\n", dirname);

    File root = fs.open(dirname);
    if(!root){
        Serial.println("- failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println(" - not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.path(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

void readFile(fs::FS &fs, const char * path){
    Serial.printf("Reading file: %s\r\n", path);

    File file = fs.open(path);
    if(!file || file.isDirectory()){
        Serial.println("- failed to open file for reading");
        return;
    }

    Serial.println("- read from file:");
    while(file.available()){
        Serial.write(file.read());
    }
    file.close();
}

void setConfig(String configJson)
{
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, configJson);

    if (error){
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return;
    }

    String response;
    serializeJson(doc, response);
    Serial.println(response);

    if (doc.containsKey("hostname")) {
        strlcpy(settings.hostname,          // <- destination
                doc["hostname"],                 // <- source
                sizeof(settings.hostname)); // <- destination's capacity
    }
    if (doc.containsKey("accesspoint")) {
        strlcpy(settings.accesspoint,          // <- destination
                doc["accesspoint"],                 // <- source
                sizeof(settings.accesspoint)); // <- destination's capacity
    }
    if (doc.containsKey("password")) {
        strlcpy(settings.password,             // <- destination
                doc["password"],                    // <- source
                sizeof(settings.password));    // <- destination's capacity
    }
    if (doc.containsKey("screenrotated")) {
        settings.screenrotated = doc["screenrotated"];
    }
    if (doc.containsKey("flashing")) {
        settings.flashing = doc["flashing"];
    }
    if (doc.containsKey("mqttenabled")) {
        settings.mqttenabled = doc["mqttenabled"];
    }
    if (doc.containsKey("mqttserver")) {
        strlcpy(settings.mqttserver,
                doc["mqttserver"],
                sizeof(settings.mqttserver));
    }
    if (doc.containsKey("mqttport")) {
        settings.mqttport = doc["mqttport"];
    }
    if (doc.containsKey("mqttuser")) {
        strlcpy(settings.mqttuser,
                doc["mqttuser"],
                sizeof(settings.mqttuser));
    }
    if (doc.containsKey("mqttpassword")) {
        strlcpy(settings.mqttpassword,
                doc["mqttpassword"],
                sizeof(settings.mqttpassword)); 
    }
    if (doc.containsKey("mqttretain")) {
        settings.mqttretain = doc["mqttretain"];
    }
    if (doc.containsKey("mqtttopic")) {
        strlcpy(settings.mqtttopic,
                doc["mqtttopic"],
                sizeof(settings.mqtttopic));
    }
}

void loadsettings(fs::FS &fs, const char * path, Config &settings)
{
    Serial.printf("Reading file: %s\r\n", path);
    File file = fs.open(path);
    if(!file || file.isDirectory()){
        Serial.println("- failed to open file for reading");
        return;
    }
    Serial.println("- read config from file");

    String configJson;
	while (file.available())
	{
		char charRead = file.read();
		configJson += charRead;
	}

    setConfig(configJson);
    file.close();
}

void savesettings(fs::FS &fs, const char *path, Config &settings)
{
    // Delete existing file, otherwise the settings is appended to the file
    if (fs.remove(path))
    {
        Serial.println("- file deleted");
        File file = fs.open(path, FILE_WRITE);
        if (!file)
        {
            Serial.println("- failed to open file for writing");
            return;
        }

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

        // Serialize JSON to file
        if (serializeJson(doc, file) == 0)
        {
            Serial.println(F("Failed to write to file"));
        }

        file.close();
    }
    else
    {
        Serial.println("- delete failed");
    }
}

void writeFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Writing file: %s\r\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        Serial.println("- failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("- file written");
    } else {
        Serial.println("- write failed");
    }
    file.close();
}

void checkConfigFile(fs::FS &fs, const char * path){
    bool fileExists = fs.exists(configfile);
    if (!fileExists){
        writeFile(LittleFS, configfile, "{\"wifi\":[{\"ap0\":\"SSID/password\"}]}");
    }
    else {
        Serial.println("Config file exists!");
        readFile(LittleFS, configfile);
    }
}

void initFileSystem()
{
    if (!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)) {
        Serial.println("LittleFS Mount Failed");
        return;
    }
    loadsettings(LittleFS, configfile, settings);

    // Load values saved in SPIFFS
    devicename = settings.hostname;
    hostName = settings.hostname;
    ssid = settings.accesspoint;
    pass = settings.password;
    blinking = settings.flashing;
    mqttBaseTopic = settings.mqtttopic;
    MQTT_PUBLISH = settings.mqttenabled;
}
