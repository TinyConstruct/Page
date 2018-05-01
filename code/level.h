#ifndef __LEVEL__
#define __LEVEL__
#include "aro_math.h"
#include "gamestate.h"

struct AABB {
  v3 center;
  v3 rad;
};

struct OBB {
  v3 c;
  v3 u[3]; //local axes
  v3 width; //halfwidth extants along OBB axes
};

int isColliding(OBB& a, OBB& b);
int isColliding(AABB& a, AABB& b);

class LevelGeometry {
public:
  std::vector<AABB> AABBs;
  std::vector<OBB> OBBs;
  
  void addAABB(v3& center, v3& rad);
  void addOOB(v3& center, v3 axes[3], v3& halfW);
  void initialize();
  void movePlayer(Player& player, KeyboardState& keyboardState, float pitchDif, float yRotationDif, float lastFrameSec);
  void moveFreeCam(Camera& freeCamera, KeyboardState& keyboardState, float pitchDif, float yRotationDif, float lastFrameSec);
};

struct Tri {
  v3 a, b, c;
};

struct TexturedQuad {
  v3 a, b, c, d;
  TextureHandle texHandle;
  v2 texScale;
};

// TODO: our levels will be loaded into memory, sorted by geometry/textures, then sent to the 
// renderer/collision detection structures
class LevelData {
private:
  //when building a level, we will sort quads and tris based on texture binding,
  //then combine them into one buffer such that [quads...tris] for rendering
  std::vector<TexturedQuad>* quads;
  std::vector<Tri>* tris;
  LevelGeometry* geo;
  Renderer* renderer;
public: 
  void initialize(LevelGeometry* g, Renderer* r);
  void addTexturedQuad(v3& vert1, v3& vert2, v3& vert3, v3& vert4, int texID);
  void addTexturedQuad(v3& vert1, v3& vert2, v3& vert3, v3& vert4, int texID, LevelGeometry* level);
  void addTexturedQuad(TexturedQuad& q);
  void bakeTestLevel();
  void finalizeQuads();
  void loadLevelFromTextFile(char* path);
  void preLoadTextureArray512(int numTexes, char* tex1, char* tex2, char* tex3, char* tex4);
  TexturedQuad processQuadTextLine(char* str);
  char* processQuadTextLine(char* str, TexturedQuad* q);
  char* processQuadTripple(char* str, v3* v);
  char* processUV2(char* str, v2* v);
};

#endif
