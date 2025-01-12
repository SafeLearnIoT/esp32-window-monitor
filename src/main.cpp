#include <Arduino.h>

#include "env.h"
#include "ml.h"
#include "communication.h"

#define BLINDS_1_PIN 0
#define BLINDS_2_PIN 26
#define WINDOW_PIN 25

auto blinds_1_ml = ML(Blinds, "blinds_1");
auto blinds_2_ml = ML(Blinds, "blinds_2");
auto window_ml = ML(Window, "window");

void callback(String &topic, String &payload);

auto comm = Communication::get_instance(SSID_ENV, PASSWORD_ENV, "esp32/window", MQTT_HOST_ENV, MQTT_PORT_ENV, callback);

void callback(String &topic, String &payload)
{
    Serial.println("[SUB][" + topic + "] " + payload);
    if(payload == "get_weights")
    {
        comm->hold_connection();
        comm->publish("cmd", "", true);
        auto weights = blinds_1_ml.get_weights();
        serializeJson(weights, Serial);
        Serial.println();
    }
    if(payload=="set_weights")
    {
        comm->release_connection();
    }
}

int blinds_1_state = -1;
int blinds_2_state = -1;
int window_state = -1;
unsigned long lastStateCheck = 0;
unsigned long lastHealthPing = 0;


void setup()
{
  Serial.begin(115200);

  pinMode(BLINDS_1_PIN, INPUT_PULLUP);
  pinMode(BLINDS_2_PIN, INPUT_PULLUP);
  pinMode(WINDOW_PIN, INPUT_PULLUP);

  comm->setup();
}

void loop()
{
  if (millis() - lastStateCheck > 60000)
  {
    lastStateCheck = millis();
    if (blinds_1_state == digitalRead(BLINDS_1_PIN) && blinds_2_state == digitalRead(BLINDS_2_PIN) && window_state == digitalRead(WINDOW_PIN))
    {
      return;
    }

    auto raw_time = comm->get_rawtime();

    blinds_1_state = digitalRead(BLINDS_1_PIN);
    blinds_2_state = digitalRead(BLINDS_2_PIN);
    window_state = digitalRead(WINDOW_PIN);

    JsonDocument sensor_data;
    sensor_data["time_sent"] = raw_time;
    sensor_data["device"] = comm->get_client_id();
    JsonObject detail_sensor_data = sensor_data["data"].to<JsonObject>();
    detail_sensor_data["blinds_1"] = blinds_1_state;
    detail_sensor_data["blinds_2"] = blinds_2_state;
    detail_sensor_data["window"] = window_state;

   
    blinds_1_ml.perform(blinds_1_state);
    blinds_2_ml.perform(blinds_2_state);
    window_ml.perform(window_state);
    
    serializeJson(sensor_data, Serial);
    Serial.println();
  
    comm->send_data(sensor_data);
  }

  if (millis() - lastHealthPing > 900000)
  {
    lastHealthPing = millis();
    comm->resume_communication();
    delay(5000);
    comm->pause_communication();
  }
}