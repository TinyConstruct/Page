#include <vector>
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

void LevelGeometry::addAABB(v3 center, v3 rad) {
  AABB box;
  box.center = center;
  box.rad = rad;
  boundingBoxes.push_back(box);
}

void LevelGeometry::initialize() {
  boundingBoxes.reserve(100);
}