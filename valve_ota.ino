#include <Arduino.h>

#include <ESP8266WiFi.h>

#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

#include <PubSubClient.h>         //Get it from here: https://github.com/knolleary/pubsubclient
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>

#include <ArduinoJson.h>
#include "secrets.h"

unsigned char certificates_esp8266_bin_crt[] = BIN_CRT;
unsigned int certificates_esp8266_bin_crt_len = BIN_CRT_LEN;
unsigned char certificates_esp8266_bin_key[] = BIN_KEY;
unsigned int certificates_esp8266_bin_key_len = BIN_KEY_LEN;
unsigned char certificates_esp8266_bin_CA[] = BIN_CA;
unsigned int certificates_esp8266_bin_CA_len = BIN_CA_LEN;

// callback header
void callback(char* topic, byte* payload, unsigned int length);

WiFiClientSecure espClient;
PubSubClient client(AWS_ENDPOINT, 8883, callback, espClient);

void checkForUpdate() {
  // wait for WiFi connection
  if (WiFi.status() == WL_CONNECTED) {
    t_httpUpdate_return ret = ESPhttpUpdate.update(FW_UPDATE_URL);

    switch (ret) {
      case HTTP_UPDATE_FAILED:
        Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
        break;

      case HTTP_UPDATE_NO_UPDATES:
        Serial.println("HTTP_UPDATE_NO_UPDATES");
        break;

      case HTTP_UPDATE_OK:
        Serial.println("HTTP_UPDATE_OK");
        break;
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  if (String(topic) == FW_UPDATE_TOPIC) {
    Serial.println("fetching firmware update...");
    return checkForUpdate();
  }

  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(payload);
  if (!root.success()) {
   Serial.println("parseObject() failed");
   return;
  }

  const char* desired = root["desired"];

  if (String(desired) == "open") {
    Serial.println("desired: open");
    digitalWrite(SOLENOID_PIN, HIGH);

    client.publish(UPDATE_TOPIC, "{\"state\":{\"reported\": {\"valve\": \"open\"}}}");
  } else {
    Serial.println("desired: closed");
    digitalWrite(SOLENOID_PIN, LOW);

    client.publish(UPDATE_TOPIC, "{\"state\":{\"reported\": {\"valve\": \"closed\"}}}");
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Booting");

  pinMode(SOLENOID_PIN, OUTPUT);

  // should read from AWS
  digitalWrite(SOLENOID_PIN, LOW);

  WiFiManager wifiManager;
  wifiManager.autoConnect();
  Serial.println("connected...Hello :)");

  espClient.setCACert(certificates_esp8266_bin_CA, certificates_esp8266_bin_CA_len);
  espClient.setCertificate(certificates_esp8266_bin_crt, certificates_esp8266_bin_crt_len);
  espClient.setPrivateKey(certificates_esp8266_bin_key, certificates_esp8266_bin_key_len);

  client.setCallback(callback);

  client.publish(UPDATE_TOPIC, "{\"state\":{\"reported\": {\"valve\": \"closed\"}}}");
  client.subscribe(DELTA_TOPIC);
  client.subscribe(FW_UPDATE_TOPIC);
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.subscribe(DELTA_TOPIC);
      client.subscribe(FW_UPDATE_TOPIC);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}
