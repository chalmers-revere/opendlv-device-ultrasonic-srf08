/*
 * Copyright (C) 2018 Ola Benderius
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <fstream>
#include <vector>

#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include "cluon-complete.hpp"
#include "opendlv-standard-message-set.hpp"

int32_t main(int32_t argc, char **argv) {
  int32_t retCode{0};
  auto commandlineArguments = cluon::getCommandlineArguments(argc, argv);
  if (0 == commandlineArguments.count("cid") || 0 == commandlineArguments.count("freq") || 0 == commandlineArguments.count("dev") || 0 == commandlineArguments.count("bus-address")) {
    std::cerr << argv[0] << " interfaces to an SRF02 ultrasonic distance sensor using i2c." << std::endl;
    std::cerr << "Usage:   " << argv[0] << " --dev=<I2C device node> --bus-address=<Sensor address on the i2c bus> --freq=<Parse frequency> --cid=<OpenDaVINCI session> [--id=<ID if more than one sensor>] [--verbose]" << std::endl;
    std::cerr << "Example: " << argv[0] << " --dev=/dev/i2c-0 --bus-address=0x70 --freq=10 --cid=111" << std::endl;
    retCode = 1;
  } else {
    uint32_t const ID{(commandlineArguments["id"].size() != 0) ? static_cast<uint32_t>(std::stoi(commandlineArguments["id"])) : 0};
    bool const VERBOSE{commandlineArguments.count("verbose") != 0};

    std::string const devNode = commandlineArguments["dev"];
    int16_t deviceFile = open(devNode.c_str(), O_RDWR);
    if (deviceFile < 0) {
      std::cerr << "Failed to open the i2c bus." << std::endl;
      return 1;
    }

    uint8_t const address = std::stoi(commandlineArguments["bus-address"]);

    uint8_t parseFirmwareBuffer[2];
    parseFirmwareBuffer[0] = address;
    parseFirmwareBuffer[1] = 0x00;
    uint8_t status = write(deviceFile, parseFirmwareBuffer, 2);
    if (status != 1) {
      std::cerr << "Could not write firmware request to device on " << devNode << "." << std::endl;
      return 1;
    }

    uint8_t firmwareBuffer[1];
    status = read(deviceFile, firmwareBuffer, 1);
    if (status != 1) {
      std::cerr << "Could not read firmware from device on " << devNode << "." << std::endl;
      return 1;
    }

    std::cout << "Connected with the SRF08 device on " << devNode << ". Reported firmware " << firmwareBuffer[0] << "." << std::endl;

    uint8_t commandBuffer[3];
    commandBuffer[0] = address;
    commandBuffer[1] = 0x00;
    commandBuffer[2] = 0x51;

    uint8_t parseRangeBuffer[2];
    parseRangeBuffer[0] = address;

    uint8_t rangeBuffer[2];

    cluon::OD4Session od4{static_cast<uint16_t>(std::stoi(commandlineArguments["cid"])), [](auto){}};

    double dt = 1.0 / std::stoi(commandlineArguments["freq"]);
    while (od4.isRunning()) {
      std::this_thread::sleep_for(std::chrono::duration<double>(dt));

      status = write(deviceFile, commandBuffer, 3);
      if (status != 1) {
        std::cerr << "Could not write ranging request." << std::endl;
      }

      uint32_t rangeCount = 0;
      uint32_t sumRangeCm = 0;
      for (uint32_t i = 0; i < 16; i++) {
        parseRangeBuffer[1] = 2 + i * 2;
        status = write(deviceFile, parseRangeBuffer, 2);
        status += read(deviceFile, rangeBuffer, 1);

        parseRangeBuffer[1] = 2 + i * 2 + 1;
        status += write(deviceFile, parseRangeBuffer, 2);
        status += read(deviceFile, rangeBuffer + 1, 1);
        
        if (status != 4) {
          std::cerr << "Could not read range." << std::endl;
          return 1;
        }

        uint32_t rangeCm = (rangeBuffer[0] << 8) + rangeBuffer[1];
        if (rangeCm == 0) {
          break;
        }

        sumRangeCm += rangeCm;
        rangeCount++;
        std::cout << rangeCm << std::endl;
      }

      float distance = static_cast<float>(0.01f * sumRangeCm / rangeCount);

      opendlv::proxy::DistanceReading distanceReading;
      distanceReading.distance(distance);

      cluon::data::TimeStamp sampleTime;
      od4.send(distanceReading, sampleTime, ID);
      if (VERBOSE) {
        std::cout << "Sensor reading is " << distanceReading.distance() << " m." << std::endl;
      }
    }
  }
  return retCode;
}
