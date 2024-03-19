#include <Arduino.h>

#include "env.h"
#include "ArduinoJson.h"
#include "communication.h"

const char ssid[] = SSID_ENV;
const char pass[] = PASSWORD_ENV;

const char mqtt_host[] = MQTT_HOST_ENV;
const int mqtt_port = MQTT_PORT_ENV;

JsonDocument data;

#define BLINDS_1_PIN 19
#define BLINDS_2_PIN 21
#define WINDOW_PIN 18

#define BLINDS_1_LED 13
#define BLINED_2_LED 12
#define WINDOW_LED 14

int blinds_1_state;
int blinds_2_state;
int window_state;

void callback(String &topic, String &payload)
{
  Serial.print("Topic: ");
  Serial.print(topic);
  Serial.print(", Payload: ");
  Serial.println(payload);
}

Communication comm(SSID_ENV, PASSWORD_ENV, "/esp32/", MQTT_HOST_ENV, MQTT_PORT_ENV, callback);

void setup()
{
  Serial.begin(115200);

  pinMode(BLINDS_1_PIN, INPUT_PULLUP);
  pinMode(BLINDS_2_PIN, INPUT_PULLUP);
  pinMode(WINDOW_PIN, INPUT_PULLUP);
  pinMode(BLINDS_1_LED, OUTPUT);
  pinMode(BLINED_2_LED, OUTPUT);
  pinMode(WINDOW_LED, OUTPUT);

  while (!Serial)
    ;
  comm.setup();
}

void loop()
{
  comm.handle_mqtt_loop();

  blinds_1_state = digitalRead(BLINDS_1_PIN);
  blinds_2_state = digitalRead(BLINDS_2_PIN);
  window_state = digitalRead(WINDOW_PIN);

  data.clear();

  data["time"] = millis();

  if (blinds_1_state == HIGH)
  {
    data["blinds_1"] = "open";
    digitalWrite(BLINDS_1_LED, HIGH);
  }
  else
  {
    data["blinds_1"] = "closed";
    digitalWrite(BLINDS_1_LED, LOW);
  }

  if (blinds_2_state == HIGH)
  {
    data["blinds_2"] = "open";
    digitalWrite(BLINED_2_LED, HIGH);
  }
  else
  {
    data["blinds_2"] = "closed";
    digitalWrite(BLINED_2_LED, LOW);
  }

  if (window_state == HIGH)
  {
    data["window"] = "open";
    digitalWrite(WINDOW_LED, HIGH);
  }
  else
  {
    data["window"] = "closed";
    digitalWrite(WINDOW_LED, LOW);
  }

  String output;
  serializeJson(data, output);
  comm.publish("window_sensor", output);
  delay(5000);
}