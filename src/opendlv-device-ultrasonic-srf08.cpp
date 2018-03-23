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

#include "cluon-complete.hpp"
#include "opendlv-standard-message-set.hpp"

int32_t main(int32_t argc, char **argv) {
  int32_t retCode{0};
  auto commandlineArguments = cluon::getCommandlineArguments(argc, argv);
  if (0 == commandlineArguments.count("cid") || 0 == commandlineArguments.count("freq")) {
    std::cerr << argv[0] << " interfaces to the SRF08 ultrasonic sensor." << std::endl;
    std::cerr << "Usage:   " << argv[0] << " --freq=<frequency> --cid=<OpenDaVINCI session> [--id=<Identifier in case of multiple sensors] [--verbose]" << std::endl;
    std::cerr << "Example: " << argv[0] << " --freq=10 --cid=111" << std::endl;
    retCode = 1;
  } else {
    const uint32_t ID{(commandlineArguments["id"].size() != 0) ? static_cast<uint32_t>(std::stoi(commandlineArguments["id"])) : 0};
    const bool VERBOSE{commandlineArguments.count("verbose") != 0};

    cluon::OD4Session od4{static_cast<uint16_t>(std::stoi(commandlineArguments["cid"])),
      [](auto){}
    };

    double timeToSleep = 1.0 / std::stoi(commandlineArguments["freq"]);
    while (od4.isRunning()) {
      std::this_thread::sleep_for(std::chrono::duration<double>(timeToSleep));

      opendlv::proxy::DistanceReading distanceReading;
      cluon::data::TimeStamp sampleTime;
      od4.send(distanceReading, sampleTime, ID);
      if (VERBOSE) {
        std::cout << "Distance reading sent." << std::endl;
      }
    }
  }
  return retCode;
}
