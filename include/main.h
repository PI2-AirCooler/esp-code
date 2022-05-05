#ifndef MAIN_H_
#define MAIN_H_

////// Includes

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_system.h"
#include "soc/soc.h"

#include "sdkconfig.h"

// C++

#include <string>
using namespace std;

/////// Definitions 

// Firmware version

#define FW_VERSION "0.3.0"

// FreeRTOS // TODO: see it!

// Dual or single core ?

#if !CONFIG_FREERTOS_UNICORE
#define TASK_CPU APP_CPU_NUM // Core 1
#else
#define TASK_CPU PRO_CPU_NUM // Core 0
#endif

// Task priorities

#define TASK_PRIOR_HIGHEST  20
#define TASK_PRIOR_HIGH     5
#define TASK_PRIOR_MEDIUM   3
#define TASK_PRIOR_LOW      1

// Stack Depth

#define TASK_STACK_LARGE 	10240
#define TASK_STACK_MEDIUM   5120
#define TASK_STACK_SMALL    1024

// Task stack Depth 

#define TASK_STACK_LARGE 10240 
#define TASK_STACK_MEDIUM 5120 
#define TASK_STACK_SMALL 1024 


#ifdef HAVE_STANDBY
    #define MAX_TIME_INACTIVE 300    // Maximum inactive time to deep sleep (comment if want it disabled)
#endif


// Actions of main_Task - by task notifications

#define MAIN_TASK_ACTION_NONE 			0	// No action
#define MAIN_TASK_ACTION_RESET_TIMER 	1	// To reset the seconds timer (for example, after a app connection)
#define MAIN_TASK_ACTION_STANDBY_BTN 	2	// For button -> To enter in deep sleep (to not do it in ISR)
#define MAIN_TASK_ACTION_STANDBY_MSG 	3	// For msg BLE -> To enter in deep sleep (to not do it in ISR)

////// Prototypes of main

extern void notifyMainTask(uint32_t action, bool fromISR=false);
extern void processBleMessage(const string& message);
extern void error(const char* message, bool fatal=false);
extern void restartESP32();
void check_position();

////// External variables 

// Log 

// Connection with App mobile started? (received message code 01) 

extern bool mAppConnected;
 
// Times

extern uint32_t mTimeSeconds;
extern uint32_t mLastTimeReceivedData;

#endif // MAIN_H_

//////// End
