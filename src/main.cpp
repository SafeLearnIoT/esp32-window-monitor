#include <Arduino.h>

#include "env.h"
#include "rtpnn.h"
#include "communication.h"
#include "sd_card.h"

#define BLINDS_1_PIN 0
#define BLINDS_2_PIN 26
#define WINDOW_PIN 25

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

rTPNN::SDP<int> blinds_1_sdp(rTPNN::SDPType::Blinds);
rTPNN::SDP<int> blinds_2_sdp(rTPNN::SDPType::Blinds);
rTPNN::SDP<int> window_sdp(rTPNN::SDPType::Window);

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

    blinds_1_state = digitalRead(BLINDS_1_PIN);
    blinds_2_state = digitalRead(BLINDS_2_PIN);
    window_state = digitalRead(WINDOW_PIN);

    JsonDocument sensor_data;
    sensor_data["time"] = comm->get_rawtime();
    sensor_data["device"] = comm->get_client_id();
    JsonObject detail_sensor_data = sensor_data["data"].to<JsonObject>();
    detail_sensor_data["blinds_1"] = blinds_1_state;
    detail_sensor_data["blinds_2"] = blinds_2_state;
    detail_sensor_data["window"] = window_state;

    JsonDocument rtpnn_data;
    rtpnn_data["time"] = comm->get_rawtime();
    rtpnn_data["device"] = comm->get_client_id();
    rtpnn_data["ml_algo"] = "rtpnn";
    JsonObject detail_rtpnn_data = rtpnn_data["data"].to<JsonObject>();

    JsonObject blinds_1_data = detail_rtpnn_data["blinds_1"].to<JsonObject>();
    auto blinds_1_calc = blinds_1_sdp.execute_sdp(blinds_1_state);
    blinds_1_data["trend"] = blinds_1_calc.first;
    blinds_1_data["level"] = blinds_1_calc.second;

    JsonObject blinds_2_data = detail_rtpnn_data["blinds_2"].to<JsonObject>();
    auto blinds_2_calc = blinds_2_sdp.execute_sdp(blinds_2_state);
    blinds_2_data["trend"] = blinds_2_calc.first;
    blinds_2_data["level"] = blinds_2_calc.second;

    JsonObject window_data = detail_rtpnn_data["window"].to<JsonObject>();
    auto window_calc = window_sdp.execute_sdp(window_state);
    window_data["trend"] = window_calc.first;
    window_data["level"] = window_calc.second;

    JsonDocument reglin_data;

    comm->send_data(sensor_data, rtpnn_data, reglin_data);
  }

  if (millis() - lastHealthPing > 900000)
  {
    lastHealthPing = millis();
    comm->resume_communication();
    delay(5000);
    comm->pause_communication();
  }
}