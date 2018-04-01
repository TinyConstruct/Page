#ifndef __LEVEL__
#define __LEVEL__

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

struct Tri {
  v3 a, b, c;
};

struct Quad {
  v3 a, b, c, d;
};

// TODO: our levels will be loaded into memory, sorted by geometry/textures, then sent to the 
// renderer/collision detection structures
class Level {
private:
  //when building a level, we will sort quads and tris based on texture binding,
  //then combine them into one buffer such that [quads...tris] for rendering
  std::vector<Quad>* quads;
  std::vector<Tri>* tris;
  LevelGeometry geo;
public: 
  void initialize();
};

#endif