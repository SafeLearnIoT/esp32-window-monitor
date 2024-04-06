#define IGNORE_DMG

#include <Arduino.h>

#include "env.h"
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
unsigned long lastDataUploadMillis = 0;

auto comm = Communication::get_instance(SSID_ENV, PASSWORD_ENV, "esp32/window/", MQTT_HOST_ENV, MQTT_PORT_ENV, callback);

bool upload_init = false;
String header = "timestamp,blinds_1_state,blinds_2_state,window_state\n";

String get_todays_file_path()
{
  return "/" + comm->get_todays_date_string() + ".csv";
};

String get_yesterdays_file_path()
{
  return "/" + comm->get_yesterdays_date_string() + ".csv";
};

String output = "";

void read_data()
{
  if (blinds_1_state == digitalRead(BLINDS_1_PIN) && blinds_2_state == digitalRead(BLINDS_2_PIN) && window_state == digitalRead(WINDOW_PIN))
  {
    return;
  }

  blinds_1_state = digitalRead(BLINDS_1_PIN);
  blinds_2_state = digitalRead(BLINDS_2_PIN);
  window_state = digitalRead(WINDOW_PIN);

  output = "";
  output += String(comm->get_rawtime()) + ",";
  output += (blinds_1_state == HIGH) ? "1," : "0,";
  output += (blinds_2_state == HIGH) ? "1," : "0,";
  output += (window_state == HIGH) ? "1\n" : "0\n";

  if (!file_exists(SD, get_todays_file_path().c_str()))
  {
    write_file(SD, get_todays_file_path().c_str(), header.c_str());
  }
  append_file(SD, get_todays_file_path().c_str(), output.c_str());
}

void setup()
{
  Serial.begin(115200);

  if (!SD.begin(14))
  {
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();

  if (cardType == CARD_NONE)
  {
    Serial.println("No SD card attached");
    return;
  }

  Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC)
  {
    Serial.println("MMC");
  }
  else if (cardType == CARD_SD)
  {
    Serial.println("SDSC");
  }
  else if (cardType == CARD_SDHC)
  {
    Serial.println("SDHC");
  }
  else
  {
    Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);

  pinMode(BLINDS_1_PIN, INPUT_PULLUP);
  pinMode(BLINDS_2_PIN, INPUT_PULLUP);
  pinMode(WINDOW_PIN, INPUT_PULLUP);

  comm->setup();
  delay(2000);
}

void loop()
{
  comm->handle_mqtt_loop();
  read_data();
  auto current_time = comm->get_localtime();
  if (current_time->tm_hour == 1 && !upload_init)
  {
    upload_init = true;
    comm->publish("data", read_file(SD, get_yesterdays_file_path().c_str()));
  }
  if (current_time->tm_hour == 2 && upload_init)
  {
    list_dir(SD, "/", 0);
    upload_init = false;
    checkAndCleanFileSystem(SD);
  }

  // if (millis() - lastDataUploadMillis > 60000)
  // {
  //   lastDataUploadMillis = millis();
  //   if (file_exists(SD, get_todays_file_path().c_str()))
  //   {
  //     comm->publish("current_data", read_file(SD, get_todays_file_path().c_str()));
  //   }
  // }
}