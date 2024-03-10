#include "globals.h"

#define BLINDS_1_PIN  19
#define BLINDS_2_PIN 21
#define WINDOW_PIN 18

#define BLINDS_1_LED 13
#define BLINED_2_LED 12
#define WINDOW_LED 14

int blinds_1_state;
int blinds_2_state;
int window_state;

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  mqtt_setup();
  pinMode(BLINDS_1_PIN, INPUT_PULLUP);
  pinMode(BLINDS_2_PIN, INPUT_PULLUP);
  pinMode(WINDOW_PIN, INPUT_PULLUP);

  pinMode(BLINDS_1_LED, OUTPUT);
  pinMode(BLINED_2_LED, OUTPUT);
  pinMode(WINDOW_LED, OUTPUT);
}

void loop() {
  mqtt_client.loop();
  delay(10);

  if (!mqtt_client.connected()) {
    mqtt_connect();
  }

  blinds_1_state = digitalRead(BLINDS_1_PIN);
  blinds_2_state = digitalRead(BLINDS_2_PIN);
  window_state = digitalRead(WINDOW_PIN);

  String topic = "/esp32/";
  topic += TYPE;
  
  String output = "{ \"blinds_1\": \"";
  
  if (blinds_1_state == HIGH) {
    output += "open";
    Serial.print("[Blinds 1 open | ");
    digitalWrite(BLINDS_1_LED, HIGH);
  } else {
    output += "closed";
    Serial.print("[Blinds 1 closed | ");
    digitalWrite(BLINDS_1_LED, LOW);
  }

  output += "\", \"blinds_2\": \"";
 
  if (blinds_2_state == HIGH) {
    output += "open";
    Serial.print("Blinds 2 open | ");
    digitalWrite(BLINED_2_LED, HIGH);
  } else {
    output += "closed";
    Serial.print("Blinds 2 closed | ");
    digitalWrite(BLINED_2_LED, LOW);
  }

  output += "\", \"window\": \"";

  if (window_state == HIGH) {
    output += "open";
    Serial.println("Window open]");
    digitalWrite(WINDOW_LED, HIGH);
  } else {
    output += "closed";
    Serial.println("Window closed]");
    digitalWrite(WINDOW_LED, LOW);
  }

  output += "\" }";
  mqtt_client.publish(topic.c_str(), output.c_str());
  delay(5000);
}