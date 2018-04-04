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
#include <unistd.h>

#include "cluon-complete.hpp"
#include "opendlv-standard-message-set.hpp"

int32_t main(int32_t argc, char **argv) {
  int32_t retCode{0};
  auto commandlineArguments = cluon::getCommandlineArguments(argc, argv);
  if (0 == commandlineArguments.count("cid") || 0 == commandlineArguments.count("freq") || 0 == commandlineArguments.count("dev") || 0 == commandlineArguments.count("bus-address")) {
    std::cerr << argv[0] << " interfaces to an SRF02 ultrasonic distance sensor using i2c." << std::endl;
    std::cerr << "Usage:   " << argv[0] << " --dev=<I2C device node> --bus-address=<Sensor address on the i2c bus, in decimal format> --freq=<Parse frequency> --cid=<OpenDaVINCI session> [--id=<ID if more than one sensor>] [--verbose]" << std::endl;
    std::cerr << "Example: " << argv[0] << " --dev=/dev/i2c-0 --bus-address=112 --freq=10 --cid=111" << std::endl;
    retCode = 1;
  } else {
    uint32_t const ID{(commandlineArguments["id"].size() != 0) ? static_cast<uint32_t>(std::stoi(commandlineArguments["id"])) : 0};
    bool const VERBOSE{commandlineArguments.count("verbose") != 0};
    uint16_t const CID = std::stoi(commandlineArguments["cid"]);
    float const FREQ = std::stof(commandlineArguments["freq"]);

    std::string const devNode = commandlineArguments["dev"];
    int16_t deviceFile = open(devNode.c_str(), O_RDWR);
    if (deviceFile < 0) {
      std::cerr << "Failed to open the i2c bus." << std::endl;
      return 1;
    }

    uint8_t const address = std::stoi(commandlineArguments["bus-address"]);

    int8_t status = ioctl(deviceFile, I2C_SLAVE, address);
    if (status < 0) {
      std::cerr << "Could not acquire bus access for device " << devNode << "." << std::endl;
      return 1;
    }

    uint8_t parseFirmwareBuffer[1];
    parseFirmwareBuffer[0] = 0x00;
    status = write(deviceFile, parseFirmwareBuffer, 1);
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

    std::cout << "Connected with the SRF08 device on " << devNode << ". Reported firmware version '" << static_cast<int32_t>(firmwareBuffer[0]) << "'." << std::endl;

    cluon::OD4Session od4{CID};

    uint8_t commandBuffer[2];
    commandBuffer[0] = 0x00;
    commandBuffer[1] = 0x51;

    auto atFrequency{[&deviceFile, &commandBuffer, &ID, &VERBOSE, &od4]() -> bool
      {
        uint8_t rangeHiReg{0x02};
        uint8_t rangeLoReg{0x03};
        uint8_t lightSensorReg{0x01};
	    
        uint8_t rangeHi;
        uint8_t rangeLo;
        uint8_t lightSensor;

        uint8_t res = write(deviceFile, commandBuffer, 2);
        if (res != 2) {
          std::cerr << "Could not write ranging request." << std::endl;
          return false;
        }

        std::this_thread::sleep_for(std::chrono::duration<double>(0.07));

        res = write(deviceFile, &rangeHiReg, 1);
        res += read(deviceFile, &rangeHi, 1);
        res += write(deviceFile, &rangeLoReg, 1);
        res += read(deviceFile, &rangeLo, 1);
        res += write(deviceFile, &lightSensorReg, 1);
        res += read(deviceFile, &lightSensor, 1);
        if (res != 6) {
          std::cerr << "Could not read data." << std::endl;
          return false;
        }

        uint32_t rangeCm = (rangeHi << 8) + rangeLo;
        float distance = static_cast<float>(rangeCm) / 100.0f;

        float lumen = static_cast<float>(lightSensor) / 248.0f * 1000.0f;

        opendlv::proxy::DistanceReading distanceReading;
        distanceReading.distance(distance);

        cluon::data::TimeStamp sampleTime;
        od4.send(distanceReading, sampleTime, ID);
        if (VERBOSE) {
          std::cout << "SRF08 distance reading is " << distanceReading.distance() << " m. Brightness is " << lumen << " lumen." << std::endl;
        }
        
        return true;
      }};

    od4.timeTrigger(FREQ, atFrequency);
  }
  return retCode;
}
