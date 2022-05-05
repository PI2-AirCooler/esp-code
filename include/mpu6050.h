#ifndef _MPU_H_
#define _MPU_H_

#include <driver/i2c.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <math.h>

#include "sdkconfig.h"

void configure_gyroscope(i2c_port_t i2c_port);
void get_xyz_by(short *accel_x, short *accel_y, short *accel_z);

#endif /* end of include guard: _I2CBUS_H_ */