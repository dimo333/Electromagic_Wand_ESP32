/*
 * ------------------------------------------------------------
 * MPU6050 minimal driver for ESP-IDF (new i2c_master API).
 * See mpu6050.h for the origin of this code.
 * ------------------------------------------------------------
 */
#include "mpu6050.h"

#include <stdlib.h>
#include "driver/i2c_master.h"
#include "esp_log.h"

#define MPU6050_REG_PWR_MGMT_1   0x6B
#define MPU6050_REG_WHO_AM_I     0x75
#define MPU6050_REG_ACCEL_XOUT_H 0x3B
#define MPU6050_WHO_AM_I_VALUE   0x68

#define MPU6050_I2C_TIMEOUT_MS   100
#define MPU6050_I2C_FREQ_HZ      400000

static const char *TAG = "mpu6050";

struct mpu6050_dev {
    i2c_master_bus_handle_t bus;
    i2c_master_dev_handle_t dev;
};

static esp_err_t mpu_write_reg(mpu6050_handle_t dev, uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = { reg, val };
    return i2c_master_transmit(dev->dev, buf, sizeof(buf), MPU6050_I2C_TIMEOUT_MS);
}

static esp_err_t mpu_read_regs(mpu6050_handle_t dev, uint8_t reg,
                               uint8_t *data, size_t len)
{
    return i2c_master_transmit_receive(dev->dev, &reg, 1, data, len,
                                       MPU6050_I2C_TIMEOUT_MS);
}

esp_err_t mpu6050_init(int sda_gpio, int scl_gpio, uint8_t i2c_addr,
                       mpu6050_handle_t *out)
{
    if (!out) {
        return ESP_ERR_INVALID_ARG;
    }

    mpu6050_handle_t dev = calloc(1, sizeof(struct mpu6050_dev));
    if (!dev) {
        return ESP_ERR_NO_MEM;
    }

    i2c_master_bus_config_t bus_cfg = {
        .clk_source                   = I2C_CLK_SRC_DEFAULT,
        .i2c_port                     = -1,          /* auto-select a free port */
        .scl_io_num                   = scl_gpio,
        .sda_io_num                   = sda_gpio,
        .glitch_ignore_cnt            = 7,
        .flags.enable_internal_pullup = true,
    };
    esp_err_t err = i2c_new_master_bus(&bus_cfg, &dev->bus);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2c_new_master_bus failed: %s", esp_err_to_name(err));
        free(dev);
        return err;
    }

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = i2c_addr,
        .scl_speed_hz    = MPU6050_I2C_FREQ_HZ,
    };
    err = i2c_master_bus_add_device(dev->bus, &dev_cfg, &dev->dev);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "i2c_master_bus_add_device failed: %s", esp_err_to_name(err));
        i2c_del_master_bus(dev->bus);
        free(dev);
        return err;
    }

    /* Wake the device: clear SLEEP bit in PWR_MGMT_1. Accel/gyro ranges stay
     * at their reset defaults (+-2g, +-250 dps), matching the Arduino code. */
    err = mpu_write_reg(dev, MPU6050_REG_PWR_MGMT_1, 0x00);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "wake-up write failed: %s", esp_err_to_name(err));
        i2c_master_bus_rm_device(dev->dev);
        i2c_del_master_bus(dev->bus);
        free(dev);
        return err;
    }

    *out = dev;
    return ESP_OK;
}

bool mpu6050_test_connection(mpu6050_handle_t dev)
{
    uint8_t who = 0;
    if (mpu_read_regs(dev, MPU6050_REG_WHO_AM_I, &who, 1) != ESP_OK) {
        return false;
    }
    return who == MPU6050_WHO_AM_I_VALUE;
}

esp_err_t mpu6050_get_motion6(mpu6050_handle_t dev,
                              int16_t *ax, int16_t *ay, int16_t *az,
                              int16_t *gx, int16_t *gy, int16_t *gz)
{
    /* 14 bytes: ACCEL(6) TEMP(2) GYRO(6), big-endian, starting at ACCEL_XOUT_H */
    uint8_t b[14];
    esp_err_t err = mpu_read_regs(dev, MPU6050_REG_ACCEL_XOUT_H, b, sizeof(b));
    if (err != ESP_OK) {
        return err;
    }

    if (ax) *ax = (int16_t)((b[0]  << 8) | b[1]);
    if (ay) *ay = (int16_t)((b[2]  << 8) | b[3]);
    if (az) *az = (int16_t)((b[4]  << 8) | b[5]);
    /* b[6],b[7] = temperature, unused */
    if (gx) *gx = (int16_t)((b[8]  << 8) | b[9]);
    if (gy) *gy = (int16_t)((b[10] << 8) | b[11]);
    if (gz) *gz = (int16_t)((b[12] << 8) | b[13]);
    return ESP_OK;
}

esp_err_t mpu6050_get_acceleration(mpu6050_handle_t dev,
                                   int16_t *ax, int16_t *ay, int16_t *az)
{
    uint8_t b[6];
    esp_err_t err = mpu_read_regs(dev, MPU6050_REG_ACCEL_XOUT_H, b, sizeof(b));
    if (err != ESP_OK) {
        return err;
    }
    if (ax) *ax = (int16_t)((b[0] << 8) | b[1]);
    if (ay) *ay = (int16_t)((b[2] << 8) | b[3]);
    if (az) *az = (int16_t)((b[4] << 8) | b[5]);
    return ESP_OK;
}
