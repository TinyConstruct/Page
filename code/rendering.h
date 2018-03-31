#ifndef __RENDERING__
#define __RENDERING__

#include "aro_platform_win32.h"
#include "gamestate.h"
enum {LEVEL_BUFFER};

struct Vertpcnu {
  v3 position;
  v3 color;
  v3 normal;
  v2 uv;
};

struct Texture {
  GLuint glTextureNum;
  Bitmap data;
};

struct Color {
  float r,g,b;
};

class Mesh {
public:
  Vertpcnu* firstVert;
  std::vector<Vertpcnu>* verts; //Vertp3c
  std::vector<int>* elements;
  AABB* boundingBox;
  int numTris;
  v3 meshCenter;
  #if DEBUG_BUILD
  std::vector<v3>* debugVerts;
  std::vector<int>* debugElements;
  #endif
  Mesh(int numTrisInMesh, std::vector<Vertpcnu>* vertArray, std::vector<int>* indexArray) {
    numTris = numTrisInMesh;
    verts = vertArray;
    elements = indexArray;
    //firstVert = level[level.size()];
    boundingBox = NULL;
  }

  void addTri(Vertpcnu a, Vertpcnu b, Vertpcnu c);
  void addCube(v3 center, float width, v3 color);
  void addQuad(v3 vert1, v3 vert2, v3 vert3, v3 vert4, v3 color);
  void addQuad(v3 vert1, v3 vert2, v3 vert3, v3 vert4, v3 color, LevelGeometry* level);
  void addCircle(v3 center, float rad, float numPts);
};

class Renderer {
public:
  GLuint vbo, ebo;
  GLuint shaderID;
  float aspectRatio;
  std::vector<Mesh*> meshes;
  std::vector<Vertpcnu> level; //Vertp3c
  std::vector<int> levelElements;
  #if DEBUG_BUILD
  bool renderDebug;
  GLuint debugVbo, debugEbo;
  std::vector<v3> debugBoundingVerts;
  std::vector<int> debugBoundingElements;
  #endif
  Texture metalBoxID;
  void initialize();
  void draw();
  void addDebugVolume(AABB* box);
};

GLuint loadTexture(Texture* texture, char* bmpPath);

#endif //__RENDERING__