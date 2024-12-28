# STM32L4 Discovery kit for IoT node drivers
This is a collection of drivers that I made for the IoT node discovery kit [B-L475E-IOT01A](https://www.st.com/en/evaluation-tools/b-l475e-iot01a.html).

## How to use
Add the ```IOT01A1-Drivers``` directory to the ```Core``` directory of your project and include the necessary headers.

### ISM43362 WiFi module
For this module, you need to include the ```ism43362.h``` header and the SPI3 must be configured to have the **DataSize at 16 bits**, if you want to use a different size, you will have to modify the ```ism43362_transmit_buffer``` function, considering the module requires 16 bits at a time in big endian. Here is an example configuration that worked for me.

```c
    hspi3.Instance = SPI3;
    hspi3.Init.Mode = SPI_MODE_MASTER;
    hspi3.Init.Direction = SPI_DIRECTION_2LINES;
    hspi3.Init.DataSize = SPI_DATASIZE_16BIT;
    hspi3.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi3.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi3.Init.NSS = SPI_NSS_SOFT;
    hspi3.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
    hspi3.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi3.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi3.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi3.Init.CRCPolynomial = 7;
    hspi3.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
    hspi3.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
```

You also need to configure the EXTI for the ```ISM43362_DRDY_EXTI1_Pin``` on rising edge, then in the callback call ```ism43362_drdy_exti_callback()```, here is an example.

```c
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == ISM43362_DRDY_EXTI1_Pin) {
        ism43362_drdy_exti_callback();
    }
}
```

You can also enable logging to the ```huart1``` by defining the macro ```USART1_LOG``` in ```ism43362.c```.

Here is an example where a UDP server is hosted on socket 0 on port 5000, where every packet received is sent to 192.168.1.13:6000 through TCP using socket 1.

```c
    // init spi3 before using the module
    // you should also ALWAYS reset the module before using it
    ism43362_reset_module();

    JoinWifiConfig c = ism43362_get_default_wifi_config();
    snprintf(c.ssid, sizeof(c.ssid), "SSID");
    snprintf(c.password, sizeof(c.password), "PASSWORD");
    c.security = WPA2;
    c.dhcp = true;
    c.country_code = FR_0; // FR_0 is used for every EU state
    ism43362_join_network(&c);

    WifiBaseServerConfig server_config = ism43362_get_default_base_server_config();
    server_config.local_port = 5000;
    server_config.s = SOCKET_0;
    ism43362_start_udp_server(&server_config);

    WifiClientConfig client_config = ism43362_get_default_client_config();
    client_config.protocol = TCP;
    client_config.s = SOCKET_1;
    client_config.remote.ip[0] = 192;
    client_config.remote.ip[1] = 168;
    client_config.remote.ip[2] = 1;
    client_config.remote.ip[3] = 13;
    client_config.remote.port = 6000;
    ism43362_start_wifi_client(&client_config);

    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1) {
        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */
        ism43362_set_socket(SOCKET_0);
        uint8_t buff[2000] = {0};
        size_t read_len;
        ism43362_read(buff, sizeof(buff), &read_len);
        if (read_len > 0) {
          WifiRemote remote;
          ism43362_get_remote(&remote);
          char msg[200];
          snprintf(msg, sizeof(msg), "%d.%d.%d.%d:%d said '%s'\n", remote.ip[0], remote.ip[1], remote.ip[2], remote.ip[3], remote.port, (const char*)buff);
          ism43362_set_socket(SOCKET_1);
          ism43362_send(msg, strlen(msg));
        }
    }
    /* USER CODE END 3 */
```

As of now, the driver doesn't support the access point mode.

### Sensors
To use sensors, you will need to enable I2C2, here is a configuration (the default by CubeMX) that worked for me.

```c
    hi2c2.Instance = I2C2;
    hi2c2.Init.Timing = 0x10D19CE4;
    hi2c2.Init.OwnAddress1 = 0;
    hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c2.Init.OwnAddress2 = 0;
    hi2c2.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
    hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
```

All of the sensors' interfaces are contained in ```sensors.h```, here's a list of implemented sensors:
- **LPS22HB**, pressure and temperature sensor
- **HTS221**, humidity and temperature sensor
- **LSM6DSL**, 3D accelerometer and gyroscope
- **LIS3MDL**, 3D magnetometer

To read each sensor:

```c
    // init i2c2 before using the sensors
    lps22hb_init(LPS_HZ_25); // sensor update rate
    // the hts221 contains calibration measurements that are different between each sensor, needed to 
    // compute the actual values
    HTS221CalibrationMeaseures m = hts221_init(HTS_HZ_12_5); // sensor update rate
    lsm6dsl_init(XL_104_HZ, XL_4_G, G_104_HZ, G_500_DPS); // sensor update rate and full scale
    lis3mdl_init(LIS_40_HZ, LIS_4_GAUSS); // sensor update rate and full scale

    
    float pressure = lps22hb_read_press(); 
    float lps_temperature = lps22hb_read_temp();
    // for the hts you need to pass the calibration measurements
    float humidity = hts221_read_hum(&m);
    float hts_temperature = hts221_read_temp(&m);
    Vec3 accel = lsm6dsl_read_accel();
    Vec3 gyro = lsm6dsl_read_gyro();
    Vec3 magnetometer = lis3mdl_read_mag();
```