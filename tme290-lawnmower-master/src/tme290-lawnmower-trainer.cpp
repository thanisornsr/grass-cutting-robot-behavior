/*
 * Copyright (C) 2019 Ola Benderius
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

#include <iostream>

#include "cluon-complete.hpp"
#include "tinyso.hpp"
#include "tme290-sim-grass-msg.hpp"

tme290::grass::Control step(tme290::grass::Sensors sensors,
    tinyso::Individual const &ind) {

  // Note that the number of variables per individual can be changed.
  // Also note that they can be cast to integers, or whatever
  // data is needed, e.g. int32_t x = static_cast<int32_t>(v0).

  double v0 = ind[0] * (sensors.i() + 1);
  // double v1 = ind[1];
  // double v2 = ind[2];
  // double v3 = ind[3];
  // double v4 = ind[4];
  // double v5 = ind[5];

  tme290::grass::Control control;
  if (v0 < 0.25) {
    control.command(1);
  } else if (v0 < 0.5) {
    control.command(2);
  } else if (v0 < 0.75) {
    control.command(3);
  } else if (v0 < 1.0) {
    control.command(4);
  } else {
    control.command(0);
  }
  
  return control;
}

int32_t main(int32_t argc, char **argv) {
  int32_t retCode{0};
  auto commandlineArguments = cluon::getCommandlineArguments(argc, argv);
  if (0 == commandlineArguments.count("j")) {
    std::cerr << argv[0] 
      << " is a trainer for a lawn mower control algorithm." << std::endl;
    std::cerr << "Usage:   " << argv[0] 
      << " --j=<Number of parallel threads (simulations)>" 
      << " [--cid-start=<CID interval start (end: cid-start+j). Default: 111>]" 
      << " [--verbose]" << std::endl;
    std::cerr << "Example: " << argv[0] << " --j=2 --verbose" << std::endl;
    retCode = 1;
  } else {
    bool const verbose = (commandlineArguments.count("verbose") != 0);
    uint16_t const jobs = std::stoi(commandlineArguments["j"]);
    uint32_t const cidStart = (commandlineArguments.count("cid-start") != 0) 
      ? std::stoi(commandlineArguments["cid-start"]) : 111;

    uint32_t generationCount = 10000;
    tinyso::CrossoverMethod crossoverMethod = tinyso::CrossoverMethod::Split;
    uint32_t individualLength = 6;
    uint32_t eliteSize = 1;
    uint32_t populationSize = 30;
    uint32_t tournamentSize = 2;
    float probCrossover = 0.3f;
    float probMutation = 0.1f;
    float probSelectTournament = 0.8f;
    
    uint32_t simMaxTime = 4000;
    bool randomSeed = true;

    std::random_device rd;
    std::mt19937 rg(rd());

    bool terminate{false};

    std::map<uint32_t, std::mutex> cidLocks;
    for (uint32_t i{0}; i < jobs; i++) {
      uint32_t cid = cidStart + i;
      cidLocks.emplace(std::piecewise_construct, std::make_tuple(cid),
          std::make_tuple());
    }

    auto evaluate{[&cidStart, &jobs, &cidLocks, &simMaxTime, &rg, &randomSeed, &terminate](
        tinyso::Individual const &ind, uint32_t const) -> double
      {
        uint32_t cid = cidStart;
        while (!cidLocks[cid].try_lock()) {
          std::this_thread::sleep_for(std::chrono::milliseconds(10));
          cid++;
          if (cid > cidStart + jobs) {
            cid = cidStart;
          }
        }

        double eta{1.0};
        {
          cluon::OD4Session od4(cid);
          bool isRunning{true};

          auto onSensors{[&od4, &ind, &isRunning, &simMaxTime](
              cluon::data::Envelope &&envelope)
            {
              auto msg = cluon::extractMessage<tme290::grass::Sensors>(
                  std::move(envelope));

              if ((msg.time() > simMaxTime) || (msg.battery() <= 0.0)) {
                isRunning = false;
                return;
              }

              auto control = step(msg, ind);
              od4.send(control);
            }};

          auto onStatus{[&eta](
              cluon::data::Envelope &&envelope)
            {
              auto msg = cluon::extractMessage<tme290::grass::Status>(
                  std::move(envelope));
              eta = msg.grassMax() * msg.grassMean();
            }};

          od4.dataTrigger(tme290::grass::Sensors::ID(), onSensors);
          od4.dataTrigger(tme290::grass::Status::ID(), onStatus);

          tme290::grass::Control control;
          control.command(0);
          od4.send(control);

          while (isRunning && od4.isRunning()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
          }

          if (!od4.isRunning()) {
            terminate = true;
          }

          tme290::grass::Restart restart;
          if (!randomSeed) {
            restart.seed(1234);
          } else {
            std::uniform_int_distribution<unsigned long long> dist;
            restart.seed(dist(rg));
          }
          od4.send(restart);

          cidLocks[cid].unlock();
        }
        return 1.0 / eta;
      }};


    if (verbose) {
      std::cout << "Starting the training using " << jobs << " threads." 
        << std::endl;
    }

    tinyso::GeneticAlgorithm ga(evaluate, crossoverMethod, individualLength, 
        eliteSize, populationSize, tournamentSize, probCrossover, probMutation, 
        probSelectTournament);

    for (uint32_t i{0}; i < generationCount && !terminate; i++) {
      ga.NextGeneration(jobs);

      if (verbose) {
        auto bestInd = ga.GetBestIndividual();
        std::cout << " .. generation " << i << ", best fitness " 
          << ga.GetBestFitness() << " (" << bestInd[0];
        for (uint32_t j{1}; j < individualLength; j++) {
          std::cout << ", " << bestInd[j];
        }
        std::cout << ")" << std::endl;
      }
    }
      
    if (verbose) {
      auto bestInd = ga.GetBestIndividual();
      std::cout << "Training done, best fitness " 
        << ga.GetBestFitness() << std::endl;
      for (uint32_t i{0}; i < individualLength; i++) {
        std::cout << "  param " << i << ": " << bestInd[i] << std::endl;
      }
    }
    retCode = 0;
  }
  return retCode;
}
