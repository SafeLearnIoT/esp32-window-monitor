#include <Arduino.h>

#include "env.h"
#include "ml.h"
#include "communication.h"

#define BLINDS_1_PIN 0
#define BLINDS_2_PIN 26
#define WINDOW_PIN 25

ML *blinds_1_ml;
ML *blinds_2_ml;
ML *window_ml;

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

auto comm = Communication::get_instance(SSID_ENV, PASSWORD_ENV, "esp32/window/", MQTT_HOST_ENV, MQTT_PORT_ENV, callback);

void setup()
{
  Serial.begin(115200);

  pinMode(BLINDS_1_PIN, INPUT_PULLUP);
  pinMode(BLINDS_2_PIN, INPUT_PULLUP);
  pinMode(WINDOW_PIN, INPUT_PULLUP);

  comm->setup();
  delay(5000);
  comm->pause_communication();

  switch (comm->get_ml_algo())
  {
  case MLAlgo::LinReg:
    blinds_1_ml = new Regression::Linear(SensorType::Blinds);
    blinds_2_ml = new Regression::Linear(SensorType::Blinds);
    window_ml = new Regression::Linear(SensorType::Window);
    break;
  case MLAlgo::LogReg:
    blinds_1_ml = new Regression::Logistic(SensorType::Blinds);
    blinds_2_ml = new Regression::Logistic(SensorType::Blinds);
    window_ml = new Regression::Logistic(SensorType::Window);
    break;
  case MLAlgo::rTPNN:
    blinds_1_ml = new RTPNN::SDP(SensorType::Blinds);
    blinds_2_ml = new RTPNN::SDP(SensorType::Blinds);
    window_ml = new RTPNN::SDP(SensorType::Window);
    break;
  case MLAlgo::None:
    break;
  }
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
    sensor_data["time"] = raw_time;
    sensor_data["device"] = comm->get_client_id();
    JsonObject detail_sensor_data = sensor_data["data"].to<JsonObject>();
    detail_sensor_data["blinds_1"] = blinds_1_state;
    detail_sensor_data["blinds_2"] = blinds_2_state;
    detail_sensor_data["window"] = window_state;

    JsonDocument ml_data;
    if (comm->get_ml_algo() != MLAlgo::None)
    {
      ml_data["time"] = raw_time;
      ml_data["device"] = comm->get_client_id();
      ml_data["ml_algo"] = "rtpnn";
      JsonObject detail_rtpnn_data = ml_data["data"].to<JsonObject>();

      JsonObject blinds_1_data = detail_rtpnn_data["blinds_1"].to<JsonObject>();
      blinds_1_data = blinds_1_ml->perform(*time_struct, blinds_1_state);

      JsonObject blinds_2_data = detail_rtpnn_data["blinds_2"].to<JsonObject>();
      blinds_2_data = blinds_2_ml->perform(*time_struct, blinds_2_state);

      JsonObject window_data = detail_rtpnn_data["window"].to<JsonObject>();
      window_data = window_ml->perform(*time_struct, window_state);
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