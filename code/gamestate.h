#ifndef __GAMESTATE__
#define __GAMESTATE__


class Player {
  public:
  v3 center, lookDir;
};

class KeyboardState {
public:
  bool strafeLeft, strafeRight, moveForward, moveBackward;
  bool turnLeft, turnRight;
};

#endif //__GAMESTATE__