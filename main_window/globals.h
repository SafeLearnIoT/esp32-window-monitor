#include "env.h"
#include "WiFi.h"
#include "MQTT.h"

#define TYPE "window_sensor"

const char ssid[] = SSID_ENV;
const char pass[] = PASSWORD_ENV;

const char mqtt_host[] = MQTT_HOST_ENV;
const int mqtt_port = MQTT_PORT_ENV;

MQTTClient mqtt_client;
WiFiClient wifi_client;
