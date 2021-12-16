#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#define PIN_SCL 3
#define PIN_SDA 1
#define PIN_LED 2
#define DEFAULT_UPDATEFREQUENCY 90
#define DEFAULT_SSID "HomeNet"
#define DEFAULT_PASSWORD "treaclewifi"
#define DEFAULT_VRHOST "192.168.50.91"
#define DEFAULT_VRPORT 36140
#define DEFAULT_WHEELRADIUS 3302
#define EEPROM_SETTINGS_VERSION 2

char ssid[32] = DEFAULT_SSID;
char password[63] = DEFAULT_PASSWORD;
char vrHost[16] = DEFAULT_VRHOST;
char deviceName[32] = "Gyro Speed Sensor";
unsigned int vrPort = DEFAULT_VRPORT;
unsigned int wheelRadius = DEFAULT_WHEELRADIUS;
unsigned int lastTimeSpeed = 0;
unsigned int updateFrequency = DEFAULT_UPDATEFREQUENCY;
float velocity = 0;
bool ledPktFlag = false;
int wifiTryCount;

ESP8266WebServer server(80);
WiFiUDP Udp;
Adafruit_MPU6050 mpu;

void sendPacket(String msg) {
  Udp.beginPacket(vrHost, vrPort);
  char data[1024];
  msg.toCharArray(data, 1024);
  Udp.write(data);
  Udp.endPacket();
}

void htmlRoot() {
  bool saveSettingsFlag = false;
  for (uint8_t i = 0; i < server.args(); i++) {
    if (server.argName(i) == "saveSettings") {
      saveSettingsFlag = true;
    }
    if (server.argName(i) == "vrHost") {
      //vrHost
      server.arg(i).toCharArray(vrHost, 16);
    } else if (server.argName(i) == "vrPort") {
      //vrPort
      vrPort = server.arg(i).toInt();
    } else if (server.argName(i) == "updateFrequency") {
      //updateFrequency
      if (1 < server.arg(i).toInt() < 1000) {
        updateFrequency = server.arg(i).toInt();
      } else {
        updateFrequency = DEFAULT_UPDATEFREQUENCY;
      }
    } else if (server.argName(i) == "wheelRadius") {
      //wheelRadius
      if (1 < server.arg(i).toInt() < 10000) {
        wheelRadius = server.arg(i).toInt();
      } else {
        wheelRadius = DEFAULT_WHEELRADIUS;
      }
    } else if (server.argName(i) == "deviceName") {
      //vrPort
      server.arg(i).toCharArray(deviceName, 32);
    }
    
  }
  if (saveSettingsFlag) {
    saveSettings();
  }
  char html[2048];
  sprintf(html, "\
  <html><head><title>%s</title></head><body>\
  <h1>%s</h1>\
  <h3>Speed</h3><br><div style=\"font-size:200%\"><span id=\"speed\">Unknown</span>m/s</div><hr>\
  <h3>Settings<h3><br><div><form method=\"POST\" action=\"/\">\
  DeviceName:<br> <input value=\"%s\" name=\"deviceName\"><br>\
  SSID: (disabled)<br> <input disabled value=\"%s\" name=\"ssid\"><br>\
  Password: (disabled) <br><input disabled value=\"%s\" name=\"password\"><br>\
  VR Host: <br><input value=\"%s\" name=\"vrHost\"><br>\
  VR Port:<br> <input value=\"%i\" name=\"vrPort\"><br>\
  WheelRadius:<br> <input value=\"%i\" name=\"wheelRadius\"><br>\
  Update Frequency:<br> <input value=\"%i\" name=\"updateFrequency\"><br><br>\
  <input type=\"submit\" value=\"Update\">\
  <input type=\"hidden\" name=\"saveSettings\">\
  </form>\
  </div></body>\
  <script>\
  function getSpeed() {\
    var req = new XMLHttpRequest();\
    req.onreadystatechange = function() {\
      console.log(this.responseText);\
      if (this.readyState == 4) {\
        document.getElementById(\"speed\").innerHTML = this.responseText;\
      }\
    };\
    req.open(\"GET\", \"/speed\", false);\
    req.send();\
  };\
  setInterval(getSpeed, 500);\
  </script></html>", deviceName, deviceName, deviceName, ssid, password, vrHost, vrPort, wheelRadius, updateFrequency);
  server.send(200, "text/html", html);
}

void html404() {
 server.send(404, "text/html", "Page not found<br><a href=\"/\">Go To Main Page</a>");
}

void htmlSpeed() {
  server.send(200, "text/plain", String(velocity));
}

void saveSettings() {
  EEPROM.write(0, EEPROM_SETTINGS_VERSION);
  int offset = 1;
  for (int i = 0; i < 32; i++) {
    EEPROM.write(i + offset, deviceName[i]);
  }
  offset = offset + 32;
  for (int i = 0; i < 32; i++) {
    EEPROM.write(i + offset, ssid[i]);
  }
  offset = offset + 32;
  for (int i = 0; i < 63; i++) {
    EEPROM.write(i + offset, password[i]);
  }
  offset = offset + 63;
  for (int i = 0; i < 16; i++) {
    EEPROM.write(i + offset, vrHost[i]);
  }
  offset = offset + 16;
  byte int1 = vrPort >> 8;
  byte int2 = vrPort & 0xFF;
  EEPROM.write(offset + 1, int1);
  EEPROM.write(offset + 2, int2);
  int1 = wheelRadius >> 8;
  int2 = wheelRadius & 0xFF;
  EEPROM.write(offset + 3, int1);
  EEPROM.write(offset + 4, int2);
  int1 = updateFrequency >> 8;
  int2 = updateFrequency & 0xFF;
  EEPROM.write(offset + 5, int1);
  EEPROM.write(offset + 6, int2);  
  //EEPROM.commit();
}

int readIntFromEEPROM(int address)
{
  byte byte1 = EEPROM.read(address);
  byte byte2 = EEPROM.read(address + 1);
  return (byte1 << 8) + byte2;
}

void populateSettings() {
  int settingsVersion = EEPROM.read(0);
  if (settingsVersion == EEPROM_SETTINGS_VERSION) {
    //deviceName
    int offset = 1;
    for (int i = 0; i < 32; i++) {
      deviceName[i] = EEPROM.read(i + offset);
    }
    offset = offset + 32;
    //ssid
    for (int i = 0; i < 32; i++) {
      ssid[i] = EEPROM.read(i + offset);
    }
    offset = offset + 32;
    //password
    for (int i = 0; i < 63; i++) {
      password[i] = EEPROM.read(i + offset);
    }
    offset = offset + 63;
    //vrHost
    for (int i = 0; i < 16; i++) {
      vrHost[i] = EEPROM.read(i + offset);
    }
    offset = offset + 16;
    //vrPort
    vrPort = readIntFromEEPROM(offset);
    wheelRadius = readIntFromEEPROM(offset + 2);
    updateFrequency = readIntFromEEPROM(offset + 4);
  } else {
    for (int i = 0; i< 10; i++) {
      digitalWrite(PIN_LED, LOW);
      delay(500);
      digitalWrite(PIN_LED, HIGH);
      delay(500);
    }
  }
}

void setup() {
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);
  //read settings
  EEPROM.begin(512);
  populateSettings();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(PIN_LED, HIGH);
    delay(50);
    digitalWrite(PIN_LED, LOW);
    delay(50);
  }
  digitalWrite(PIN_LED, LOW);
  sendPacket("Booted and online");
  server.on("/", htmlRoot);
  server.on("/speed", htmlSpeed);
  server.onNotFound(html404);
  server.begin();
  sendPacket("Beginning Wire");
  Wire.begin(PIN_SDA,PIN_SCL);
  sendPacket("Setting up MPU");
  while(!mpu.begin())
  {
    sendPacket("Searching for MPU");
    digitalWrite(PIN_LED, HIGH);
    delay(100);
    digitalWrite(PIN_LED, LOW);
    delay(100);
  }
  digitalWrite(PIN_LED, LOW);
  sendPacket("MPU found calibrating");
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_2000_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  sendPacket("Setup complete");
}
void sendSpeed() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  float totalRotation = g.gyro.x + g.gyro.y + g.gyro.z;
  velocity = totalRotation * 0.5;
  if (velocity < 0.15) {
    velocity = 0.0;
  }
  char data[256];
  //sprintf(data,"S%fT%fF%iD%i",velocity,temp,updateFrequency,(millis() - lastTimeSpeed));
    sprintf(data,"%f",velocity);

  sendPacket(data);
  if (ledPktFlag) {
    ledPktFlag = false;
    digitalWrite(PIN_LED, HIGH);
  } else {
    ledPktFlag = true;
    digitalWrite(PIN_LED, LOW);
  }
}

void loop() {
  if ((millis() - lastTimeSpeed) > (1000 / updateFrequency)) {
    sendSpeed();
    lastTimeSpeed = millis();
  }
    server.handleClient();
}
