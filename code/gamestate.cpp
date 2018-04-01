#include <vector>
#include <Windows.h>
#include <gl/gl.h>

#include "aro_math.h"
#include "level.h"
#include "rendering.h"

#include "gamestate.h"

void KeyboardState::initialize() {
  strafeLeft = false;
  strafeRight = false;
  moveForward = false;  
  moveBackward = false;
  turnLeft = false;  
  turnRight = false;
  pause = false;
}

void Player::initialize() {
  center.x = 0.0f;
  center.y = 1.70f;
  center.z = 3.0f;
  onGround = true; 
  yRotation = -90.0f;
  pitch = 0.0f;
  joggingSpeed = 2.6;
  walkingSpeed = 1.3;
  sprintSpeed = 7.10;
}


