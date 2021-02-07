# 2021 Update - Archiving
We stopped working on the project, therefore we are no longer mantaining the drivers/changes contained in thi repo. Archiving.

# Mitreo project 2018/2019 notes - Politecnico di Milano
We developed a new board for the platform srf06-cc26xx, named launchpad-bp (bp stands for BoosterPack).
It can be found at arch/platform/srf06-cc26xx/launchpad-bp/. The only board supported as of now is TI CC1350.

Drivers for BOSCH sensors are from the producer (https://github.com/BoschSensortec), updated to february 2018.
Drivers for TI sensors (temperature and optical) are based on the ones for TI SensorTag (arch/platform/simplelink/cc13xx-cc26xx/sensortag/).

The code is updated to the latest stable version of Contiki-NG, version 4.2 at the time of writing (commit 7b076e4af14b2259596f6d3765105d0593ae5257).

Examples for each sensor and a demo of the application with further instructions can be found on the examples folder under platform-specific/cc1350-bp.

### List of changes
#### Transmission frequency
To send data between different devices we use the 433Mhz frequency, it isn’t enabled in Contiki-NG by default. In order to change it one need to set the following define in any config.h header

```
DOT_15_4G_CONF_FREQUENCY_BAND_ID id
```

id can be found in arch/cpu/cc26xx-cc13xx/rf-core/dot-15-4g.h

#### Sensors
In order to use sensors with the CC1350 LaunchPad we need to import the following files

 | Imported files                       | Description                                                  |
 | ------------------------------------ |--------------------------------------------------------------|
 | board-i2c.h, board-i2c.c             | I2C library                                                  |
 | sensor-common.h, sensor-common.c     | RW functions for I2C                                         |
 | sensortag-sensors.c                  | Allow integrations of new drivers throught Contiki-NG's macro|
 | tmp-007-sensor.h, tmp-007-sensor.c   | Contiki-NG TMP007 drivers                                    |
 | opt-3001-sensor.h, opt-3001-sensor.c | Contiki-NG OPT2001 drivers                                   |
 | bmi160.h, bmi160.c, bmi160_defs.h    | Official BOSCH drivers                                       |
 | bme280.h, bme280.c, bme280_defs.h    | Official BOSCH drivers                                       |

##### Hardware Configurations

The following configurations allow the communication between senors boosterpack and board CC1350

 |              | TMP007 | OPT3001 | BME280 | BMI160 |
 | -------------|--------|---------|--------|--------|
 | HW Protocol  |  I2C   | I2C     | I2C    |I2C     |
 | I2C Address  | 0x40   | 0x47    | 0x77   | 0x69   |
 | Interface    |   0    |   0     | 0      | 0      | 
 | Mode         | Normal | Sleep   | Normal | Normal |
 
 ##### BME280 Configuration
 
 The following configurations describe the mode of sampling of bme280. Others can be found in the official Bosch datasheet 
 and modified in arch/platform/srf06-cc26xx/launchpad-bp/bme280.c at function configure
 
  | Settings               | Value                                                   
  |------------------------|--------------------|
  | Pressure Oversample    | 1                  |                                                   
  | Temperature Oversample | 1                  |
  | Humidity Oversample    | 1                  |
  | Filter                 | 0                  |
  | Standby Time           | 0 (0.5 ms)         |
  | Encoding Data          | 32 bit             |
  | Notes                  | In order to save clock cicle and energy it's better use 32 bit encoding data                                     |

##### BMI160 Configuration
 
 The following configurations describe the mode of sampling of bmi160. Others can be found in the official Bosch datasheet 
 and modified in arch/platform/srf06-cc26xx/launchpad-bp/bmi160.c at function configure_bmi
 
  | Settings                        | Value                                                   
  |---------------------------------|----------------------------|
  | Output data rate accelerometer  | BMI160_ACCEL_ODR_1600HZ    |                                                  
  | Range accelerometer             | BMI160_ACCEL_RANGE_2G      |
  | Bandwidth accelerometer         | BMI160_ACCEL_BW_NORMAL_AVG4|
  | Mode accelerometer              | BMI160_ACCEL_NORMAL_MODE   |
  | Output data rate gyroscope      | BMI160_GYRO_ODR_3200HZ     |
  | Range gyroscope                 | BMI160_GYRO_RANGE_2000_DPS |
  | Bandwidth gyroscope             | BMI160_GYRO_BW_NORMAL_MODE |
  | Mode gyroscope                  | BMI160_GYRO_NORMAL_MODE    |

# Contiki-NG: The OS for Next Generation IoT Devices

[![Build Status](https://travis-ci.org/contiki-ng/contiki-ng.svg?branch=master)](https://travis-ci.org/contiki-ng/contiki-ng/branches)
[![license](https://img.shields.io/badge/license-3--clause%20bsd-brightgreen.svg)](https://github.com/contiki-ng/contiki-ng/blob/master/LICENSE.md)
[![Latest release](https://img.shields.io/github/release/contiki-ng/contiki-ng.svg)](https://github.com/contiki-ng/contiki-ng/releases/latest)
[![GitHub Release Date](https://img.shields.io/github/release-date/contiki-ng/contiki-ng.svg)](https://github.com/contiki-ng/contiki-ng/releases/latest)
[![Last commit](https://img.shields.io/github/last-commit/contiki-ng/contiki-ng.svg)](https://github.com/contiki-ng/contiki-ng/commit/HEAD)

Contiki-NG is an open-source, cross-platform operating system for Next-Generation IoT devices. It focuses on dependable (secure and reliable) low-power communication and standard protocols, such as IPv6/6LoWPAN, 6TiSCH, RPL, and CoAP. Contiki-NG comes with extensive documentation, tutorials, a roadmap, release cycle, and well-defined development flow for smooth integration of community contributions.

Unless excplicitly stated otherwise, Contiki-NG sources are distributed under
the terms of the [3-clause BSD license](LICENSE.md). This license gives
everyone the right to use and distribute the code, either in binary or
source code format, as long as the copyright license is retained in
the source code.

Contiki-NG started as a fork of the Contiki OS and retains some of its original features.

Find out more:

* GitHub repository: https://github.com/contiki-ng/contiki-ng
* Documentation: https://github.com/contiki-ng/contiki-ng/wiki
* Web site: http://contiki-ng.org

Engage with the community:

* Gitter: https://gitter.im/contiki-ng
* Twitter: https://twitter.com/contiki_ng
