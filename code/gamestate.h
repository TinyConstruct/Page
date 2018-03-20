#ifndef __GAMESTATE__
#define __GAMESTATE__

#include "aro_math.h"

class Player {
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
  void initialize();
};

#endif //__GAMESTATE__