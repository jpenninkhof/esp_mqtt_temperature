/*
 *  Example of working sensor DHT22 (temperature and humidity) and send data to MQTT
 *
 *  For a single device, connect as follows:
 *  DHT22 1 (Vcc) to Vcc (3.3 Volts)
 *  DHT22 2 (DATA_OUT) to ESP Pin GPIO2
 *  DHT22 3 (NC)
 *  DHT22 4 (GND) to GND
 *
 *  Between Vcc and DATA_OUT needs to connect a pull-up resistor of 10 kOhm.
 *
 *  (c) 2015 by Mikhail Grigorev <sleuthhound@gmail.com>
 *
 */
#include "ets_sys.h"
#include "driver/uart.h"
#include "driver/dht22.h"
#include "osapi.h"
#include "mqtt.h"
#include "wifi.h"
#include "config.h"
#include "debug.h"
#include "user_interface.h"
#include "mem.h"

MQTT_Client mqttClient;
LOCAL os_timer_t dhtTimer;

void wifi_connect_cb(uint8_t status)
{
	if(status == STATION_GOT_IP){
		MQTT_Connect(&mqttClient);
	} else {
		MQTT_Disconnect(&mqttClient);
	}
}
void mqtt_connected_cb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
	INFO("MQTT: Connected\r\n");
}

void mqtt_disconnected_cb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
	INFO("MQTT: Disconnected\r\n");
}

void mqtt_published_cb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
	INFO("MQTT: Published\r\n");
}

void mqtt_data_cb(uint32_t *args, const char* topic, uint32_t topic_len, const char *data, uint32_t data_len)
{
	char *topicBuf = (char*)os_zalloc(topic_len+1),
			*dataBuf = (char*)os_zalloc(data_len+1);

	MQTT_Client* client = (MQTT_Client*)args;

	os_memcpy(topicBuf, topic, topic_len);
	topicBuf[topic_len] = 0;

	os_memcpy(dataBuf, data, data_len);
	dataBuf[data_len] = 0;

	INFO("Receive topic: %s, data: %s \r\n", topicBuf, dataBuf);
	os_free(topicBuf);
	os_free(dataBuf);
}

LOCAL void ICACHE_FLASH_ATTR dhtCb(void *arg)
{
	static char data[256];
	static char temp[10];
	static char hum[10];
	uint8_t status;
	os_timer_disarm(&dhtTimer);
	struct dht_sensor_data* r = DHTRead();
	float curTemp = r->temperature;
	float curHum = r->humidity;
	static float lastTemp;
	static float lastHum;
	uint8_t topic[32];
	if(r->success)
	{
		os_sprintf(temp, "%d.%d",(int)(curTemp),(int)((curTemp - (int)curTemp)*100));
		os_sprintf(hum, "%d.%d",(int)(curHum),(int)((curHum - (int)curHum)*100));
		INFO("Temperature: %s *C, Humidity: %s %%\r\n", temp, hum);
		if (mqttClient.connState == MQTT_DATA && lastTemp != curTemp) {
			os_sprintf(topic, "%s%s", config.mqtt_topic, "temperature");
			MQTT_Publish(&mqttClient, topic, temp, strlen(temp), 0, 0);
			lastTemp = curTemp;
		}
		if (mqttClient.connState == MQTT_DATA && lastHum != curHum) {
			os_sprintf(topic, "%s%s", config.mqtt_topic, "humidity");
			MQTT_Publish(&mqttClient, topic, hum, strlen(hum), 0, 0);
			lastHum = curHum;
		}
	}
	else
	{
		INFO("Error reading temperature and humidity.\r\n");
	}
	os_timer_setfn(&dhtTimer, (os_timer_func_t *)dhtCb, (void *)0);
	os_timer_arm(&dhtTimer, DELAY, 1);
}

void user_init(void)
{
	uart_init(BIT_RATE_115200, BIT_RATE_115200);
	os_delay_us(1000000);

	config_load();

	DHTInit(DHT22);

	MQTT_InitConnection(&mqttClient, config.mqtt_host, config.mqtt_port, config.security);
	MQTT_InitClient(&mqttClient, config.device_id, config.mqtt_user, config.mqtt_pass, config.mqtt_keepalive, 1);
	MQTT_InitLWT(&mqttClient, "/lwt", "offline", 0, 0);
	MQTT_OnConnected(&mqttClient, mqtt_connected_cb);
	MQTT_OnDisconnected(&mqttClient, mqtt_disconnected_cb);
	MQTT_OnPublished(&mqttClient, mqtt_published_cb);
	MQTT_OnData(&mqttClient, mqtt_data_cb);

	WIFI_Connect(config.sta_ssid, config.sta_pwd, wifi_connect_cb);

	os_timer_disarm(&dhtTimer);
	os_timer_setfn(&dhtTimer, (os_timer_func_t *)dhtCb, (void *)0);
	os_timer_arm(&dhtTimer, DELAY, 1);

	INFO("\r\nSystem started ...\r\n");
}

void user_rf_pre_init(void) {}
