#ifndef __GAMESTATE__
#define __GAMESTATE__

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

#endif //__GAMESTATE__