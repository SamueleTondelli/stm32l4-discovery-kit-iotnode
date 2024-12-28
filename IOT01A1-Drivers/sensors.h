#ifndef SENSORS_H
#define SENSORS_H

#include <stdint.h>

typedef enum {
    LPS_HZ_1 = 0x10,
    LPS_HZ_10 = 0x20,
    LPS_HZ_25 = 0x30,
    LPS_HZ_50 = 0x40,
    LPS_HZ_75 = 0x50
} LPS22HBUpdateRate;

void lps22hb_init(LPS22HBUpdateRate update_rate);
float lps22hb_read_press();
float lps22hb_read_temp();

typedef enum {
    HTS_HZ_1 = 0x01,
    HTS_HZ_7 = 0x02,
    HTS_HZ_12_5 = 0x03,
} HTS221UpdateRate;

typedef struct {
    int16_t t0_out;
    uint16_t t0_degc;
    int16_t t1_out;
    uint16_t t1_degc;
    int16_t h0_out;
    uint16_t h0_rh;
    int16_t h1_out;
    uint16_t h1_rh;
    float t_m;
    float t_b;
    float h_m;
    float h_b;
} HTS221CalibrationMeaseures;

HTS221CalibrationMeaseures hts221_init(HTS221UpdateRate update_rate);
float hts221_read_temp(const HTS221CalibrationMeaseures *measeures);
float hts221_read_hum(const HTS221CalibrationMeaseures *measeures);

typedef enum {
    XL_POWER_OFF = 0x00,
    XL_12_5_HZ = 0x01,
    XL_26_HZ = 0x02,
    XL_52_HZ = 0x03,
    XL_104_HZ = 0x04,
    XL_208_HZ = 0x05,
    XL_416_HZ = 0x06,
    XL_833_HZ = 0x07,
    XL_1_66_KHZ = 0x08,
    XL_3_33_KHZ = 0x09,
    XL_6_66_KHZ = 0x0a
} LSM6DSLXLUpdateRate;

typedef enum {
    XL_2_G = 0x00,
    XL_16_G = 0x01,
    XL_4_G = 0x02,
    XL_8_G = 0x03
} LSM6DSLXLFullScale;

typedef enum {
    G_POWER_DOWN = 0x00,
    G_12_5_HZ = 0x01,
    G_26_HZ = 0x02,
    G_52_HZ = 0x03,
    G_104_HZ = 0x04,
    G_208_HZ = 0x05,
    G_416_HZ = 0x06,
    G_833_HZ = 0x07,
    G_1_66_KHZ = 0x08,
    G_3_33_KHZ = 0x09,
    G_6_66_KHZ = 0x0a
} LSM6DSLGUpdateRate;

typedef enum {
    G_250_DPS = 0x00,
    G_500_DPS = 0x01,
    G_1000_DPS = 0x02,
    G_2000_DPS = 0x03
} LSM6DSLGFullScale;

typedef struct {
    float x;
    float y;
    float z;
} Vec3;

void lsm6dsl_init(LSM6DSLXLUpdateRate accel_update_rate, LSM6DSLXLFullScale accel_full_scale, LSM6DSLGUpdateRate gyro_update_rate, LSM6DSLGFullScale gyro_full_scale);
Vec3 lsm6dsl_read_accel();
Vec3 lsm6dsl_read_gyro();

typedef enum {
    LIS_0_625_HZ = 0x00,
    LIS_1_25_HZ = 0x01,
    LIS_2_5_HZ = 0x02,
    LIS_5_HZ = 0x03,
    LIS_10_HZ = 0x04,
    LIS_20_HZ = 0x05,
    LIS_40_HZ = 0x06,
    LIS_80_HZ = 0x07
} LIS3MDLUpdateRate;

typedef enum {
    LIS_4_GAUSS = 0x00,
    LIS_8_GAUSS = 0x01,
    LIS_12_GAUSS = 0x02,
    LIS_16_GAUSS = 0x03
} LIS3MDLFullScale;

void lis3mdl_init(LIS3MDLUpdateRate update_rate, LIS3MDLFullScale full_scale);
Vec3 lis3mdl_read_mag();

#endif
