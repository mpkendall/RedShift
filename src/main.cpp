/*----------------------------------------------------------------------------*/
/*                                                                            */
/*    Module:       main.cpp                                                  */
/*    Author:       maxsr                                                     */
/*    Created:      11/7/2025, 3:00:17 PM                                     */
/*    Description:  V5 project                                                */
/*                                                                            */
/*----------------------------------------------------------------------------*/

#include "vex.h"

using namespace vex;

// A global instance of competition
competition Competition;
brain Brain;
controller Controller = controller(controllerType::primary);
optical OpticalSensor = optical(PORT3);
motor motor1 = motor(PORT20, ratio18_1, true);
motor motor2 = motor(PORT10, ratio18_1, true);
motor motor3 = motor(PORT11, ratio18_1, false);
motor motor4 = motor(PORT1, ratio18_1, false);
motor intake = motor(PORT6, ratio18_1, false);

/*

* drive train motor mapping:
* - motor 1    * - motor 3
* - motor 2    * - motor 4

*/

enum class motorDirection
{
  forward,
  backward,
  leftSlow,
  rightSlow,
  leftFast,
  rightFast,
  stop
};

struct joystickValues {
  int axis1;
  int axis2;
  int axis3;
  int axis4;
};
joystickValues currentJoystickValues;



void drawJoystickBar(int x, int y, int value, const char *label)
{
  // Draw a horizontal bar graph for joystick values
  // value ranges from -100 to 100
  // Bar width is about 90 pixels (45 in each direction from center)

  const int barHeight = 8;
  const int barWidth = 90;
  const int centerX = x + barWidth / 2;

  // Draw background bar outline
  Brain.Screen.drawRectangle(x, y, barWidth, barHeight, color::white);

  // Draw center line (neutral position)
  Brain.Screen.drawLine(centerX, y, centerX, y + barHeight);

  // Calculate filled portion based on value
  int filledWidth = (value * barWidth) / (2 * 100); // -100 to 100
  int fillX = centerX;
  int fillW = 0;
  color fillColor = color::green;

  if (filledWidth > 0)
  {
    fillX = centerX;
    fillW = filledWidth;
  }
  else if (filledWidth < 0)
  {
    fillX = centerX + filledWidth;
    fillW = -filledWidth;
    fillColor = color::blue;
  }

  // Draw filled portion
  if (fillW > 0)
  {
    Brain.Screen.drawRectangle(fillX, y, fillW, barHeight, fillColor);
  }
}

void setMotorDirection(motorDirection targetDirection, int speedPercent)
{
  switch (targetDirection)
  {
  case motorDirection::forward:
    motor1.spin(forward, speedPercent, percent);
    motor2.spin(forward, speedPercent, percent);
    motor3.spin(forward, speedPercent, percent);
    motor4.spin(forward, speedPercent, percent);
    break;
  case motorDirection::backward:
    motor1.spin(reverse, speedPercent, percent);
    motor2.spin(reverse, speedPercent, percent);
    motor3.spin(reverse, speedPercent, percent);
    motor4.spin(reverse, speedPercent, percent);
    break;
  case motorDirection::leftSlow:
    motor3.spin(forward, speedPercent, percent);
    motor4.spin(forward, speedPercent, percent);
    motor1.stop();
    motor2.stop();
    break;
  case motorDirection::rightSlow:
    motor1.spin(forward, speedPercent, percent);
    motor2.spin(forward, speedPercent, percent);
    motor3.stop();
    motor4.stop();
    break;
  case motorDirection::leftFast:
    motor1.spin(reverse, speedPercent, percent);
    motor2.spin(reverse, speedPercent, percent);
    motor3.spin(forward, speedPercent, percent);
    motor4.spin(forward, speedPercent, percent);
    break;
  case motorDirection::rightFast:
    motor1.spin(forward, speedPercent, percent);
    motor2.spin(forward, speedPercent, percent);
    motor3.spin(reverse, speedPercent, percent);
    motor4.spin(reverse, speedPercent, percent);
    break;
  case motorDirection::stop:
    motor1.stop();
    motor2.stop();
    motor3.stop();
    motor4.stop();
    break;
  }
}

void updateMotorsFromJoystick(joystickValues values)
{
  int verticalInput = values.axis3;
  int horizontalInput = values.axis1;
  
  // deadzone
  const int DEADZONE = 5;
  if (abs(verticalInput) < DEADZONE)
    verticalInput = 0;
  if (abs(horizontalInput) < DEADZONE)
    horizontalInput = 0;
  
  if (verticalInput == 0 && horizontalInput == 0)
  {
    setMotorDirection(motorDirection::stop, 0);
    return;
  }
  
  // tank drive logic
  // left motors (1, 2) and right motors (3, 4)
  if (verticalInput > 0 && horizontalInput == 0)
  {
    // Moving forward
    setMotorDirection(motorDirection::forward, verticalInput);
  }
  else if (verticalInput < 0 && horizontalInput == 0)
  {
    // Moving backward
    setMotorDirection(motorDirection::backward, abs(verticalInput));
  }
  else if (verticalInput == 0 && horizontalInput > 0)
  {
    // Turning right
    setMotorDirection(motorDirection::rightFast, abs(horizontalInput));
  }
  else if (verticalInput == 0 && horizontalInput < 0)
  {
    // Turning left
    setMotorDirection(motorDirection::leftFast, abs(horizontalInput));
  }
  else if (verticalInput > 0 && horizontalInput > 0)
  {
    // Forward + right turn (slow right)
    setMotorDirection(motorDirection::rightSlow, verticalInput);
  }
  else if (verticalInput > 0 && horizontalInput < 0)
  {
    // Forward + left turn (slow left)
    setMotorDirection(motorDirection::leftSlow, verticalInput);
  }
  else if (verticalInput < 0 && horizontalInput > 0)
  {
    // Backward + right turn (slow right)
    setMotorDirection(motorDirection::rightSlow, abs(verticalInput));
  }
  else if (verticalInput < 0 && horizontalInput < 0)
  {
    // Backward + left turn (slow left)
    setMotorDirection(motorDirection::leftSlow, abs(verticalInput));
  }

}

/*---------------------------------------------------------------------------*/
/*                          Pre-Autonomous Functions                         */
/*                                                                           */
/*  You may want to perform some actions before the competition starts.      */
/*  Do them in the following function.  You must return from this function   */
/*  or the autonomous and usercontrol tasks will not be started.  This       */
/*  function is only called once after the V5 has been powered on and        */
/*  not every time that the robot is disabled.                               */
/*---------------------------------------------------------------------------*/

void pre_auton(void)
{
  setMotorDirection(motorDirection::stop, 0);
  // take button input
}

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*                              Autonomous Task                              */
/*                                                                           */
/*  This task is used to control your robot during the autonomous phase of   */
/*  a VEX Competition.                                                       */
/*                                                                           */
/*  You must modify the code to add your own robot specific commands here.   */
/*---------------------------------------------------------------------------*/

void autonomous(void)
{
  // ..........................................................................
  // Insert autonomous user code here.
  // ..........................................................................
  Brain.Screen.clearScreen();
  Brain.Screen.setCursor(1, 1);
  Brain.Screen.print("Autonomous Mode");
}

/*---------------------------------------------------------------------------*/
/*                                                                           */
/*                              User Control Task                            */
/*                                                                           */
/*  This task is used to control your robot during the user control phase of */
/*  a VEX Competition.                                                       */
/*                                                                           */
/*  You must modify the code to add your own robot specific commands here.   */
/*---------------------------------------------------------------------------*/

void usercontrol(void)
{
  // User control code here, inside the loop
  while (1)
  {
    currentJoystickValues.axis1 = Controller.Axis1.position();
    currentJoystickValues.axis2 = Controller.Axis2.position();
    currentJoystickValues.axis3 = Controller.Axis3.position();
    currentJoystickValues.axis4 = Controller.Axis4.position();

    // Update motor commands based on joystick input
    updateMotorsFromJoystick(currentJoystickValues);

    Brain.Screen.clearScreen();

    // Title
    Brain.Screen.setCursor(1, 1);
  
    Brain.Screen.print("User Control");
    Brain.Screen.setCursor(2, 1);
    Brain.Screen.print("== JOYSTICK VALUES ==");
    Brain.Screen.setCursor(2, 30);
    Brain.Screen.print("MOTOR DIAGNOSTICS");

    // Draw graphical bars for each axis
    // Y position for each bar (spacing them out)
    drawJoystickBar(10, 40, currentJoystickValues.axis1, "Axis1");
    drawJoystickBar(10, 60, currentJoystickValues.axis2, "Axis2");
    drawJoystickBar(10, 80, currentJoystickValues.axis3, "Axis3");
    drawJoystickBar(10, 100, currentJoystickValues.axis4, "Axis4");

    // Motor commands display (right side)
    Brain.Screen.setCursor(5, 30);
    Brain.Screen.print("M1: %f  ", motor1.velocity(percent));
    Brain.Screen.setCursor(6, 30);
    Brain.Screen.print("M2: %f  ", motor2.velocity(percent));
    Brain.Screen.setCursor(7, 30);
    Brain.Screen.print("M3: %f   ", motor3.velocity(percent));
    Brain.Screen.setCursor(8, 30);
    Brain.Screen.print("M4: %f  ", motor4.velocity(percent));

    Brain.Screen.setCursor(9, 30);
    Brain.Screen.print("Intake: %f  ", intake.velocity(percent));
    intake.spin(forward, Controller.Axis3.position(), percent);

    wait(20, msec); // Sleep the task for a short amount of time to
                    // prevent wasted resources.
  }
}

int main()
{
  // Set up callbacks for autonomous and driver control periods.
  Competition.autonomous(autonomous);
  Competition.drivercontrol(usercontrol);

  // Run the pre-autonomous function.
  pre_auton();

  // Prevent main from exiting with an infinite loop.
  while (true)
  {
    wait(50, msec);
  }
}
