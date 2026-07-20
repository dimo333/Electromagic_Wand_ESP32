idf_component_register(
    SRCS
        "get_data_main.c"
        "mpu6050.c"
        "kalman_wand.c"
    INCLUDE_DIRS "."
    REQUIRES
        driver
        esp_timer
)
