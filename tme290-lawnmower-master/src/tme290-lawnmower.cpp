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
int myCommand;

float myGrassTopLeft;
float myGrassTopCentre;
float myGrassTopRight;
float myGrassRight;
float myGrassBottomRight;
float myGrassBottomCentre;
float myGrassBottomLeft;
float myGrassLeft;
float myGrassCentre;
int myState;

void foo(){
  myCommand = 0;
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

void updateDirectionNext(float grassTopLeft, float grassTopCentre, float grassTopRight, float grassRight, 
      float grassBottomRight, float grassBottomCentre, float grassBottomLeft, float grassLeft)
{
  float maxGrassNear = 0.0f;
  int maxGrassDir = 0;
  //next is the maximum grass direction
  // check 1
  if (grassTopLeft > maxGrassNear) {
      maxGrassNear = grassTopLeft;
      maxGrassDir = 1;
  }
  // check 2
  if (grassTopCentre > maxGrassNear) {
      maxGrassNear = grassTopCentre;
      maxGrassDir = 2;
  }
  // check 3
  if (grassTopRight >= maxGrassNear) {
      maxGrassNear = grassTopRight;
      maxGrassDir = 3;
  }
  // check 4
  if (grassRight > maxGrassNear) {
      maxGrassNear = grassRight;
      maxGrassDir = 4;
  }
  // check 5
  if (grassBottomRight > maxGrassNear) {
      maxGrassNear = grassBottomRight;
      maxGrassDir = 5;
  }
  // check 6
  if (grassBottomCentre > maxGrassNear) {
      maxGrassNear = grassBottomCentre;
      maxGrassDir = 6;
  }
  // check 7
  if (grassBottomLeft > maxGrassNear) {
      maxGrassNear = grassBottomLeft;
      maxGrassDir = 7;
  }
  // check 8
  if (grassLeft > maxGrassNear) {
      maxGrassNear = grassLeft;
      maxGrassDir = 8;
  }
  myDirectionNext = maxGrassDir;
}

void decideNext(float rain, float battery){
  if (battery < myBatteryToHome){
    // Low battery >> Go home
    std::cout << "Low Battery, Going Home" << std::endl;
    //Remember last point
    myLastPosI = myPosI;
    myLastPosJ = myPosJ;
    myState = stateGoBackHome;
  }else{
    if (myAtLastPos == 1){
      //Going to last pos
      myCommand = 0;
      myState = stateGoToLastPoint;
    }else{
      if(rain >= 0.2f){
        //Raining So hard >> Stay and do nothing
        std::cout << "Let's Raining. I'll wait..." << std::endl;

        myCommand = 0;
      }else{
        //Not Raining + Good Battery >> Decide to move or to cut
        if (myJustMove == 1){
          //It is time to cut!!
          myJustMove = 0;
          myCommand = 0;
          myState = stateStayAndCut;
        }else{
          //Let make some move!!
          myState = stateMoving;
        }
      }
    }
  }
}




void movingState(){
  std::cout << "Moving" << std::endl;
  switch(myDirectionNext){
    case 1:
      myCommand = 1;
      myPosI = myPosI-1;
      myPosJ = myPosJ-1;
      break;
    case 2:
      myCommand =2;
      myPosJ = myPosJ-1;
      break;
    case 3:
      myCommand = 3;
      myPosI = myPosI+1;
      myPosJ = myPosJ-1;
      break;
    case 4:
      myCommand = 4;
      myPosI = myPosI+1;
      break;
    case 5:
      myCommand = 5;
      myPosI = myPosI+1;
      myPosJ = myPosJ+1;
      break;
    case 6:
      myCommand = 6;
      myPosJ = myPosJ+1;
      break;
    case 7:
      myCommand = 7;
      myPosI = myPosI-1;
      myPosJ = myPosJ+1;
      break;
    case 8:
      myCommand = 8;
      myPosI = myPosI-1;
      break;
  }
  //Done Moving and update current position
  //Then calculate minimum battery
  int myDistanceToHome = myPosI + myPosJ;
  myBatteryToHome = (float) myDistanceToHome * 0.02f; // This 0.02 are battery drain per one step

  myJustMove = 1;
  myState = stateDecideNext;
}


void stayAndCutState(float battery, float grassCentre)
{
  if (battery < myBatteryToHome){
    //Low battery Gohome!
    std::cout << "Low Battery, Going Home From cut" << std::endl;
    //Remember last point
    myLastPosI = myPosI;
    myLastPosJ = myPosJ;
    myCommand = 0;
    myState = stateGoBackHome;
  }else{
    //Check center grass
    if (grassCentre >= myTargetCut){
      // Keep cutting
      std::cout << "Cutting..." << std::endl;
      myCommand = 0;           
    }else{
      // Done cutting
      std::cout << "Cut Done" << std::endl;
      myState = stateDecideNext;
      myCommand = 0;
    }
  }
}


void goingHomeState()
{
  if (myPosI == 0 and myPosJ == 0){
    //Reach Home Move to Charging
    myCommand = 0;
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
        myCommand = 4;
        myPosI = myPosI+1;
      }else{
        //on the right go up
        myCommand = 2;
        myPosJ = myPosJ-1;
      }
    }else{
      //Room1
      if (myPosI > 0 && myPosJ > 0 ){
        //move topleft
        myCommand = 1;
        myPosI = myPosI-1;
        myPosJ = myPosJ-1;
      }else{
        if(myPosI > 0){
          // move left
          myCommand = 8;
          myPosI = myPosI-1;
        }else{
          // move up
          myCommand = 2;
          myPosJ = myPosJ-1;
        }
      }
    }
  }
}


void chargingState(float battery){
    //Keep stay and checking bettery
    if (battery >= 0.98f){
      myCharging = 0;
      myCommand = 0;
      myState = stateDecideNext;
      std::cout << "Charge Done" << std::endl;
    }else{
      myCharging = 1;
      myCommand = 0;
      std::cout << "Charging" << std::endl;
    }
}

auto goingToLastPointState(){
    int myDiffI = myLastPosI - myPosI;
    int myDiffJ = myLastPosJ - myPosJ;
    if (myDiffI == 0 && myDiffJ == 0){
      //reach last point
      myAtLastPos = 0;
      myCommand = 0;
      od4.send(control);
      std::cout << "Reach Last point" << std::endl;
    }else{
      myAtLastPos = 1;
      std::cout << "Moving to Last Point..." << std::endl;
      if (myLastPosJ <= 20){
        //Target in room1
        if(myDiffI != 0){
          //Move right first
          myCommand = 4;
          myPosI = myPosI+1;
        }else{
          //Then Move down
          myCommand = 6;
          myPosJ = myPosJ+1;
        }
      }else{
        //Target in room2
        if(myPosJ<=21 ){
          //In Room 1
          if(myPosI<=29){
            //Move right
            myCommand = 4;
            myPosI = myPosI+1;
          }else{
            //Move down
            myCommand = 6;
            myPosJ = myPosJ+1;
          }
        }else{
          //In Room 2
          if(myDiffJ != 0){
            //Move down
            myCommand = 6;
            myPosJ = myPosJ+1;
          }else{
            if(myDiffI > 0){
              //Move right
              myCommand = 4;
              myPosI = myPosI+1;
            }else{
              //Move left
              myCommand = 8;
              myPosI = myPosI-1;
            }
          }
        }
      }
    } 
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


    auto onSensors{[&od4](cluon::data::Envelope &&envelope)
      {
        auto msg = cluon::extractMessage<tme290::grass::Sensors>(
            std::move(envelope));
        myGrassTopLeft = msg.grassTopLeft();
        myGrassTopCentre = msg.grassTopCentre();
        myGrassTopRight = msg.grassTopRight();
        myGrassRight = msg.grassRight();
        myGrassBottomRight = msg.grassBottomRight();
        myGrassBottomCentre = msg.grassBottomCentre();
        myGrassBottomLeft = msg.grassBottomLeft();
        myGrassLeft = msg.grassLeft();
        myGrassCentre = msg.grassCentre();
        myRain = msg.rain();
        myBattery = msg.battery();

        switch(myState){
          case stateDecideNext:
            std::cout << "State: decideNext" << std::endl;  
            decideNext(myRain, myBattery);
            break;
          case stateMoving:
            std::cout << "State: Moving" << std::endl;
            updateDirectionNext(myGrassTopLeft,myGrassTopCentre,myGrassTopRight,myGrassRight,myGrassBottomRight, myGrassBottomCentre, myGrassBottomLeft,myGrassLeft);
            movingState();
            break;
          case stateStayAndCut:
            std::cout << "State: Cutting" << std::endl;
            stayAndCutState(myBattery, myGrassCentre);
            break;
          case stateGoBackHome:
            std::cout << "State: GoBackHome" << std::endl;
            goingHomeState();
            break;
          case stateCharging:
            std::cout << "State: Charging" << std::endl;
            chargingState(myBattery);
            break;
          case stateGoToLastPoint:
            std::cout << "State: stateGoToLastPoint" << std::endl;
            goingToLastPointState();
            break;
          default :
            std::cout << "State Unknown" << std::endl;
        }
        tme290::grass::Control control;
        control.command(myCommand);
        od4.send(control);
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
    std::cout << "Mystate: " << myState << std::endl;

    tme290::grass::Control control;
    control.command(0);
    od4.send(control);
    /*

      case stateGoToLastPoint:
        od4.dataTrigger(tme290::grass::Sensors::ID(), goingToLastPointState);
        break;
      default :
        std::cout << "State Unknown" << std::endl;
    }*/
    


    while (od4.isRunning()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    retCode = 0;
  }
  return retCode;
}
