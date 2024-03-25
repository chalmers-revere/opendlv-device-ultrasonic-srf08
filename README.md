Moved to https://git.opendlv.org.

# opendlv-device-ultrasonic-srf08
OpenDLV Microservice to interface with a Devantech SRF08 Ultrasonic Range Finder over i2c.

[![License: GPLv3](https://img.shields.io/badge/license-GPL--3-blue.svg
)](https://www.gnu.org/licenses/gpl-3.0.txt)

## Table of Contents
* [Dependencies](#dependencies)
* [License](#license)

## Dependencies
You need a C++14-compliant compiler to compile this project. This repository
ships the following dependencies as part of the source distribution:

* [libcluon](https://github.com/chrberger/libcluon) - [![License: GPLv3](https://img.shields.io/badge/license-GPL--3-blue.svg
)](https://www.gnu.org/licenses/gpl-3.0.txt)
* [Unit Test Framework Catch2](https://github.com/catchorg/Catch2/releases/tag/v2.1.2) - [![License: Boost Software License v1.0](https://img.shields.io/badge/License-Boost%20v1-blue.svg)](http://www.boost.org/LICENSE_1_0.txt)


## Devantech address flashing

1. Build the binary in tools/

` cd tools && make`

2. Run following command

`./devantech_change_addr 1 0x70 0x71`

to change the back sensor on the i2c-1 bus from addr 0x70 to 0x71. When the command its executed, the led flash on the sensor should be lit up upon success. Unplug and plug the sensor again, when booting up, you should see the sensor flashing the led twice. Now plug in the front sensor again. You will also see that it flashes once

## License

* This project is released under the terms of the GNU GPLv3 License
