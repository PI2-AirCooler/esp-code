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

using namespace std;

// C

extern "C" {
	int rom_phy_get_vdd33();
}



static void main_Task(void *pvParameters) ;
static void debugInitial();
static void sendInfo(Fields& fields);


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

	// Initialize the Esp32 

	mUtil.esp32Initialize();

	// Initialize Ble Server 

	bleInitialize();

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

			// Resets the time variables and returns to the beginning of the loop ?
			// Usefull to initialize the time (for example, after App connection)

			if (reset_timer) {

				// Clear variables

				mTimeSeconds = 0;
				mLastTimeFeedback = 0;
				mLastTimeReceivedData = 0;

				logD("Reseted time");

				continue; // Returns to loop begin

			}
		} 

		mTimeSeconds++; 

		// TODO: see it! Put here your custom code to run every second

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

	// Restore logging ?

	if (mLogActiveSaved && !mLogActive) {
		mLogActive = true; // Restore state
	}

	logD ("Initializing ..."); 

	///// Initialize the variables

	// Initialize times 

	mLastTimeReceivedData = 0;
	mLastTimeFeedback = 0;

	// TODO: see it! Please put here custom global variables or code for init

	// Debugging

	logD ("Initialized");

} 

/**
 * @brief Process the message received from BLE
 * Note: this routine is in main.cc due major resources is here
 */
void processBleMessage (const string& message) {

	// This is to process ASCII (text) messagens - not binary ones

	string response = ""; // Return response to mobile app

	// --- Process the received line 

	// Check the message

	if (message.length () < 2 ) {

		error("Message length must have 2 or more characters");
		return; 

	} 

	// Process fields of the message 

	Fields fields(message, ":");

	// Code of the message 

	uint8_t code = 0;

	if (!fields.isNum (1)) { // Not numerical
		error ("Non-numeric message code"); 
		return; 
	} 

	code = fields.getInt (1);

	if (code == 0) { // Invalid code
		error ("Invalid message code"); 
		return; 
	} 

	logV("Code -> %u Message -> %s", code, mUtil.strExpand (message).c_str());

	// Considers the message received as feedback also 

	mLastTimeFeedback = mTimeSeconds;


	switch (code) { // Note: the '{' and '}' in cases, is to allow to create local variables, else give an error cross definition ... 

	// --- Initial messages 

	case 1: // Start 
		{
			// Initial message sent by the mobile application, to indicate start of the connection

			if (mLogActive) { 
				debugInitial();
			}

			// Reinicialize the app - include timer of seconds

			appInitialize(true);

			// Indicates connection initiated by the application 

			mAppConnected = true;

			// Inform to mobile app, if this device is battery powered and sensors 
			// Note: this is important to App works with differents versions or models of device



			// No, no is a battery powered device

			string haveBattery = "N";
			string sensorCharging = "N";

			// Debug

			bool turnOnDebug = false;


			turnOnDebug = !mLogActive;


			// Turn on the debugging (if the USB cable is connected)

			if (turnOnDebug) {

				mLogActive = true; 
				debugInitial();
			} 

			// Reset the time in main_Task

			notifyMainTask(MAIN_TASK_ACTION_RESET_TIMER);

			// Returns status of device, this firware version and if is a battery powered device

			response = "01:";
			response.append(FW_VERSION);
			response.append(1u, ':');
			response.append(haveBattery);
			response.append(1u, ':');
			response.append(sensorCharging);
			
		}
		break; 
	
	case 11: // Request of ESP32 informations
		{
			// Example of passing fields class to routine process

			sendInfo(fields);

		}
		break;

	// TODO: see it! Please put here custom messages

	case 70: // Echo (for test purpose)
		{
			response = message;
		}
		break; 

	case 71: // Logging - activate or desactivate debug logging - save state to use after
		{
			switch (fields.getChar(2)) // Process options
			{
				case 'Y': // Yes

					mLogActiveSaved = mLogActive; // Save state
					mLogActive = true; // Activate it

					logV("Logging activated now");
					break;

				case 'N': // No

					logV("Logging deactivated now");

					mLogActiveSaved = mLogActive; // Save state
					mLogActive = false; // Deactivate it
					break;

				case 'R': // Restore

					mLogActive = mLogActiveSaved; // Restore state
					logV("Logging state restored now");
					break;
			}
		}
		break; 

	case 80: // Feedback 
		{
			// Message sent by the application periodically, for connection verification

			logV("Feedback recebido");

			// Response it (put here any information that needs)

			response = "80:"; 
		}
		break; 

	case 98: // Reinicialize the app
		{
			logI ("Reinitialize");

			// End code placed at the end of this routine to send OK before 
		}
		break; 

	case 99: // Enter in standby
		{
			logI ("Entering in standby");

			// End code placed at the end of this routine to send OK before 
		}
		break; 

	default: 
		{
			string errorMsg = "Code of message invalid: ";
			errorMsg.append(mUtil.intToStr(code)); 
			error (errorMsg.c_str()); 
			return;
		}
	}

	// return 

	if (response.size() > 0) {

		// Return -> Send message response

		if (bleConnected ()) { 
			bleSendData(response);
		} 
	} 


	// Mark the mTimeSeconds of the receipt 

	mLastTimeReceivedData = mTimeSeconds;

	// Here is processed messages that have actions to do after response sended

	switch (code) {

	case 98: // Restart the Esp32

		// Wait 500 ms, to give mobile app time to quit 

		if (mAppConnected) {
			delay (500); 
		}

		restartESP32();
		break; 

	case 99: 
		restartESP32();

		break; 

	} 

} 

/**
 * @brief Show error and notifies the application error occurred
 */
void error (const char *message, bool fatal) {

	// Debug

	logE("Error -> %s", message);

	// Send the message 

	if (bleConnected ()) { 
		string error = "-1:"; // -1 is a code of error messages
		error.append (message);
		bleSendData(error);
	} 

	// Fatal ?

	if (fatal) {

		// Wait a time

		delay (200);

		// Restart ESP32

		restartESP32 ();

	}

} 

/**
 * @brief Reset the ESP32
 */
void restartESP32 () { 

	// TODO: see it! if need, put your custom code here 

	// Reinitialize 

	esp_restart (); 

} 

/**
 * @brief Process informations request
 */
static void sendInfo(Fields& fields) {

	// Note: the field 1 is a code of message

	// Type 

	string type = fields.getString(2);

	logV("type=%s", type.c_str());

	// Note: this is a example of send large message 

	const uint16_t MAX_INFO = 300;
	char info[MAX_INFO];

	// Return response (can bem more than 1, delimited by \n)

	string response = "";

	if (type == "ESP32" || type == "ALL") { // Note: For this example string type, but can be numeric

		// About the ESP32 // based on Kolban GeneralUtils
		// With \r as line separator


		const uint8_t* macAddr = bleMacAddress();

		char deviceName[30] = BLE_DEVICE_NAME;

		uint8_t size = strlen(deviceName);

		if (size > 0 && deviceName[size-1] == '_') { // Put last 2 of mac address in the name

			char aux[7];
			sprintf(aux, "%02X%02X", macAddr[4], macAddr[5]);

			strcat (deviceName, aux);
		}

#if !CONFIG_FREERTOS_UNICORE
		const char* uniCore = "No"; 
#else
		const char* uniCore = "Yes"; 
#endif

		// Note: the \n is a message separator and : is a field separator
		// Due this send # and ; (this will replaced in app mobile) 

		snprintf(info, MAX_INFO, "11:ESP32:"\
									"*** Chip Info#" \
									"* FreeRTOS unicore ?; %s#"
									"* ESP-IDF;#  %s#" \
									"*** BLE info#" \
									"* Device name; %s#" \
									"* Mac-address; %02X;%02X;%02X;%02X;%02X;%02X#" \
									"\n", \
									uniCore, 
									esp_get_idf_version(), \
									deviceName,
									macAddr[0], macAddr[1], \
									macAddr[2], macAddr[3], \
									macAddr[4], macAddr[5] \
									); 

		response.append(info);
	}

	if (type == "FMEM" || type == "ALL") {

		// Free memory of ESP32 
		
		snprintf(info, MAX_INFO, "11:FMEM:%u\n", heap_caps_get_free_size(MALLOC_CAP_8BIT));

		response.append(info);

	}

	if (type == "VDD33" || type == "ALL") {

		// Voltage of ESP32 
		
		int read = rom_phy_get_vdd33();
		logV("rom_phy_get_vdd33=%d", read);

		snprintf(info, MAX_INFO, "11:VDD33:%d\n", read);

		response.append(info);

	}

//	logV("response -> %s", response.c_str());

	// Send

	bleSendData(response);

}

/**
 * @brief Initial Debugging 
 */
static void debugInitial () {

	logV ("Debugging is on now");

	logV ("Firmware device version: %s", FW_VERSION);

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

//////// End
