#include <ArduinoJson.hpp>
#include <credentials.h>
#include <EspMQTTClient.h>
#include <MqttKalmanPublish.h>
#include <vector>
#include <WebSocketsClient.h>

using namespace ArduinoJson;

#include "matrix-neomatrix.h"

#define CLIENT_NAME "espMatrix-etVertical"
const bool MQTT_RETAINED = true;

EspMQTTClient mqttClient(
	WIFI_SSID,
	WIFI_PASSWORD,
	MQTT_SERVER,
	MQTT_USERNAME,
	MQTT_PASSWORD,
	CLIENT_NAME,
	1883);

WebSocketsClient ws;

// #define PRINT_TO_SERIAL

#define BASE_TOPIC CLIENT_NAME "/"
#define BASE_TOPIC_SET BASE_TOPIC "set/"
#define BASE_TOPIC_STATUS BASE_TOPIC "status/"

#ifdef ESP8266
	#define LED_BUILTIN_ON LOW
	#define LED_BUILTIN_OFF HIGH
#else // for ESP32
	#define LED_BUILTIN_ON HIGH
	#define LED_BUILTIN_OFF LOW
#endif

MQTTKalmanPublish mkCommandsPerSecond(mqttClient, BASE_TOPIC_STATUS "commands-per-second", false, 30 /* every 30 sec */, 10);
MQTTKalmanPublish mkRssi(mqttClient, BASE_TOPIC_STATUS "rssi", MQTT_RETAINED, 12 * 5 /* every 5 min */, 10);

boolean on = true;
uint8_t mqttBri = 2;
uint8_t lastConnected = 0;

uint32_t commands = 0;
size_t lastPublishedClientAmount = 0;

void testMatrix()
{
	Serial.println("Fill screen: RED");
	matrix_fill(255, 0, 0);
	matrix_update();
	delay(250);

	Serial.println("Fill screen: GREEN");
	matrix_fill(0, 255, 0);
	matrix_update();
	delay(250);

	Serial.println("Fill screen: BLUE");
	matrix_fill(0, 0, 255);
	matrix_update();
	delay(250);

	Serial.println("Pixel: top right and bottom left, top left shows an arrow pointing up");
	matrix_fill(0, 0, 0);
	matrix_pixel(TOTAL_WIDTH - 2, 1, 255, 255, 255);
	matrix_pixel(1, TOTAL_HEIGHT - 2, 255, 255, 255);

	matrix_pixel(3, 1, 255, 255, 255);
	matrix_pixel(3, 2, 255, 255, 255);
	matrix_pixel(3, 3, 255, 255, 255);
	matrix_pixel(3, 4, 255, 255, 255);
	matrix_pixel(3, 5, 255, 255, 255);
	matrix_pixel(2, 2, 255, 255, 255);
	matrix_pixel(4, 2, 255, 255, 255);
	matrix_pixel(1, 3, 255, 255, 255);
	matrix_pixel(5, 3, 255, 255, 255);

	matrix_update();
	delay(1000);
}

void webSocketEvent(WStype_t type, uint8_t *payload, size_t length)
{
	commands += 1;

	switch (type)
	{
	case WStype_DISCONNECTED:
		Serial.printf("[WSc] Disconnected!\n");
		break;
	case WStype_CONNECTED:
		Serial.printf("[WSc] Connected to url: %s\n", payload);
		break;
	case WStype_TEXT:
	{
#ifdef PRINT_TO_SERIAL
		Serial.printf("[WSc] get text: %s\n", payload);
#endif
		const size_t CAPACITY = JSON_OBJECT_SIZE(6);
		static StaticJsonDocument<CAPACITY> doc;
		deserializeJson(doc, payload);
		JsonObject object = doc.as<JsonObject>();

		matrix_pixel(object["x"], object["y"], object["r"], object["g"], object["b"]);

		size_t clients = object["clients"];
		if (lastPublishedClientAmount != clients)
		{
			bool success = mqttClient.publish(BASE_TOPIC_STATUS "clients", String(clients), MQTT_RETAINED);
			if (success)
			{
				lastPublishedClientAmount = clients;
			}
		}
	}
	break;
	case WStype_BIN:
#ifdef PRINT_TO_SERIAL
		Serial.printf("[WSc] get binary length: %u\n", length);
#endif
		break;
	default:
		break;
	}
}

void setup()
{
	pinMode(LED_BUILTIN, OUTPUT);
	Serial.begin(115200);
	Serial.println();

	// ws.begin("dev-device", 8080, "/ws");
	ws.beginSSL("ledmatrix.edjopato.de", 443, "/ws");
	ws.onEvent(webSocketEvent);
	ws.setReconnectInterval(5000);
	// every ms, timeout ms, failed after n times
	ws.enableHeartbeat(15000, 3000, 2);

	matrix_setup(mqttBri);

#ifdef PRINT_TO_SERIAL
	mqttClient.enableDebuggingMessages();
#endif
	mqttClient.enableHTTPWebUpdater();
	mqttClient.enableOTA();
	mqttClient.enableLastWillMessage(BASE_TOPIC "connected", "0", MQTT_RETAINED);

	// well, hope we are OK, let's draw some colors first :)
	// testMatrix();

	matrix_fill(0, 0, 0);
	matrix_update();

	Serial.println("Setup done...");
}

void onConnectionEstablished()
{
	mqttClient.subscribe(BASE_TOPIC_SET "bri", [](const String &payload) {
		int value = strtol(payload.c_str(), 0, 10);
		mqttBri = max(1, min(255, value));
		matrix_brightness(mqttBri * on);
		mqttClient.publish(BASE_TOPIC_STATUS "bri", String(mqttBri), MQTT_RETAINED);
	});

	mqttClient.subscribe(BASE_TOPIC_SET "on", [](const String &payload) {
		boolean value = payload != "0";
		on = value;
		matrix_brightness(mqttBri * on);
		mqttClient.publish(BASE_TOPIC_STATUS "on", String(on), MQTT_RETAINED);
	});

	mqttClient.publish(BASE_TOPIC_STATUS "bri", String(mqttBri), MQTT_RETAINED);
	mqttClient.publish(BASE_TOPIC_STATUS "on", String(on), MQTT_RETAINED);
	mqttClient.publish(BASE_TOPIC "git-version", GIT_VERSION, MQTT_RETAINED);
	mqttClient.publish(BASE_TOPIC "connected", "1", MQTT_RETAINED);
	lastConnected = 1;
	lastPublishedClientAmount = 0;
}

void loop()
{
	mqttClient.loop();
	digitalWrite(LED_BUILTIN, mqttClient.isConnected() ? LED_BUILTIN_OFF : LED_BUILTIN_ON);
	if (!mqttClient.isWifiConnected())
	{
		return;
	}

	ws.loop();

	auto nextConnected = ws.isConnected() ? 2 : 1;
	if (nextConnected != lastConnected && mqttClient.isConnected())
	{
		if (mqttClient.publish(BASE_TOPIC "connected", String(nextConnected), MQTT_RETAINED))
		{
			lastConnected = nextConnected;
		}
	}

	auto now = millis();

	static unsigned long nextCommandsUpdate = 0;
	if (now >= nextCommandsUpdate)
	{
		nextCommandsUpdate = now + 1000;
		float avgCps = mkCommandsPerSecond.addMeasurement(commands);
#ifdef PRINT_TO_SERIAL
		Serial.printf("Commands  per Second: %8d    Average: %10.2f\n", commands, avgCps);
#endif

		commands = 0;
	}

	static unsigned long nextMeasure = 0;
	if (now >= nextMeasure)
	{
		nextMeasure = now + 5000;
		long rssi = WiFi.RSSI();
		float avgRssi = mkRssi.addMeasurement(rssi);
#ifdef PRINT_TO_SERIAL
		Serial.printf("RSSI          in dBm: %8ld    Average: %10.2f\n", rssi, avgRssi);
#endif
	}

	// 50 ms -> 20 FPS
	// 33 ms -> 30 FPS
	// 25 ms -> 40 FPS
	// 20 ms -> 50 FPS
	// 16 ms -> 62.2 FPS
	static unsigned long nextMatrix = 0;
	if (now >= nextMatrix)
	{
		nextMatrix = now + 33;
		matrix_update();
	}
}
