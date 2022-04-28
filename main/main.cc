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

using namespace std;

// C

extern "C" {
	int rom_phy_get_vdd33();
}



static void main_Task(void *pvParameters) ;
static void debugInitial();
static void sendInfo(Fields& fields);

static string currStatus = STATUS_INITIALIZED;
static const char *TAG = "main";

// Utility 

static Esp_Util& mUtil = Esp_Util::getInstance(); 

// Times and intervals 

uint32_t mTimeSeconds = 0; 			// Current time in seconds (for timeouts calculations)

uint32_t mLastTimeFeedback = 0; 	// Indicates the time of the last feedback message

uint32_t mLastTimeReceivedData = 0; // time of receipt of last line via

// Log active (debugging)? 

bool mLogActive = false;
bool mLogActiveSaved = false;

// Connected? 

bool mAppConnected = false; // Indicates connection when receiving message 01: 

////// FreeRTOS

// Task main

static TaskHandle_t xTaskMainHandler = NULL;

////// Main

extern "C" {
	void app_main();
}

/**
 * @brief app_main of ESP-IDF 
 */
void app_main()
{

	mLogActive = true; // To show initial messages

	logI("Initializing");  

	mUtil.esp32Initialize();
	bleInitialize();
	ds18b20_init(12);

	// Task -> Initialize task_main in core 1, if is possible

	xTaskCreatePinnedToCore (&main_Task,
				"main_Task", TASK_STACK_LARGE, NULL, TASK_PRIOR_HIGH, &xTaskMainHandler, TASK_CPU);

    return; 
} 

static void main_Task (void * pvParameters) { 

	logI ("Starting main Task"); 
	
	mTimeSeconds = 0; 


	appInitialize (false);

	const TickType_t xTicks = (1000u / portTICK_PERIOD_MS);


	uint32_t notification; // Notification variable 

	for (;;) {
		if (xTaskNotifyWait (0, 0xffffffff, &notification, xTicks) == pdPASS) { 


			logD ("Notification received -> %u", notification); 

			bool reset_timer = false;

			switch (notification) {

				case MAIN_TASK_ACTION_RESET_TIMER: 	// Reset timer
					reset_timer = true;
					break;

				case MAIN_TASK_ACTION_STANDBY_BTN: 	// Enter in standby - to deep sleep not run in ISR
					
					break;

				case MAIN_TASK_ACTION_STANDBY_MSG: 	// Enter in standby - to deep sleep not run in ISR
					break;

				// TODO: see it! If need put here your custom notifications
				default:
					break;
			}
		} 

		mTimeSeconds++; 

		if (!bleConnected()) {
			continue;
		}

	} 

	vTaskDelete (NULL); 
	xTaskMainHandler = NULL;
}

/**
 * @brief Initializes the app
 */
void appInitialize(bool resetTimerSeconds) {
	// TODO

} 

void processBleMessage(const string& message) {


	string response = ""; // Return response to mobile app

	// Process fields of the message 
	
	if (response.size() > 0) {

		// Return -> Send message response

		if (bleConnected ()) { 
			bleSendData(response);
		} 
	} 


	// Mark the mTimeSeconds of the receipt 

	mLastTimeReceivedData = mTimeSeconds;

} 

/**
 * @brief Reset the ESP32
 */
void restartESP32 () { 
	esp_restart (); 
} 


/**
 * @brief Cause an action on main_Task by task notification
 */
void IRAM_ATTR notifyMainTask(uint32_t action, bool fromISR) {

	// Main Task is alive ?

	if (xTaskMainHandler == NULL) {
		return;
	}

	// Debug (for non ISR only) 

	if (!fromISR) {
		logD ("action=%u", action);
	} 

	// Notify the main task

	if (fromISR) {// From ISR
		BaseType_t xHigherPriorityTaskWoken;
		xTaskNotifyFromISR (xTaskMainHandler, action, eSetValueWithOverwrite, &xHigherPriorityTaskWoken);
	} else {
		xTaskNotify (xTaskMainHandler, action, eSetValueWithOverwrite);
	}

}
