#include <stdio.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "sys/_default_fcntl.h"
#include "esp_bt_defs.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_bt.h"
#include "nvs_flash.h"
#include "freertos/event_groups.h"
#include "constants.h"
#include "driver/can.h"
#include "a2dp.h"
#include "esp_avrc_api.h"
#include "driver/ledc.h"

void initBluetooth() {
	esp_err_t ret;

	esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
	if (esp_bt_controller_init(&bt_cfg)) {
		ESP_LOGE("InitBT", "%s initialize controller failed\n", __func__);
		return;
	}

    if ((ret = esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT)) != ESP_OK) {
        ESP_LOGE("InitBT", "%s enable CLASSIC controller failed: %s", __func__, esp_err_to_name(ret));
        return;
    }

	if (esp_bluedroid_init()) {
		ESP_LOGE("InitBT", "%s init bluedroid failed\n", __func__);
		return;
	}

	if (esp_bluedroid_enable()) {
		ESP_LOGE("InitBT", "%s init bluedroid failed\n", __func__);
		return;
	}
}

void initCan() {
	can_general_config_t g_config = CAN_GENERAL_CONFIG_DEFAULT(CAN_TX, CAN_RX, CAN_MODE_LISTEN_ONLY);
	//can_general_config_t g_config = CAN_GENERAL_CONFIG_DEFAULT(CAN_TX, CAN_RX, CAN_MODE_NORMAL);
	can_timing_config_t t_config = CAN_SPEED;
	can_filter_config_t f_config = CAN_FILTER_CONFIG_ACCEPT_ALL();

	//Install CAN driver
	if (can_driver_install(&g_config, &t_config, &f_config) == ESP_OK)
		ESP_LOGI("CAN", "Driver installed");
	else {
		ESP_LOGE("CAN", "Failed to install driver");
		return;
	}

	//Start CAN driver
	if (can_start() == ESP_OK)
		ESP_LOGI("CAN", "Driver started");
	else {
		ESP_LOGE("CAN", "Failed to start driver");
		return;
	}
}

static uint8_t avrcpTransactionId = 0;
static uint8_t lastButtonStatus = 0x00;
void processSteeringWheelButton(uint8_t data, uint8_t mask, uint8_t avrcpCommand) {
	uint8_t currentStatus = data & mask;
	uint8_t previousStatus = lastButtonStatus & mask;

	if (currentStatus != previousStatus) {
		if (currentStatus) { // pressed
			currentStatus |= mask;
			esp_avrc_ct_send_passthrough_cmd(avrcpTransactionId, avrcpCommand, ESP_AVRC_PT_CMD_STATE_PRESSED);
		} else { // released
			currentStatus &= ~mask;
			esp_avrc_ct_send_passthrough_cmd(avrcpTransactionId, avrcpCommand, ESP_AVRC_PT_CMD_STATE_RELEASED);
		}

		avrcpTransactionId++;
		avrcpTransactionId &= 0x0F; // values in [0, 15] are valid
	}
}

bool isInAuxMode = false;
bool isBlueAndMeActive = false;

void canReceiverTask(void *pvParameters) {
	can_message_t message;
	while (1)
		if (can_receive(&message, pdMS_TO_TICKS(2000)) == ESP_OK) {
			//ESP_LOGI("CAN", "id: %x, data[0]: %d", message.identifier, message.data[0]);

			if (message.identifier == 0xa114005 && message.data_length_code >= 5)
				isInAuxMode = message.data[4] == 8;
			else if (message.identifier == 0x6314021 && message.data_length_code >= 7)
				isBlueAndMeActive = message.data[6] == 196;
			else if (message.identifier == 0x6354000 && message.data_length_code >= 2
					&& isInAuxMode && !isBlueAndMeActive) {
				processSteeringWheelButton(message.data[0], 0x10, ESP_AVRC_PT_CMD_BACKWARD);
				processSteeringWheelButton(message.data[0], 0x08, ESP_AVRC_PT_CMD_FORWARD);
				lastButtonStatus = message.data[0];
			}
		}
}

void app_main(void) {
	ESP_LOGI("Init", "Starting...");

	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);

	initBluetooth();
	startA2dp();

	initCan();

	xTaskCreate(&canReceiverTask, "canReceiverTask", 4096, NULL, 5, NULL);
}
