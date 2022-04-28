#ifndef EVENTS_
#define EVENTS_

/**
 * Estrutura do json: 
 * 
 * {
 *      evento: payload
 * }
 * 
 * GET_TEMPERATURE payload: number
 * GET_TEMPERATURE status: string
 */
#define GET_TEMPERATURE "temperature"
#define GET_STATUS "status"
#define SET_STATUS "set_status"
#define SET_TEMPERATURE "set_status"


#define STATUS_INITIALIZED "started"
#define STATUS_IDLE "idle"
#define STATUS_ERROR "error"
#define STATUS_FREEZING "freezing"
#define STATUS_OFF "off"
#endif