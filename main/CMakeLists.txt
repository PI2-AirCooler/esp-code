set(COMPONENT_SRCS 
    "main.cc"
    "../drivers/mpu6050.c"
    "../drivers/ble.cc"
    "../drivers/cJSON.c"
    "../drivers/ds18b20.c"
    "../drivers/ble_server.cc"
    "../drivers/ble_uart_server.c"
    "../drivers/esp_util.cc"
    "../drivers/fields.cc"
    "../drivers/I2Cbus.cpp"
    INCLUDE_DIRS "../include/.")

register_component()
