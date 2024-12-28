#include "sensors.h"

#include <stdint.h>

#include "main.h"

extern I2C_HandleTypeDef hi2c2;

#define LPS22HB_ADDR 0xba
#define LPS22HB_CTRL1 0x10
#define LPS22HB_PRESS_XL 0x28
#define LPS22HB_PRESS_L 0x29
#define LPS22HB_PRESS_H 0x2a
#define LPS22HB_TEMP_L 0x2b
#define LPS22HB_TEMP_H 0x2c

void lps22hb_init(LPS22HBUpdateRate update_rate) {
    uint8_t payload = update_rate;
    HAL_I2C_Mem_Write(&hi2c2, LPS22HB_ADDR, LPS22HB_CTRL1, 1, &payload, 1, 1);
}

float lps22hb_read_press() {
    uint8_t press_xl, press_l, press_h;
    HAL_I2C_Mem_Read(&hi2c2, LPS22HB_ADDR, LPS22HB_PRESS_XL, 1, &press_xl, 1, 1);
    HAL_I2C_Mem_Read(&hi2c2, LPS22HB_ADDR, LPS22HB_PRESS_L, 1, &press_l, 1, 1);
    HAL_I2C_Mem_Read(&hi2c2, LPS22HB_ADDR, LPS22HB_PRESS_H, 1, &press_h, 1, 1);

    uint32_t tmp = press_xl | ((uint32_t)press_l << 8) | ((uint32_t)press_h << 16);
    if (tmp & 0x00800000) { // 24 bits complement
        tmp |= 0xFF000000;
    }
    int32_t raw = (int32_t)tmp;
    return (float)raw / 4096.f;
}

float lps22hb_read_temp() {
    uint8_t temp_l, temp_h;
    HAL_I2C_Mem_Read(&hi2c2, LPS22HB_ADDR, LPS22HB_TEMP_L, 1, &temp_l, 1, 1);
    HAL_I2C_Mem_Read(&hi2c2, LPS22HB_ADDR, LPS22HB_TEMP_H, 1, &temp_h, 1, 1);

    uint32_t tmp = temp_l | (temp_h << 8);
    if (tmp & 0x00008000) { // 16 bits complement
        tmp |= 0xFFFF0000;
    }
    int32_t raw = (int32_t)tmp;
    return (float)raw / 100.f;
}

#define HTS221_ADDR 0xbe
#define HTS221_CTRL1 0x20
#define HTS221_T0_DEGC 0x32
#define HTS221_T1_DEGC 0x33
#define HTS221_T_MSB 0x35
#define HTS221_T0_OUT_L 0x3c
#define HTS221_T0_OUT_H 0x3d
#define HTS221_T1_OUT_L 0x3e
#define HTS221_T1_OUT_H 0x3f
#define HTS221_H0_RH 0x30
#define HTS221_H1_RH 0x31
#define HTS221_H0_OUT_L 0x36
#define HTS221_H0_OUT_H 0x37
#define HTS221_H1_OUT_L 0x3a
#define HTS221_H1_OUT_H 0x3b
#define HTS221_T_OUT_L 0x2a
#define HTS221_T_OUT_H 0x2b
#define HTS221_H_OUT_L 0x28
#define HTS221_H_OUT_H 0x29

HTS221CalibrationMeaseures hts221_init(HTS221UpdateRate update_rate) {
    uint8_t payload = update_rate | 0x80;
    HAL_I2C_Mem_Write(&hi2c2, HTS221_ADDR, HTS221_CTRL1, 1, &payload, 1, 1);

    HTS221CalibrationMeaseures measeures = {0};
    // temperature measurements
    uint8_t t0_degc_l = 0, t1_degc_l = 0, t_msb = 0, t0_out_l = 0, t0_out_h = 0, t1_out_l = 0, t1_out_h = 0;
    HAL_I2C_Mem_Read(&hi2c2, HTS221_ADDR, HTS221_T0_DEGC, 1, &t0_degc_l, 1, 1);
    HAL_I2C_Mem_Read(&hi2c2, HTS221_ADDR, HTS221_T1_DEGC, 1, &t1_degc_l, 1, 1);
    HAL_I2C_Mem_Read(&hi2c2, HTS221_ADDR, HTS221_T_MSB, 1, &t_msb, 1, 1);
    HAL_I2C_Mem_Read(&hi2c2, HTS221_ADDR, HTS221_T0_OUT_L, 1, &t0_out_l, 1, 1);
    HAL_I2C_Mem_Read(&hi2c2, HTS221_ADDR, HTS221_T0_OUT_H, 1, &t0_out_h, 1, 1);
    HAL_I2C_Mem_Read(&hi2c2, HTS221_ADDR, HTS221_T1_OUT_L, 1, &t1_out_l, 1, 1);
    HAL_I2C_Mem_Read(&hi2c2, HTS221_ADDR, HTS221_T1_OUT_H, 1, &t1_out_h, 1, 1);

    uint16_t t0_degc = t0_degc_l | ((t_msb & 0x03) << 8); // t_msb[1:0] + t0_degc_l
    uint16_t t1_degc = t1_degc_l | ((t_msb & 0x0c) << 6); // t_msb[3:2] + t1_degc_l

    measeures.t0_degc = (t0_degc) >> 3; // division by 8
    measeures.t1_degc = (t1_degc) >> 3; // division by 8
    measeures.t0_out = ((int16_t)t0_out_h << 8) | t0_out_l;
    measeures.t1_out = ((int16_t)t1_out_h << 8) | t1_out_l;
    measeures.t_m = (float)(measeures.t1_degc - measeures.t0_degc) / ((float)(measeures.t1_out - measeures.t0_out));
    measeures.t_b = (float)measeures.t0_degc - measeures.t_m * (float)measeures.t0_out;

    // humidity measurements
    uint8_t h0_rh, h1_rh, h0_out_l, h0_out_h, h1_out_l, h1_out_h;
    HAL_I2C_Mem_Read(&hi2c2, HTS221_ADDR, HTS221_H0_RH, 1, &h0_rh, 1, 1);
    HAL_I2C_Mem_Read(&hi2c2, HTS221_ADDR, HTS221_H1_RH, 1, &h1_rh, 1, 1);
    HAL_I2C_Mem_Read(&hi2c2, HTS221_ADDR, HTS221_H0_OUT_L, 1, &h0_out_l, 1, 1);
    HAL_I2C_Mem_Read(&hi2c2, HTS221_ADDR, HTS221_H0_OUT_H, 1, &h0_out_h, 1, 1);
    HAL_I2C_Mem_Read(&hi2c2, HTS221_ADDR, HTS221_H1_OUT_L, 1, &h1_out_l, 1, 1);
    HAL_I2C_Mem_Read(&hi2c2, HTS221_ADDR, HTS221_H1_OUT_H, 1, &h1_out_h, 1, 1);

    measeures.h0_rh = (uint16_t)h0_rh >> 1; // division by 2
    measeures.h1_rh = (uint16_t)h1_rh >> 1; // division by 2
    measeures.h0_out = ((int16_t)h0_out_h << 8) | h0_out_l;
    measeures.h1_out = ((int16_t)h1_out_h << 8) | h1_out_l;
    measeures.h_m = (float)(measeures.h1_rh - measeures.h0_rh) / ((float)(measeures.h1_out - measeures.h0_out));
    measeures.h_b = (float)measeures.h0_rh - measeures.h_m * (float)measeures.h0_out;

    return measeures;
}

float hts221_read_temp(const HTS221CalibrationMeaseures* measeures) {
    uint8_t t_out_l, t_out_h;
    HAL_I2C_Mem_Read(&hi2c2, HTS221_ADDR, HTS221_T_OUT_L, 1, &t_out_l, 1, 1);
    HAL_I2C_Mem_Read(&hi2c2, HTS221_ADDR, HTS221_T_OUT_H, 1, &t_out_h, 1, 1);
    uint32_t ut_out = ((uint32_t)t_out_h << 8) | t_out_l;
    if (ut_out & 0x00008000) {
        ut_out |= 0xFFFF0000;
    }
    int32_t t_out = (int32_t)ut_out;
    return (float)t_out * measeures->t_m + measeures->t_b;
}

float hts221_read_hum(const HTS221CalibrationMeaseures* measeures) {
    uint8_t h_out_l, h_out_h;
    HAL_I2C_Mem_Read(&hi2c2, HTS221_ADDR, HTS221_H_OUT_L, 1, &h_out_l, 1, 1);
    HAL_I2C_Mem_Read(&hi2c2, HTS221_ADDR, HTS221_H_OUT_H, 1, &h_out_h, 1, 1);
    uint32_t uh_out = ((uint32_t)h_out_h << 8) | h_out_h;
    if (uh_out & 0x00008000) {
        uh_out |= 0xFFFF0000;
    }
    int32_t h_out = (int32_t)uh_out;
    return (float)h_out * measeures->h_m + measeures->h_b;
}

#define LSM6DSL_ADDR 0xd4
#define LSM6DSL_CTRL1_XL 0x10
#define LSM6DSL_CTRL2_G 0x11
#define LSM6DSL_X_L_G 0x22
#define LSM6DSL_X_H_G 0x23
#define LSM6DSL_Y_L_G 0x24
#define LSM6DSL_Y_H_G 0x25
#define LSM6DSL_Z_L_G 0x26
#define LSM6DSL_Z_H_G 0x27
#define LSM6DSL_X_L_XL 0x28
#define LSM6DSL_X_H_XL 0x29
#define LSM6DSL_Y_L_XL 0x2a
#define LSM6DSL_Y_H_XL 0x2b
#define LSM6DSL_Z_L_XL 0x2c
#define LSM6DSL_Z_H_XL 0x2d

void lsm6dsl_init(LSM6DSLXLUpdateRate accel_update_rate, LSM6DSLXLFullScale accel_full_scale, LSM6DSLGUpdateRate gyro_update_rate, LSM6DSLGFullScale gyro_full_scale) {
    uint8_t xl_payload = (accel_update_rate << 4) | (accel_full_scale << 2);
    HAL_I2C_Mem_Write(&hi2c2, LSM6DSL_ADDR, LSM6DSL_CTRL1_XL, 1, &xl_payload, 1, 1);
    uint8_t g_payload = (gyro_update_rate << 4) | (gyro_full_scale << 2);
    HAL_I2C_Mem_Write(&hi2c2, LSM6DSL_ADDR, LSM6DSL_CTRL2_G, 1, &g_payload, 1, 1);
}

static float lsm6dsl_read_component(uint16_t l_addr, uint16_t h_addr, float scale) {
    uint8_t l, h;
    HAL_I2C_Mem_Read(&hi2c2, LSM6DSL_ADDR, l_addr, 1, &l, 1, 1);
    HAL_I2C_Mem_Read(&hi2c2, LSM6DSL_ADDR, h_addr, 1, &h, 1, 1);
    uint32_t u_val = ((uint32_t)h << 8) | l;
    if (u_val & 0x00008000) {
        u_val |= 0xFFFF0000;
    }
    int32_t i_val = (int32_t)u_val;
    return (float)i_val * scale;
}

Vec3 lsm6dsl_read_accel() {
    uint8_t ctrl1;
    HAL_I2C_Mem_Read(&hi2c2, LSM6DSL_ADDR, LSM6DSL_CTRL1_XL, 1, &ctrl1, 1, 1);
    uint8_t fs_xl = (ctrl1 >> 2) & 0x03;
    float scale;
    switch (fs_xl) {
        case 0x00:
            scale = 0.061f;
            break;
        case 0x01:
            scale = 0.488f;
            break;
        case 0x02:
            scale = 0.122f;
            break;
        case 0x03:
            scale = 0.244f;
            break;
        default:
            scale = 1.0f;
    };

    const float mg_to_ms2 = 9.81f / 1000.f;
    Vec3 xl = {
        .x = lsm6dsl_read_component(LSM6DSL_X_L_XL, LSM6DSL_X_H_XL, scale) * mg_to_ms2,
        .y = lsm6dsl_read_component(LSM6DSL_Y_L_XL, LSM6DSL_Y_H_XL, scale) * mg_to_ms2,
        .z = lsm6dsl_read_component(LSM6DSL_Z_L_XL, LSM6DSL_Z_H_XL, scale) * mg_to_ms2,
    };

    return xl;
}

Vec3 lsm6dsl_read_gyro() {
    uint8_t ctrl2;
    HAL_I2C_Mem_Read(&hi2c2, LSM6DSL_ADDR, LSM6DSL_CTRL2_G, 1, &ctrl2, 1, 1);
    uint8_t fs_g = (ctrl2 >> 2) & 0x03;
    float scale;
    switch (fs_g) {
        case 0x00:
            scale = 8.75f;
        break;
        case 0x01:
            scale = 17.5f;
        break;
        case 0x02:
            scale = 35.0f;
        break;
        case 0x03:
            scale = 70.0f;
        break;
        default:
            scale = 1.0f;
    };

    Vec3 g = {
        .x = lsm6dsl_read_component(LSM6DSL_X_L_G, LSM6DSL_X_H_G, scale) / 1000.f,
        .y = lsm6dsl_read_component(LSM6DSL_Y_L_G, LSM6DSL_Y_H_G, scale) / 1000.f,
        .z = lsm6dsl_read_component(LSM6DSL_Z_L_G, LSM6DSL_Z_H_G, scale) / 1000.f,
    };

    return g;
}

#define LIS3MDL_ADDR 0x3c
#define LIS3MDL_CTRL1 0x20
#define LIS3MDL_CTRL2 0x21
#define LIS3MDL_CTRL3 0x22
#define LIS3MDL_CTRL4 0x23
#define LIS3MDL_X_L 0x28
#define LIS3MDL_X_H 0x29
#define LIS3MDL_Y_L 0x2a
#define LIS3MDL_Y_H 0x2b
#define LIS3MDL_Z_L 0x2c
#define LIS3MDL_Z_H 0x2d

void lis3mdl_init(LIS3MDLUpdateRate update_rate, LIS3MDLFullScale full_scale) {
    const uint8_t HIGH_PERF = 0x02;
    uint8_t ctrl1 = (HIGH_PERF << 5) | (update_rate << 2);
    HAL_I2C_Mem_Write(&hi2c2, LIS3MDL_ADDR, LIS3MDL_CTRL1, 1, &ctrl1, 1, 1);
    uint8_t ctrl2 = full_scale << 5;
    HAL_I2C_Mem_Write(&hi2c2, LIS3MDL_ADDR, LIS3MDL_CTRL2, 1, &ctrl2, 1, 1);
    uint8_t ctrl3 = 0;
    HAL_I2C_Mem_Write(&hi2c2, LIS3MDL_ADDR, LIS3MDL_CTRL3, 1, &ctrl3, 1, 1);
    uint8_t ctrl4 = HIGH_PERF << 2;
    HAL_I2C_Mem_Write(&hi2c2, LIS3MDL_ADDR, LIS3MDL_CTRL4, 1, &ctrl4, 1, 1);
}

static float lis3mdl_read_component(uint8_t addr_l, uint8_t addr_h, float scale) {
    uint8_t l, h;
    HAL_I2C_Mem_Read(&hi2c2, LIS3MDL_ADDR, addr_l, 1, &l, 1, 1);
    HAL_I2C_Mem_Read(&hi2c2, LIS3MDL_ADDR, addr_h, 1, &h, 1, 1);
    uint32_t uval = ((uint32_t)h << 8) | l;

    if (uval & 0x00008000) {
        uval |= 0xFFFF0000;
    }

    int32_t ival = (int32_t)uval;
    return ((float)ival) * scale;
}

Vec3 lis3mdl_read_mag() {
    uint8_t ctrl2;
    HAL_I2C_Mem_Read(&hi2c2, LIS3MDL_ADDR, LIS3MDL_CTRL2, 1, &ctrl2, 1, 1);
    uint8_t full_scale = (ctrl2 >> 5) & 0x03;
    float scale;
    switch (full_scale) {
        case 0x0:
            scale = 0.14f;
            break;
        case 0x1:
            scale = 0.29f;
            break;
        case 0x2:
            scale = 0.43f;
            break;
        case 0x3:
            scale = 0.58f;
            break;
        default:
            scale = 1.0f;
    };

    Vec3 mag = {
        .x = lis3mdl_read_component(LIS3MDL_X_L, LIS3MDL_X_H, scale),
        .y = lis3mdl_read_component(LIS3MDL_Y_L, LIS3MDL_Y_H, scale),
        .z = lis3mdl_read_component(LIS3MDL_Z_L, LIS3MDL_Z_H, scale),
    };

    return mag;
}

