/* main.c -- MQTT client example
*
* Copyright (c) 2014-2015, Tuan PM <tuanpm at live dot com>
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* * Redistributions of source code must retain the above copyright notice,
* this list of conditions and the following disclaimer.
* * Redistributions in binary form must reproduce the above copyright
* notice, this list of conditions and the following disclaimer in the
* documentation and/or other materials provided with the distribution.
* * Neither the name of Redis nor the names of its contributors may be used
* to endorse or promote products derived from this software without
* specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*/
#include "ets_sys.h"
#include "driver/uart.h"
#include "driver/dht22.h"
#include "driver/gpio16.h"
#include "osapi.h"
#include "mqtt.h"
#include "wifi.h"
#include "config.h"
#include "debug.h"
#include "user_interface.h"
#include "mem.h"

MQTT_Client mqttClient;
LOCAL os_timer_t dhtTimer;
extern uint8_t pin_num[GPIO_PIN_NUM];
DHT_Sensor sensor;

void wifiConnectCb(uint8_t status)
{
	if(status == STATION_GOT_IP){
		MQTT_Connect(&mqttClient);
	} else {
		MQTT_Disconnect(&mqttClient);
	}
}
void mqttConnectedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
	INFO("MQTT: Connected\r\n");
	MQTT_Publish(client, "/mqtt/topic/0", "hello0", 6, 0, 0);
}

void mqttDisconnectedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
	INFO("MQTT: Disconnected\r\n");
}

void mqttPublishedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;
	INFO("MQTT: Published\r\n");
}

void mqttDataCb(uint32_t *args, const char* topic, uint32_t topic_len, const char *data, uint32_t data_len)
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
	static uint8_t i;
	DHT_Sensor_Data data;
	uint8_t pin;
	os_timer_disarm(&dhtTimer);
	pin = pin_num[sensor.pin];
	if (DHTRead(&sensor, &data))
	{
	    char buff[20];
	    INFO("Reading sensor on GPIO%d\r\n", pin);
	    INFO("Temperature: %s *C\r\n", DHTFloat2String(buff, data.temperature));
	    INFO("Humidity: %s %%\r\n", DHTFloat2String(buff, data.humidity));

	    if (mqttClient.connState == WIFI_INIT) INFO("WIFI_INIT");
	    if (mqttClient.connState == WIFI_CONNECTING) INFO("WIFI_CONNECTING");
	    if (mqttClient.connState == WIFI_CONNECTING_ERROR) INFO("WIFI_CONNECTING_ERROR");
	    if (mqttClient.connState == WIFI_CONNECTED) INFO("WIFI_CONNECTED");
	    if (mqttClient.connState == DNS_RESOLVE) INFO("DNS_RESOLVE");
	    if (mqttClient.connState == TCP_DISCONNECTED) INFO("TCP_DISCONNECTED");
	    if (mqttClient.connState == TCP_RECONNECT_REQ) INFO("TCP_RECONNECT_REQ");
	    if (mqttClient.connState == TCP_RECONNECT) INFO("TCP_RECONNECT");
	    if (mqttClient.connState == TCP_CONNECTING) INFO("TCP_CONNECTING");
	    if (mqttClient.connState == TCP_CONNECTING_ERROR) INFO("TCP_CONNECTING_ERROR");
	    if (mqttClient.connState == TCP_CONNECTED) INFO("TCP_CONNECTED");
	    if (mqttClient.connState == MQTT_CONNECT_SEND) INFO("MQTT_CONNECT_SEND");
	    if (mqttClient.connState == MQTT_CONNECT_SENDING) INFO("MQTT_CONNECT_SENDING");
	    if (mqttClient.connState == MQTT_SUBSCIBE_SEND) INFO("MQTT_SUBSCIBE_SEND");
	    if (mqttClient.connState == MQTT_SUBSCIBE_SENDING) INFO("MQTT_SUBSCIBE_SENDING");
	    if (mqttClient.connState == MQTT_DATA) INFO("MQTT_DATA");
	    if (mqttClient.connState == MQTT_PUBLISH_RECV) INFO("MQTT_PUBLISH_RECV");
	    if (mqttClient.connState == MQTT_PUBLISHING) INFO("MQTT_PUBLISHING");

	} else {
		INFO("Failed to read temperature and humidity sensor on GPIO%d\n", pin);
	}
	os_timer_arm(&dhtTimer, DELAY, 1);
}

void user_init(void)
{
	uart_init(BIT_RATE_115200, BIT_RATE_115200);
	os_delay_us(1000000);

	CFG_Load();

	sensor.pin = 4; // Pin number 4 = GPIO2
	sensor.type = DHT22;
	INFO("DHT init on GPIO%d\r\n", pin_num[sensor.pin]);
	DHTInit(&sensor);

	MQTT_InitConnection(&mqttClient, sysCfg.mqtt_host, sysCfg.mqtt_port, sysCfg.security);
	MQTT_InitClient(&mqttClient, sysCfg.device_id, sysCfg.mqtt_user, sysCfg.mqtt_pass, sysCfg.mqtt_keepalive, 1);
	MQTT_InitLWT(&mqttClient, "/lwt", "offline", 0, 0);
	MQTT_OnConnected(&mqttClient, mqttConnectedCb);
	MQTT_OnDisconnected(&mqttClient, mqttDisconnectedCb);
	MQTT_OnPublished(&mqttClient, mqttPublishedCb);
	MQTT_OnData(&mqttClient, mqttDataCb);

	WIFI_Connect(sysCfg.sta_ssid, sysCfg.sta_pwd, wifiConnectCb);

	os_timer_disarm(&dhtTimer);
	os_timer_setfn(&dhtTimer, (os_timer_func_t *)dhtCb, (void *)0);
	os_timer_arm(&dhtTimer, DELAY, 1);

	INFO("\r\nSystem started ...\r\n");
}
