#include <Arduino.h>

#include "env.h"
#include "ml.h"
#include "communication.h"

#define BLINDS_1_PIN 0
#define BLINDS_2_PIN 26
#define WINDOW_PIN 25

auto ml_algo = MLAlgo::LogReg;
auto blinds_1_ml = ML(Blinds, ml_algo, "blinds_1");
auto blinds_2_ml = ML(Blinds, ml_algo, "blinds_2");
auto window_ml = ML(Window, ml_algo, "window");

void callback(String &topic, String &payload)
{
  Serial.print("Topic: ");
  Serial.print(topic);
  Serial.print(", Payload: ");
  Serial.println(payload);
}

int blinds_1_state = -1;
int blinds_2_state = -1;
int window_state = -1;
unsigned long lastStateCheck = 0;
unsigned long lastHealthPing = 0;

auto comm = Communication::get_instance(SSID_ENV, PASSWORD_ENV, "esp32/window", MQTT_HOST_ENV, MQTT_PORT_ENV, callback);

void setup()
{
  Serial.begin(115200);

  pinMode(BLINDS_1_PIN, INPUT_PULLUP);
  pinMode(BLINDS_2_PIN, INPUT_PULLUP);
  pinMode(WINDOW_PIN, INPUT_PULLUP);

  comm->setup();
  delay(5000);
  comm->pause_communication();
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
    auto time_struct = localtime(&raw_time);

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

    JsonDocument ml_data;
    if (ml_algo != MLAlgo::None)
    {
      ml_data["time_sent"] = raw_time;
      ml_data["device"] = comm->get_client_id();
      ml_data["ml_algo"] = algoString[ml_algo];
      JsonObject data = ml_data["data"].to<JsonObject>();

      blinds_1_ml.perform(*time_struct, blinds_1_state, data);
      blinds_2_ml.perform(*time_struct, blinds_2_state, data);
      window_ml.perform(*time_struct, window_state, data);
    }
    comm->send_data(sensor_data, ml_data);
  }

  if (millis() - lastHealthPing > 900000)
  {
    lastHealthPing = millis();
    comm->resume_communication();
    delay(5000);
    comm->pause_communication();
  }
}