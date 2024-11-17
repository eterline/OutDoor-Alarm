#include <ESP8266WiFi.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <PubSubClient.h>

/* =============== Define network data =============== */

// Network config for IEEE 802.11
#define WIFI_ID "ROYTER"
#define WIFI_PASSWORD "tizaebal84"

// Home MQTT client config
#define MQTT_USER "test0"
#define MQTT_PASSWORD "test"

#define TIMEOUT_DELAY_SEC 10

/* =============== Define sensors data =============== */

// OneWire parameters (DS18B20)
#define ONE_WIRE_BUS 0

// Alarm button pin
#define ALARM_BUS 2
#define ALARM_DELAY 50

/* =============== MQTT settings =============== */

#define MQTT_BROKER "docker.lan"
#define MQTT_PORT 1883

const char *temp_topic  = "middle_out/temp";
const char *alarm_topic  = "middle_out/alarm";
// unsigned long timer_mqtt;

/* =============== Define objects for libs =============== */

WiFiClient netClient;
PubSubClient mqtt(MQTT_BROKER, MQTT_PORT, netClient); // MQTT Client obj

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire); // DS18B20 measure

/* =============== functions =============== */

void connect_wifi(char *ssid, char *pwd) {
  pinMode(BUILTIN_LED, OUTPUT);

  Serial.print("Connecting to WIFI: ");
  Serial.print(ssid);

  while (WiFi.begin(ssid, pwd) != WL_CONNECTED) {
    Serial.print(".");
    digitalWrite(BUILTIN_LED, 1);
    delay(5000);
    digitalWrite(BUILTIN_LED, 0);
  }

  Serial.print("\nConnected!\nnetwork IP: ");
  Serial.println(WiFi.localIP());
}

void connect_mqtt(char *user, char *pass) {
  Serial.println("Attempting MQTT connection...");

  while (!mqtt.connected()) {
    Serial.print(".");
    if (mqtt.connect("1", user, pass)) {
      Serial.println("\nMQTT connected");
    }
    delay(1000);
  }
}

void get_temp() {
  char buffer[64];
  float temp = sensors.getTempCByIndex(0);

  snprintf(buffer, sizeof buffer, "%.1f", temp);

  mqtt.publish(temp_topic, buffer);
  Serial.print(buffer); Serial.println("°C");
}

volatile bool btnPressed = false;
unsigned long lastButtonPressTime = 0;

void ICACHE_RAM_ATTR btnIsr() {
  unsigned long currentMillis = millis();

  if (currentMillis - lastButtonPressTime > ALARM_DELAY * 1000) {
    btnPressed = true;
    lastButtonPressTime = currentMillis;
    mqtt.publish(alarm_topic, "ON");
  }
}

unsigned long previousTempMillis = 0;

/* =============== main =============== */

void setup() {
  Serial.begin(9200);

  connect_wifi(WIFI_ID, WIFI_PASSWORD);
  connect_mqtt(MQTT_USER, MQTT_PASSWORD);

  pinMode(ALARM_BUS, INPUT_PULLUP);
  attachInterrupt(ALARM_BUS, btnIsr, FALLING);

  sensors.requestTemperatures();
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    if (mqtt.connected()) {
      unsigned long currentMillis = millis();

      if (btnPressed) {
        btnPressed = false;
        Serial.println("pressed");
        delay(5000);
        mqtt.publish(alarm_topic, "OFF");
      }

      if (currentMillis - previousTempMillis >= 5000) {
        previousTempMillis = currentMillis;
        get_temp();
        sensors.requestTemperatures();
      }
    } else {
        connect_mqtt(MQTT_USER, MQTT_PASSWORD);
    }
  } else {
    connect_wifi(WIFI_ID, WIFI_PASSWORD);
  }
}