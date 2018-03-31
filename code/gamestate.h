#ifndef __GAMESTATE__
#define __GAMESTATE__

#include "aro_math.h"

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


class KeyboardState {
public:
  bool strafeLeft, strafeRight, moveForward, moveBackward;
  bool turnLeft, turnRight;
  bool pause;
  bool jump, duck;
  void initialize();
};

struct AABB {
  v3 center;
  v3 rad;
};

class LevelGeometry {
public:
  std::vector<AABB> boundingBoxes;
  void addAABB(v3 center, v3 rad);
  void initialize();
};

#endif //__GAMESTATE__