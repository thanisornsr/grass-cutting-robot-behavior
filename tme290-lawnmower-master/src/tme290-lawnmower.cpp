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
#include "tme290-sim-grass-msg.hpp"

// here is to define parameter
const int stateRobotOn = 0;
const int stateDecideNext =1;
const int stateMoving = 2;
const int stateStayAndCut = 3;
const int stateGoBackHome = 4;
const int stateCharging = 5;
const int stateGoToLastPoint = 6;



int32_t myState;
float myGrass;
float myRain;
float myBattery;
int myPosI;
int myPosJ;
float myTargetCut;
int myJustMove;
int myGoingHome;
int myCharging;
float myBatteryToHome;
int myLastPosI;
int myLastPosJ;
int myAtLastPos;
int myDirectionNext;

void foo(){




  myState = stateRobotOn;
  myBatteryToHome = 0.2f;
  myJustMove = 0;
  myGoingHome = 0;
  myCharging = 0;
  myPosI = 0;
  myPosJ = 0;
  myAtLastPos = 0;
  myTargetCut = 0.3f;
  myState = stateDecideNext;
}


int32_t main(int32_t argc, char **argv) {
  int32_t retCode{0};
  auto commandlineArguments = cluon::getCommandlineArguments(argc, argv);
  if (0 == commandlineArguments.count("cid")) {
    std::cerr << argv[0] 
      << " is a lawn mower control algorithm." << std::endl;
    std::cerr << "Usage:   " << argv[0] << " --cid=<OpenDLV session>" 
      << "[--verbose]" << std::endl;
    std::cerr << "Example: " << argv[0] << " --cid=111 --verbose" << std::endl;
    retCode = 1;
  } else {
    foo();

    bool const verbose{commandlineArguments.count("verbose") != 0};
    uint16_t const cid = std::stoi(commandlineArguments["cid"]);
    
    cluon::OD4Session od4{cid};
    // Functions
    auto updateDirectionNext{[&od4](cluon::data::Envelope &&envelope){
      float maxGrassNear = 0.0f;
      int maxGrassDir = 0;
      auto msg = cluon::extractMessage<tme290::grass::Sensors>(
            std::move(envelope));      
      //next is the maximum grass direction
      // check 1
      if (msg.grassTopLeft() > maxGrassNear) {
          maxGrassNear = msg.grassTopLeft();
          maxGrassDir = 1;
      }
      // check 2
      if (msg.grassTopCentre() > maxGrassNear) {
          maxGrassNear = msg.grassTopCentre();
          maxGrassDir = 2;
      }
      // check 3
      if (msg.grassTopRight() >= maxGrassNear) {
          maxGrassNear = msg.grassTopRight();
          maxGrassDir = 3;
      }
      // check 4
      if (msg.grassRight() > maxGrassNear) {
          maxGrassNear = msg.grassRight();
          maxGrassDir = 4;
      }
      // check 5
      if (msg.grassBottomRight() > maxGrassNear) {
          maxGrassNear = msg.grassBottomRight();
          maxGrassDir = 5;
      }
      // check 6
      if (msg.grassBottomCentre() > maxGrassNear) {
          maxGrassNear = msg.grassBottomCentre();
          maxGrassDir = 6;
      }
      // check 7
      if (msg.grassBottomLeft() > maxGrassNear) {
          maxGrassNear = msg.grassBottomLeft();
          maxGrassDir = 7;
      }
      // check 8
      if (msg.grassLeft() > maxGrassNear) {
          maxGrassNear = msg.grassLeft();
          maxGrassDir = 8;
      }
      myDirectionNext = maxGrassDir;


    }};


    auto decideNext{[&od4](cluon::data::Envelope &&envelope)
      {
        auto msg = cluon::extractMessage<tme290::grass::Sensors>(
            std::move(envelope));
        myRain = msg.rain();
        myBattery = msg.battery();
        tme290::grass::Control control;
        if (myBattery < myBatteryToHome){
          // Low battery >> Go home
          std::cout << "Low Battery, Going Home" << std::endl;
          //Remember last point
          myLastPosI = myPosI;
          myLastPosJ = myPosJ;
          myState = stateGoBackHome;
        }else{
          if (myAtLastPos == 1){
            //Going to last pos
            myState = stateGoToLastPoint;
          }else{
            if(myRain >= 0.2f){
              //Raining So hard >> Stay and do nothing
              std::cout << "Let's Raining. I'll wait..." << std::endl;

              control.command(0);
              od4.send(control);
            }else{
              //Not Raining + Good Battery >> Decide to move or to cut
              if (myJustMove == 1){
                //It is time to cut!!
                myJustMove = 0;
                myState = stateStayAndCut;
              }else{
                //Let make some move!!
                myState = stateMoving;
              }
            }
          }
        }
    }};


    auto movingState{[&od4](cluon::data::Envelope &&envelope)
    {
      updateDirectionNext();
      auto msg = cluon::extractMessage<tme290::grass::Sensors>(
            std::move(envelope));
      tme290::grass::Control control;
      std::cout << "Moving" << std::endl;
      switch(myDirectionNext){
        case 1:
          control.command(1);
          od4.send(control);
          myPosI = myPosI-1;
          myPosJ = myPosJ-1;
          break;
        case 2:
          control.command(2);
          od4.send(control);
          myPosJ = myPosJ-1;
          break;
        case 3:
          control.command(3);
          od4.send(control);
          myPosI = myPosI+1;
          myPosJ = myPosJ-1;
          break;
        case 4:
          control.command(4);
          od4.send(control);
          myPosI = myPosI+1;
          break;
        case 5:
          control.command(5);
          od4.send(control);
          myPosI = myPosI+1;
          myPosJ = myPosJ+1;
          break;
        case 6:
          control.command(6);
          od4.send(control);
          myPosJ = myPosJ+1;
          break;
        case 7:
          control.command(7);
          od4.send(control);
          myPosI = myPosI-1;
          myPosJ = myPosJ+1;
          break;
        case 8:
          control.command(1);
          od4.send(control); 
          myPosI = myPosI-1;
          break;
      }
      //Done Moving and update current position
      //Then calculate minimum battery
      int myDistanceToHome = myPosI + myPosJ;
      myBatteryToHome = (float) myDistanceToHome * 0.02f; // This 0.02 are battery drain per one step

      myJustMove = 1;
      myState = stateDecideNext;
    }};

    auto stayAndCutState{[&od4](cluon::data::Envelope &&envelope)
      {
        auto msg = cluon::extractMessage<tme290::grass::Sensors>(
            std::move(envelope));
        myBattery = msg.battery();
        tme290::grass::Control control;
        if (myBattery < myBatteryToHome){
          //Low battery Gohome!
          std::cout << "Low Battery, Going Home From cut" << std::endl;
          //Remember last point
          myLastPosI = myPosI;
          myLastPosJ = myPosJ;
          control.command(0);
          od4.send(control); 
          myState = stateGoBackHome;
        }else{
          //Check center grass
          if (msg.grassCentre() >= myTargetCut){
            // Keep cutting
            std::cout << "Cutting..." << std::endl;
            control.command(0);
            od4.send(control);            
          }else{
            // Done cutting
            std::cout << "Cut Done" << std::endl;
            myState = stateDecideNext;
            control.command(0);
            od4.send(control); 
          }
        }
      }};

    auto goingHomeState{[&od4](cluon::data::Envelope &&envelope)
      {
        auto msg = cluon::extractMessage<tme290::grass::Sensors>(
            std::move(envelope));
        tme290::grass::Control control;
        if (myPosI == 0 and myPosJ == 0){
          //Reach Home Move to Charging
          control.command(0);
          od4.send(control);
          myState = stateCharging;
          myGoingHome = 0;
          std::cout << "I'm Home" << std::endl;
        }else{
          myGoingHome = 1;
          std::cout << "Going Home" << std::endl;
          //Not home yet, Keep moving
          if (myPosJ >= 20) {
            //Behind wall case Room2
            if(myPosI <= 29) {
              //on the left side go right first
              control.command(4);
              od4.send(control);
              myPosI = myPosI+1;
            }else{
              //on the right go up
              control.command(2);
              od4.send(control);
              myPosJ = myPosJ-1;
            }
          }else{
            //Room1
            if (myPosI > 0 && myPosJ > 0 ){
              //move topleft
              control.command(1);
              od4.send(control);
              myPosI = myPosI-1;
              myPosJ = myPosJ-1;
            }else{
              if(myPosI > 0){
                // move left
                control.command(1);
                od4.send(control); 
                myPosI = myPosI-1;
              }else{
                // move right
                control.command(2);
                od4.send(control);
                myPosJ = myPosJ-1;
              }
            }
          }

        }
      }};

    auto chargingState{[&od4](cluon::data::Envelope &&envelope)
      {
        auto msg = cluon::extractMessage<tme290::grass::Sensors>(
            std::move(envelope));
        //Keep stay and checking bettery
        myBattery = msg.battery();
        tme290::grass::Control control;
        if (myBattery >= 0.98f){
          myCharging = 0;
          control.command(0);
          od4.send(control);
          myState = stateDecideNext;
          std::cout << "Charge Done" << std::endl;
        }else{
          myCharging = 1;
          control.command(0);
          od4.send(control);
          std::cout << "Charging" << std::endl;
        }
      }};

    auto goingToLastPointState{[&od4](cluon::data::Envelope &&envelope)
      {
        auto msg = cluon::extractMessage<tme290::grass::Sensors>(
            std::move(envelope));
        int myDiffI = myLastPosI - myPosI;
        int myDiffJ = myLastPosJ - myPosJ;
        tme290::grass::Control control;

        if (myDiffI == 0 && myDiffJ == 0){
          //reach last point
          myAtLastPos = 0;
          myState = stateDecideNext;
          control.command(0);
          od4.send(control);
          std::cout << "Reach Last point" << std::endl;
        }else{
          myAtLastPos = 1;
          std::cout << "Moving to Last Point..." << std::endl;
          if (myLastPosJ <= 20){
            //Target in room1
            if(myDiffI != 0){
              //Move right first
              control.command(4);
              od4.send(control);
              myPosI = myPosI+1;
            }else{
              //Then Move down
              control.command(6);
              od4.send(control);
              myPosJ = myPosJ+1;
            }
          }else{
            //Target in room2
            if(myPosJ<=21 ){
              //In Room 1
              if(myPosI<=29){
                //Move right
                control.command(4);
                od4.send(control);
                myPosI = myPosI+1;
              }else{
                //Move down
                control.command(6);
                od4.send(control);
                myPosJ = myPosJ+1;
              }
            }else{
              //In Room 2
              if(myDiffJ != 0){
                //Move down
                control.command(6);
                od4.send(control);
                myPosJ = myPosJ+1;
              }else{
                if(myDiffI > 0){
                  //Move right
                  control.command(4);
                  od4.send(control);
                  myPosI = myPosI+1;
                }else{
                  //Move left
                  control.command(1);
                  od4.send(control); 
                  myPosI = myPosI-1;
                }
              }
            }
          }
        }


        //Keepmoving to last point
        //send command to move 
        //update current pos
      }};

    auto onSensors{[&od4](cluon::data::Envelope &&envelope)
      {
        auto msg = cluon::extractMessage<tme290::grass::Sensors>(
            std::move(envelope));
      }};




    auto onStatus{[&verbose](cluon::data::Envelope &&envelope)
      {
        auto msg = cluon::extractMessage<tme290::grass::Status>(
            std::move(envelope));
        if (verbose) {
          std::cout << "Status at time " << msg.time() << ": " 
            << msg.grassMean() << "/" << msg.grassMax() << std::endl;
        }
      }};
    od4.dataTrigger(tme290::grass::Sensors::ID(), onSensors);
    od4.dataTrigger(tme290::grass::Status::ID(), onStatus);

    if (verbose) {
      std::cout << "All systems ready, let's cut some grass!" << std::endl;
    }

    switch(myState){
      case stateDecideNext :
        od4.dataTrigger(tme290::grass::Sensors::ID(), decideNext);
        break;
      case stateMoving :
        od4.dataTrigger(tme290::grass::Sensors::ID(), movingState);
        break;
      case stateStayAndCut :
        od4.dataTrigger(tme290::grass::Sensors::ID(), stayAndCutState);
        break;
      case stateGoBackHome :
        od4.dataTrigger(tme290::grass::Sensors::ID(), goingHomeState);
        break;
      case stateCharging :
        od4.dataTrigger(tme290::grass::Sensors::ID(), chargingState);
        break;
      case stateGoToLastPoint:
        od4.dataTrigger(tme290::grass::Sensors::ID(), goingToLastPointState);
        break;
      default :
        std::cout << "State Unknown" << std::endl;
    }



    while (od4.isRunning()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    retCode = 0;
  }
  return retCode;
}
