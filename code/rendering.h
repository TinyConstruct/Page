#ifndef __RENDERING__
#define __RENDERING__

#include "bitmap.h"
#include "aro_math.h"
enum {LEVEL_BUFFER};
enum {FLOOR1=0,FLOOR2, FLOOR3, WALL1, WALL2, CEIL1, MAX_TEX};

struct Vertpcnu {
  v3 position;
  v3 color;
  v3 normal;
  v3 uv;
};

struct Texture {
  GLuint glTextureNum;
  Bitmap data;
};

struct Color {
  float r,g,b;
};

/*
class Mesh {
public:
  Vertpcnu* firstVert;
  std::vector<Vertpcnu>* verts; //Vertp3c
  std::vector<int>* elements;
  //AABB* boundingBox;
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
*/

class Renderer {
public:
  TextureHandle texTable[MAX_TEX];
  GLuint vbo, ebo, uniformBuffer;
  GLuint shaderID;
  float aspectRatio;
  //std::vector<Mesh*> meshes;
  std::vector<Vertpcnu> level; 
  std::vector<int> levelElements;
  #if DEBUG_BUILD
  bool renderDebug;
  GLuint debugVbo, debugEbo, debugShaderID;
  std::vector<v3> debugBoundingVerts;
  std::vector<int> debugBoundingElements;
  #endif
    
  void initialize();
  void draw();
  void addDebugVolume(const v3& center, const v3& rad);
  void loadTextureIntoArray(char* bmpPath, int width, int height);
  GLuint createTextureArrayBuffer(int width, int height);
  void loadTextureArray512(int a, char* aPath, int b, char* bPath, int c, char* cPath, int d, char* dPath);
  void textureArrayTest();
  TextureHandle getGLTexID(int texID);
  void addTri(Vertpcnu& a, Vertpcnu& b, Vertpcnu& c);
  void addDebugVolume(v3& center, v3 axes[3], v3& halfW);
  void debugDrawLine(v3& start, v3& end);
};

GLuint loadTexture(Texture* texture, char* bmpPath);

#endif //__RENDERING__