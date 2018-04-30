#include <vector>
#include <Windows.h>
#include <gl/gl.h>

#include "aro_math.h"
#include "texture.h"
#include "rendering.h"
#include "level.h"

#include "gamestate.h"

void KeyboardState::initialize() {
  strafeLeft = false;
  strafeRight = false;
  moveForward = false;  
  moveBackward = false;
  turnLeft = false;  
  turnRight = false;
  pause = false;
  cameraLock = true;
}

void Player::initialize() {
  center.x = 0.0f;
  center.y = 0.5f;
  center.z = -0.5f;
  onGround = true; 
  yRotation = 0.0f;
  pitch = 0.0f;
  joggingSpeed = 2.6f;
  walkingSpeed = 1.3f;
  sprintSpeed = 7.10f;
}

void movePlayer(Player& player, KeyboardState& keyboardState) {

}
