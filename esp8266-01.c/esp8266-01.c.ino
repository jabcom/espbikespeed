#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>
#include <WiFiUdp.h>

#ifndef STASSID
#define EEPROM_DISTANCE 0
#define STASSID "HomeNet"
#define STAPSK  "treaclewifi"
#endif

const char* ssid = STASSID;
const char* password = STAPSK;
String menuHTML = "<div><h1>SpeedSensor</h1><br><a href=\"speed\">Speed</a> | <a href=\"settings\">Settings</a></div><hr>";
ESP8266WebServer server(80);
unsigned long lastSendTime = 0;
WiFiUDP Udp;

int lastZeroTime = 0;
int zeroFrequency = 200;
int packetFrequency = 1000;
char* vrHost = "192.168.050.091";
unsigned short vrPort = 5505;
unsigned short stoppingRateMax = 3;
unsigned short stoppingRateCurrent = 0;
float lastSpeed = 0.0;
float currentSpeed = 0.0;
float distance = 0.1;
unsigned long lastUpdateTime = 0;

void handleRoot() {
  server.send(200, "text/html", menuHTML);
}

void handleNotFound() {
  server.send(404, "text/html", menuHTML + "404");
}

void handleSpeed() {
  server.send(200, "text/html", menuHTML + "<span style=\"font-size:300%\">" + String(currentSpeed) + " m/s</span>"+ "<script>setTimeout(function(){ location.reload(); }, 100);</script>");
}

void handlePlainSpeed() {
  server.send(200, "text/html", String(currentSpeed));
}

void handleSettings() {
  for (uint8_t i = 0; i < server.args(); i++) {
    if (server.argName(i) == "distance") {
    distance = server.arg(i).toFloat();
    }
    if (server.argName(i) == "vrhost") {
      server.arg(i).toCharArray(vrHost, 16);
    }
    if (server.argName(i) == "vrport") {
      vrPort = server.arg(i).toInt();
    }
    if (server.argName(i) == "packetfrequency") {
      packetFrequency = server.arg(i).toInt();
    }
    if (server.argName(i) == "zerofrequency") {
      zeroFrequency = server.arg(i).toInt();
    }
  }
  server.send(200, "text/html", menuHTML + "<form action=\"settings\" method=\"POST\">Distance:<input name=\"distance\" value=\"" + String(distance) + "\"><br>VR Host<input name=\"vrhost\" value=\"" + String(vrHost) + "\"><br>VR Port<input name=\"vrport\" value=\"" + String(vrPort) + "\"><br>Packet Frequency<input name=\"packetfrequency\" value=\"" + String(packetFrequency) + "\"><br>Zero Frequency<input name=\"zerofrequency\" value=\"" + String(zeroFrequency) + "\"><br><input type=\"submit\" value=\"Update\"></form>");

}

ICACHE_RAM_ATTR void interruptTimeUpdate() {
  int timePassed = millis() - lastUpdateTime;
  if (timePassed > 3) {
    lastUpdateTime = millis();
    currentSpeed = distance / timePassed * 1000;
    Serial.println("Irpt" + String(timePassed) + " " + String(distance));
  }
}

void saveDistance(){
  //EEPROM.write(EEPROM_DISTANCE, val);
}

void setup(void) {
  //EEPROM.begin(512);
  pinMode(2, INPUT);
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("bikespeed2")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);

  server.on("/speed", handleSpeed);

  server.on("/plainspeed", handlePlainSpeed);

  server.on("/settings", handleSettings);

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
  attachInterrupt(digitalPinToInterrupt(2), interruptTimeUpdate, FALLING);
}

void loop(void) {
  server.handleClient();
  MDNS.update();
  if (millis() > lastSendTime + packetFrequency) {
    lastSendTime = millis();
    Udp.beginPacket(vrHost, vrPort);
    char data[255];
    char speedChar[7];
    dtostrf(currentSpeed, 6, 3, speedChar);
    strcpy(data, "S");
    strcpy(data, speedChar);
    Udp.write(data);
    Udp.endPacket();
    Serial.println("Sending packet");
  }
  if (millis() > lastZeroTime + zeroFrequency) {
    lastZeroTime = millis();
    //Check if bike is stopped
    if (currentSpeed == lastSpeed) {
      if (stoppingRateCurrent >= stoppingRateMax) {
        currentSpeed = 0.0;
      } else {
        stoppingRateCurrent++;
      }
    } else {
      stoppingRateCurrent = 0;
    }
  }
  lastSpeed = currentSpeed;
}
