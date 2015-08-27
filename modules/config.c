/*
/* config.c
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
#include "os_type.h"
#include "mem.h"
#include "osapi.h"
#include "user_interface.h"

#include "mqtt.h"
#include "config.h"
#include "user_config.h"
#include "debug.h"

SYSCFG config;
SAVE_FLAG save_flag;

void ICACHE_FLASH_ATTR
config_save()
{
	 spi_flash_read((CFG_LOCATION + 3) * SPI_FLASH_SEC_SIZE,
	                   (uint32 *)&save_flag, sizeof(SAVE_FLAG));

	if (save_flag.flag == 0) {
		spi_flash_erase_sector(CFG_LOCATION + 1);
		spi_flash_write((CFG_LOCATION + 1) * SPI_FLASH_SEC_SIZE,
						(uint32 *)&config, sizeof(SYSCFG));
		save_flag.flag = 1;
		spi_flash_erase_sector(CFG_LOCATION + 3);
		spi_flash_write((CFG_LOCATION + 3) * SPI_FLASH_SEC_SIZE,
						(uint32 *)&save_flag, sizeof(SAVE_FLAG));
	} else {
		spi_flash_erase_sector(CFG_LOCATION + 0);
		spi_flash_write((CFG_LOCATION + 0) * SPI_FLASH_SEC_SIZE,
						(uint32 *)&config, sizeof(SYSCFG));
		save_flag.flag = 0;
		spi_flash_erase_sector(CFG_LOCATION + 3);
		spi_flash_write((CFG_LOCATION + 3) * SPI_FLASH_SEC_SIZE,
						(uint32 *)&save_flag, sizeof(SAVE_FLAG));
	}
}

void ICACHE_FLASH_ATTR
config_load()
{

	INFO("\r\nload ...\r\n");
	spi_flash_read((CFG_LOCATION + 3) * SPI_FLASH_SEC_SIZE,
				   (uint32 *)&save_flag, sizeof(SAVE_FLAG));
	if (save_flag.flag == 0) {
		spi_flash_read((CFG_LOCATION + 0) * SPI_FLASH_SEC_SIZE,
					   (uint32 *)&config, sizeof(SYSCFG));
	} else {
		spi_flash_read((CFG_LOCATION + 1) * SPI_FLASH_SEC_SIZE,
					   (uint32 *)&config, sizeof(SYSCFG));
	}
	if(config.cfg_holder != CFG_HOLDER){
		os_memset(&config, 0x00, sizeof config);

		config.cfg_holder = CFG_HOLDER;

		os_sprintf(config.sta_ssid, "%s", STA_SSID);
		os_sprintf(config.sta_pwd, "%s", STA_PASS);
		config.sta_type = STA_TYPE;

		os_sprintf(config.device_id, MQTT_CLIENT_ID, system_get_chip_id());
		os_sprintf(config.mqtt_topic, MQTT_TOPIC, system_get_chip_id());
		os_sprintf(config.mqtt_host, "%s", MQTT_HOST);
		config.mqtt_port = MQTT_PORT;
		os_sprintf(config.mqtt_user, "%s", MQTT_USER);
		os_sprintf(config.mqtt_pass, "%s", MQTT_PASS);

		config.security = DEFAULT_SECURITY;	/* default non ssl */

		config.mqtt_keepalive = MQTT_KEEPALIVE;

		INFO("Default configuration\r\n");

		config_save();
	}

}
