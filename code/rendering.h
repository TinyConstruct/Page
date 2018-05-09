#ifndef __RENDERING__
#define __RENDERING__

#include "bitmap.h"
#include "aro_math.h"
enum {LEVEL_BUFFER};
enum {FLOOR1=0,FLOOR2, FLOOR3, WALL1, WALL2, CEIL1, CITY2_2, CITY3_2, CITY4_1, METALBOX, MAX_TEX};
enum {N_FLOOR1=0,N_FLOOR2, N_FLOOR3, N_WALL1, N_WALL2, N_CEIL1, N_CITY2_2, N_CITY3_2, N_CITY4_1, N_METALBOX, N_MAX_TEX};

struct Vertpcnu {
  v3 position;
  v3 color;
  v3 normal;
  v3 uv;
  v3 tangent;
  v3 bitangent;
};

struct Texture {
  GLuint glTextureNum;
  Bitmap data;
};

struct Color {
  float r,g,b;
};

class Renderer {
public:
  TextureHandle texTable[MAX_TEX];
  TextureHandle texNormTable[N_MAX_TEX];
  GLuint textureLocation, normTextureLocation;
  GLuint vbo, ebo, uniformBuffer;
  GLuint shaderID;
  float aspectRatio;
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
  void loadTextureArray512(TextureHandle texTable[], int a, char* aPath, int b, char* bPath, int c, char* cPath, int d, char* dPath);
  TextureHandle getGLTexID(int texID);
  TextureHandle getGLNormTexID(int texID);
  void addTri(Vertpcnu& a, Vertpcnu& b, Vertpcnu& c);
  void addDebugVolume(v3& center, v3 axes[3], v3& halfW);
  void debugDrawLine(v3& start, v3& end);
};

GLuint loadTexture(Texture* texture, char* bmpPath);
int texStrToID(char* str);
int texNormStrToID(char* str);

#endif //__RENDERING__