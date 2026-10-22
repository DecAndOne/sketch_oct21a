#include <WiFi.h>
#include <PubSubClient.h>
#include <math.h>
#include "DHT.h"

#define DHTPIN 4
#define DHTTYPE DHT11

const char* WIFI_SSID = "十三月的P90";
const char* WIFI_PASSWORD = "18931202212";

const char* MQTT_SERVER = "idc4a91d.ala.eu-central-1.emqxsl.com";
const uint16_t MQTT_PORT = 1883;
const char* MQTT_USERNAME = "ccrright";
const char* MQTT_PASSWORD = "071203";
const char* MQTT_CLIENT_ID = "dht11-esp32-01";
const char* MQTT_TOPIC = "YOUR_MQTT_TOPIC";

DHT dht(DHTPIN, DHTTYPE);
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

unsigned long lastSampleMs = 0;
unsigned long lastWifiAttemptMs = 0;
unsigned long lastMqttAttemptMs = 0;

void maintainConnections() {
	const unsigned long now = millis();

	if (WiFi.status() != WL_CONNECTED) {
		if (mqttClient.connected()) {
			mqttClient.disconnect();
		}

		if (now - lastWifiAttemptMs >= 10000) {
			lastWifiAttemptMs = now;
			Serial.println("WiFi disconnected; reconnecting");
			WiFi.disconnect();
			WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
		}
		return;
	}

	if (!mqttClient.connected() && now - lastMqttAttemptMs >= 5000) {
		lastMqttAttemptMs = now;
		Serial.print("Connecting to MQTT... ");
		if (mqttClient.connect(MQTT_CLIENT_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
			Serial.println("connected");
		} else {
			Serial.print("failed, state=");
			Serial.println(mqttClient.state());
		}
	}
}

void setup() {
	Serial.begin(115200);
	dht.begin();

	WiFi.mode(WIFI_STA);
	WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
	lastWifiAttemptMs = millis();

	mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
	mqttClient.setSocketTimeout(2);
}

void loop() {
	maintainConnections();
	mqttClient.loop();

	const unsigned long now = millis();
	if (now - lastSampleMs >= 2000) {
		lastSampleMs = now;

		const float humidity = dht.readHumidity();
		const float temperature = dht.readTemperature();

		if (isnan(humidity) || isnan(temperature)) {
			Serial.println("ERROR,DHT_READ_FAILED");
		} else {
			Serial.print(temperature, 1);
			Serial.print(',');
			Serial.println(humidity, 1);

			if (mqttClient.connected()) {
				char payload[80];
				snprintf(payload, sizeof(payload),
								 "{\"temperature\":%.1f,\"humidity\":%.1f}",
								 temperature, humidity);
				if (!mqttClient.publish(MQTT_TOPIC, payload)) {
					Serial.println("ERROR,MQTT_PUBLISH_FAILED");
				}
			}
		}
	}

	delay(10);
}
