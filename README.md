# Mitreo project 2018/2019 notes - Politecnico di Milano
We developed a new board for the platform srf06-cc26xx, named launchpad-bp (bp stands for BoosterPack).
It can be found at arch/platform/srf06-cc26xx/launchpad-bp/. The only board supported as of now is TI CC1350.

Drivers for BOSCH sensors are from the producer (https://github.com/BoschSensortec), updated to february 2018.
Drivers for TI sensors (temperature and optical) are based on the ones for TI SensorTag (arch/platform/simplelink/cc13xx-cc26xx/sensortag/).

The code is updated to the latest stable version of Contiki-NG, version 4.2 at the time of writing (commit 7b076e4af14b2259596f6d3765105d0593ae5257).

Examples for each sensor and a demo of the application with further instructions can be found on the examples folder under platform-specific/cc1350-bp.

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
