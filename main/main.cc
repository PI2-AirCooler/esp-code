#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "soc/soc.h"
#include <string>
#include "log.h"
#include "esp_util.h"
#include "fields.h"
#include "main.h"
#include "ble.h"
#include "ds18b20.h"
#include "events.h"
#include "cJSON.h"
#include "I2Cbus.hpp"
#include "constants.h"

using namespace std;


// C

extern "C" {
	int rom_phy_get_vdd33();
}

static void main_Task(void *pvParameters) ;

static string currentStatus = STATUS_INITIALIZED;
static const char *TAG = "main";
static float currentTemperature = 0;

static Esp_Util& mUtil = Esp_Util::getInstance(); 
static I2C_t& myI2C = i2c0;
static I2C_t& myI2C2 = i2c1;

static TaskHandle_t xTaskMainHandler = NULL;
bool mAppConnected = false;

extern "C" {
	void app_main();
}


void app_main()
{

	mUtil.esp32Initialize();
	bleInitialize();
	ds18b20_init(TEMPERATURE_SENSOR_PORT);

	xTaskCreatePinnedToCore(&main_Task,"main_Task", TASK_STACK_LARGE, NULL, TASK_PRIOR_HIGH, &xTaskMainHandler, TASK_CPU);

    return; 
} 

static void main_Task (void * pvParameters) { 

	logI ("Starting main Task"); 
	

	const TickType_t xTicks = (1000u / portTICK_PERIOD_MS);

	for (;;) {
		if (!bleConnected()) {
			continue;
		}

		else {
			cJSON *temperature = cJSON_CreateObject();
			cJSON *status = cJSON_CreateObject();
			char *statusByteArray = &currentStatus[0];

			currentTemperature = ds18b20_get_temp();

			if (temperature== NULL || status == NULL) {
				continue;
			}
			else {
				cJSON_AddNumberToObject(temperature, GET_TEMPERATURE, currentTemperature);
				cJSON_AddStringToObject(status, GET_STATUS, statusByteArray);
				
				bleSendData(cJSON_Print(temperature));
				bleSendData(cJSON_Print(status));
			}

		}

		vTaskDelay( xTicks );
	} 

	vTaskDelete (NULL); 
	xTaskMainHandler = NULL;
}

void processBleMessage(const string& message) {
	char *statusByteArray = &currentStatus[0];
	cJSON *json = cJSON_Parse(statusByteArray);
	
	if (message.size() > 0) {

		const cJSON *temperature = NULL;
		temperature = cJSON_GetObjectItemCaseSensitive(json, SET_TEMPERATURE);
		if (cJSON_IsNumber(temperature) && (temperature->valuedouble != NULL))
		{
			currentTemperature = temperature->valuedouble;
		}

		const cJSON *status = NULL;
		status = cJSON_GetObjectItemCaseSensitive(json, SET_STATUS);
		if (cJSON_IsString(status) && (status->valuestring != NULL))
		{
			currentStatus = temperature->valuestring;
		}

		const cJSON *restart = NULL;
		restart = cJSON_GetObjectItemCaseSensitive(json, RESTART_DEVICE);
		if (cJSON_IsBool(restart))
		{
			restartESP32();
		}
	}
}

void init_reles() {    
    myI2C.begin((gpio_num_t)RELE_1_PORT, (gpio_num_t)(RELE_1_PORT + 1), 400000);
    myI2C2.begin((gpio_num_t)RELE_2_PORT, (gpio_num_t)(RELE_2_PORT + 1), 400000);
    myI2C.setTimeout(10);
    myI2C.scanner();

    myI2C2.setTimeout(10);
    myI2C2.scanner();
}

void close_reles() {
	myI2C.close();
	myI2C2.close();
}

void init_freezing() {
	currentStatus = STATUS_FREEZING;
	init_reles();
}

/**
 * @brief Reset the ESP32
 */
void restartESP32() { 
	esp_restart(); 
} 
