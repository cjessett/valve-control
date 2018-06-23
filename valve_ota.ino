#include <ESP8266WiFi.h>
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

const char* AWS_endpoint = AWS_ENDPOINT; //MQTT broker ip

int solenoidPin = 2;

void callback(char* topic, byte* payload, unsigned int length);

WiFiClientSecure espClient;
PubSubClient client(AWS_endpoint, 8883, callback, espClient);

void callback(char* topic, byte* payload, unsigned int length) {
  // Memory pool for JSON object tree.
  //
  // Inside the brackets, 200 is the size of the pool in bytes.
  // Don't forget to change this value to match your JSON document.
  // Use arduinojson.org/assistant to compute the capacity.
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.parseObject(payload);
  if (!root.success()) {
   Serial.println("parseObject() failed");
   return;
  }
  const char* desired = root["desired"];
  Serial.println(desired);
  if (String(desired) == "open") {
    Serial.println("desired: open");
    digitalWrite(solenoidPin, HIGH);

    client.publish(UPDATE_TOPIC, "{\"state\":{\"reported\": {\"valve\": \"open\"}}}");
  } else {
    Serial.println("desired: closed");
    digitalWrite(solenoidPin, LOW);

    client.publish(UPDATE_TOPIC, "{\"state\":{\"reported\": {\"valve\": \"closed\"}}}");
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Booting");
  pinMode(solenoidPin, OUTPUT);
  // should read from AWS
  digitalWrite(solenoidPin, LOW);

  WiFiManager wifiManager;
  wifiManager.autoConnect();
  Serial.println("connected...Hello :)");
  Serial.println(WiFi.localIP());

  espClient.setCACert(certificates_esp8266_bin_CA, certificates_esp8266_bin_CA_len);
  espClient.setCertificate(certificates_esp8266_bin_crt, certificates_esp8266_bin_crt_len);
  espClient.setPrivateKey(certificates_esp8266_bin_key, certificates_esp8266_bin_key_len);

  client.setCallback(callback);
  
  client.publish(UPDATE_TOPIC, "{\"state\":{\"reported\": {\"valve\": \"closed\"}}}");
  client.subscribe(DELTA_TOPIC);
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

