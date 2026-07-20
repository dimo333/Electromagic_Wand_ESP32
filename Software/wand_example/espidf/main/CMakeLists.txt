idf_component_register(
    SRCS
        "wand_main.cpp"
        "mpu6050.c"
        "kalman_wand.c"
    INCLUDE_DIRS "."
    REQUIRES
        driver
        esp_timer
        esp-tflite-micro
)
