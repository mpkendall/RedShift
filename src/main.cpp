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
controller Controller =   controller(controllerType::primary);
motor motor1 =            motor(PORT20, ratio18_1, true);
motor motor2 =            motor(PORT10, ratio18_1, true);
motor motor3 =            motor(PORT11, ratio18_1, false);
motor motor4 =            motor(PORT1, ratio18_1, false);
motor intake_front =      motor(PORT6, ratio36_1, true);
motor intake_middle =     motor(PORT3, ratio36_1, false);
motor intake_back =       motor(PORT19, ratio36_1, false);

int quadrant = 0; // Global variable for quadrant selection (0-3)


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

struct joystickValues
{
  int axis1;
  int axis2;
  int axis3;
  int axis4;
  bool left1;
  bool left2;
  bool right1;
  bool right2;
};
joystickValues currentJoystickValues;

struct drivetrainPosition
{
  int m1D;
  int m2D;
  int m3D;
  int m4D;
  motorDirection intake1;
  motorDirection intake2;
  motorDirection intake3;
};

double clamp(double value, double minVal, double maxVal) {
  if (value > maxVal) return maxVal;
  if (value < minVal) return minVal;
  return value;
}


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
  vex::brakeType brakeBehavior = vex::brakeType::brake;
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
    motor1.stop(brakeBehavior);
    motor2.stop(brakeBehavior);
    break;
  case motorDirection::rightSlow:
    motor1.spin(forward, speedPercent, percent);
    motor2.spin(forward, speedPercent, percent);
    motor3.stop(brakeBehavior);
    motor4.stop(brakeBehavior);
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
    motor1.stop(brakeBehavior);
    motor2.stop(brakeBehavior);
    motor3.stop(brakeBehavior);
    motor4.stop(brakeBehavior);
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
  Brain.Screen.clearScreen();
  Brain.Screen.setPenColor(color::white);
  Brain.Screen.printAt(1, 20, "quadrant");

  Brain.Screen.drawRectangle(130, 30, 100, 100, color::red);
  Brain.Screen.printAt(175, 85, "1");

  Brain.Screen.drawRectangle(250, 30, 100, 100, color::blue);
  Brain.Screen.printAt(295, 85, "2");

  Brain.Screen.drawRectangle(130, 140, 100, 100, color::green);
  Brain.Screen.printAt(175, 195, "3");

  Brain.Screen.drawRectangle(250, 140, 100, 100, color::yellow);
  Brain.Screen.printAt(295, 195, "4");

  bool selected = false;
  while (!selected)
  {
    if (Brain.Screen.pressing())
    {
      int x = Brain.Screen.xPosition();
      int y = Brain.Screen.yPosition();

      if (x >= 130 && x <= 230 && y >= 30 && y <= 130)
      {
        quadrant = 0;
        selected = true;
      }
      else if (x >= 250 && x <= 350 && y >= 30 && y <= 130)
      {
        quadrant = 1;
        selected = true;
      }
      else if (x >= 130 && x <= 230 && y >= 140 && y <= 240)
      {
        quadrant = 2;
        selected = true;
      }
      else if (x >= 250 && x <= 350 && y >= 140 && y <= 240)
      {
        quadrant = 3;
        selected = true;
      }
    }
    wait(20, msec);
  }
  Brain.Screen.setCursor(1, 1);
  Brain.Screen.print("quadrant: %d", quadrant + 1);
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

void bringToDegrees(drivetrainPosition targetPosition)
{
  // Start all motors moving non-blocking


  motor1.spinTo(targetPosition.m1D, degrees, 20, velocityUnits::pct, false);
  motor2.spinTo(targetPosition.m2D, degrees, 20, velocityUnits::pct, false);
  motor3.spinTo(targetPosition.m3D, degrees, 20, velocityUnits::pct, false);
  motor4.spinTo(targetPosition.m4D, degrees, 20, velocityUnits::pct, false);
  while (motor1.isSpinning() || motor2.isSpinning() || motor3.isSpinning() || motor4.isSpinning()) {
    wait(20, msec);
  }

    if(targetPosition.intake1 == motorDirection::forward)
    intake_front.spin(forward, 100, percent);
  else if(targetPosition.intake1 == motorDirection::backward)
    intake_front.spin(reverse, 100, percent);
  else
    intake_front.stop();

  if(targetPosition.intake2 == motorDirection::forward)
    intake_middle.spin(reverse, 100, percent);
  else if(targetPosition.intake2 == motorDirection::backward)
    intake_middle.spin(forward, 100, percent);
  else
    intake_middle.stop();

  if(targetPosition.intake3 == motorDirection::forward)
    intake_back.spin(forward, 100, percent);
  else if(targetPosition.intake3 == motorDirection::backward)
    intake_back.spin(reverse, 100, percent);
  else
    intake_back.stop();
}

drivetrainPosition points[] = {
    {0, 0, 0, 0, motorDirection::forward, motorDirection::forward, motorDirection::stop},
    {360, 360, 360, 360, motorDirection::forward, motorDirection::forward, motorDirection::stop},
    {358, 359, 496, 496, motorDirection::stop, motorDirection::stop, motorDirection::stop},
    {826, 826, 514, 518, motorDirection::stop, motorDirection::stop, motorDirection::stop},
    {994, 992, 649, 648, motorDirection::backward, motorDirection::backward, motorDirection::backward},
};

void autonomous(void)
{
  // ..........................................................................
  // Insert autonomous user code here.
  // ..........................................................................
  Brain.Screen.clearScreen();
  Brain.Screen.setCursor(1, 1);
  Brain.Screen.print("Autonomous Mode");

  motor1.setPosition(0, degrees);
  motor2.setPosition(0, degrees);
  motor3.setPosition(0, degrees);
  motor4.setPosition(0, degrees);

  // Loop through points. Note: strictly less than (<) size, not <=
  int numPoints = sizeof(points)/sizeof(points[0]);
  for(int i = 0; i < numPoints; i++)
  {
    Brain.Screen.setCursor(2, 1);
    Brain.Screen.print("Driving to Point %d", i);
    bringToDegrees(points[i]);
    wait(300, msec);
  }
  Brain.Screen.setCursor(3, 1);
  Brain.Screen.print("Autonomous Complete");
  motor1.stop(coast);
  motor2.stop(coast);
  motor3.stop(coast);
  motor4.stop(coast);

  while(true) {
    Brain.Screen.setCursor(4, 1);
    Brain.Screen.print("motor 1 pos: %f", motor1.position(degrees));
    Brain.Screen.setCursor(5, 1);
    Brain.Screen.print("motor 2 pos: %f", motor2.position(degrees));
    Brain.Screen.setCursor(6, 1);
    Brain.Screen.print("motor 3 pos: %f", motor3.position(degrees));
    Brain.Screen.setCursor(7, 1);
    Brain.Screen.print("motor 4 pos: %f", motor4.position(degrees));
    wait(100, msec);
  }
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
  motor1.setPosition(0, degrees);
  motor2.setPosition(0, degrees);
  motor3.setPosition(0, degrees);
  motor4.setPosition(0, degrees);
  // User control code here, inside the loop
  while (1)
  {
    currentJoystickValues.axis1 = Controller.Axis1.position();
    currentJoystickValues.axis2 = Controller.Axis2.position();
    currentJoystickValues.axis3 = Controller.Axis3.position();
    currentJoystickValues.axis4 = Controller.Axis4.position();

    currentJoystickValues.left1 = Controller.ButtonL1.pressing();
    currentJoystickValues.left2 = Controller.ButtonL2.pressing();
    currentJoystickValues.right1 = Controller.ButtonR1.pressing();
    currentJoystickValues.right2 = Controller.ButtonR2.pressing();

    if (currentJoystickValues.left1)
    {
      intake_front.spin(forward, 100, percent);
      intake_middle.spin(reverse, 100, percent);
    }
    else if (currentJoystickValues.left2)
    {
      intake_front.spin(reverse, 50, percent);
      intake_middle.spin(forward, 50, percent);
    }
    else
    {
      intake_front.stop();
      intake_middle.stop();
    }

    if (currentJoystickValues.right1)
    {
      intake_middle.spin(forward, 100, percent);
      intake_back.spin(forward, 100, percent);
    }
    else if (currentJoystickValues.right2)
    {
      intake_middle.spin(reverse, 100, percent);
      intake_back.spin(reverse, 100, percent);
    }
    else
    {
      //intake_middle.stop();
      intake_back.stop();
    }
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
    Brain.Screen.print("M1: %f  ", motor1.position(degrees));
    Brain.Screen.setCursor(6, 30);
    Brain.Screen.print("M2: %f  ", motor2.position(degrees));
    Brain.Screen.setCursor(7, 30);
    Brain.Screen.print("M3: %f   ", motor3.position(degrees));
    Brain.Screen.setCursor(8, 30);
    Brain.Screen.print("M4: %f  ", motor4.position(degrees));

    Brain.Screen.setCursor(9, 30);
    Brain.Screen.print("Intake: %f  ", intake_middle.velocity(percent));

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
