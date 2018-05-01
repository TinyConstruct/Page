#ifndef __GAMESTATE__
#define __GAMESTATE__
#include "aro_math.h"

static const v3 gravity = V3(0.0f, -9.86f, 0.0f);
static const float mouseLookDampen = .5f;
static const float playerWalkingSpeed = 1.3f;

class Player {
  public:
  v3 center;
  bool onGround;
  float yRotation, pitch;
  v3 viewDir;
  float joggingSpeed;
  float walkingSpeed;
  float sprintSpeed;
  v3 vel, acc;
  void initialize();
};

class Camera {
public:
  v3 center;
  float yRotation, pitch;
  v3 viewDir;
};

class KeyboardState {
public:
  bool strafeLeft, strafeRight, moveForward, moveBackward;
  bool turnLeft, turnRight;
  bool pause;
  bool jump, duck;
  bool cameraLock;
  void initialize();
};

#endif //__GAMESTATE__