#include <Arduino.h>

#include "jsonToArray.h"
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

bool get_weights = false;
bool set_weights = false;
String payload_content;

void callback(String &topic, String &payload)
{
  if (topic == "configuration")
  {
    comm->initConfig(payload);
    return;
  }
  Serial.println("[SUB][" + topic + "] " + payload);
  if (payload == "get_weights")
  {
    if (!get_weights)
    {
      get_weights = true;
    }
    return;
  }
  if (payload.startsWith("set_weights;"))
  {
    if (!set_weights)
    {
      payload_content = payload;
      set_weights = true;
    }
    return;
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

  delay(5000);
}

void loop()
{
  if (comm->is_system_configured())
  {
    if (get_weights)
    {
      comm->hold_connection();

      JsonDocument doc;
      blinds_1_ml.get_weights(doc);
      blinds_2_ml.get_weights(doc);
      window_ml.get_weights(doc);
      comm->publish("cmd_mcu", "weights;" + doc.as<String>());
      get_weights = false;
    }
    if (set_weights)
    {
      JsonDocument doc;
      if (payload_content.substring(12) == "ok")
      {
        comm->publish("cmd_gateway", "", true);
        comm->release_connection();
        payload_content.clear();
        set_weights = false;
        return;
      }
      deserializeJson(doc, payload_content.substring(12));

      auto blinds_1_weights = Converter<std::array<double, 4>>::fromJson(doc["blinds_1"]);
      Serial.print("[Blinds 1]");
      blinds_1_ml.set_weights(blinds_1_weights);

      auto blinds_2_weights = Converter<std::array<double, 4>>::fromJson(doc["blinds_2"]);
      Serial.print("[Blinds 2]");
      blinds_2_ml.set_weights(blinds_2_weights);

      auto window_weights = Converter<std::array<double, 4>>::fromJson(doc["window"]);
      Serial.print("[Window]");
      window_ml.set_weights(window_weights);

      // Clear message
      comm->publish("cmd_gateway", "", true);

      comm->release_connection();
      payload_content.clear();
      set_weights = false;
    }

    comm->handle_mqtt_loop();
    if (millis() - lastStateCheck > 900000)
    {
      comm->resume_communication();

      lastStateCheck = millis();
      auto raw_time = comm->get_rawtime();

      blinds_1_state = digitalRead(BLINDS_1_PIN);
      blinds_2_state = digitalRead(BLINDS_2_PIN);
      window_state = digitalRead(WINDOW_PIN);

      if (comm->m_configuration->getSendSensorData())
      {
        JsonDocument sensor_data;
        sensor_data["time_sent"] = raw_time;
        sensor_data["device"] = comm->get_client_id();
        JsonObject detail_sensor_data = sensor_data["data"].to<JsonObject>();
        detail_sensor_data["test_name"] = comm->m_configuration->getTestName();
        detail_sensor_data["blinds_1"] = blinds_1_state;
        detail_sensor_data["blinds_2"] = blinds_2_state;
        detail_sensor_data["window"] = window_state;

        comm->send_data(sensor_data);
      }
      
      if (comm->m_configuration->getRunMachineLearning())
      {
        JsonDocument ml_data;
        ml_data["time_sent"] = raw_time;
        ml_data["device"] = comm->get_client_id();

        JsonObject detail_ml_data = ml_data["data"].to<JsonObject>();
        detail_ml_data["test_name"] = comm->m_configuration->getTestName();

        JsonObject blinds_1_data = detail_ml_data["blinds_1"].to<JsonObject>();
        JsonObject blinds_2_data = detail_ml_data["blinds_2"].to<JsonObject>();
        JsonObject window_data = detail_ml_data["window"].to<JsonObject>();
        blinds_1_ml.perform(blinds_1_state, blinds_1_data);
        blinds_2_ml.perform(blinds_2_state, blinds_2_data);
        window_ml.perform(window_state, window_data);

        comm->send_ml(ml_data);
      }
    }
  }
  else
  {
    Serial.println("System not configured");
    comm->handle_mqtt_loop();
    delay(1000);
  }
}