set(COMPONENT_SRCS 
    "main.cc"
    "../drivers/ble.cc"
    "../drivers/mpu6050.c"
    "../drivers/ble_server.cc"
    "../drivers/ble_uart_server.c"
    "../drivers/esp_util.cc"
    "../drivers/fields.cc"
    INCLUDE_DIRS "../include/.")

register_component()
