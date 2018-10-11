#ifndef __RENDERING__
#define __RENDERING__

#include "bitmap.h"
#include "aro_math.h"
#include "aro_platform_win32.h"
enum {LEVEL_BUFFER};
enum {FLOOR1=0,FLOOR2, FLOOR3, WALL1, WALL2, CEIL1, CITY2_2, CITY3_2, CITY4_1, METALBOX, MAX_TEX};
enum {N_FLOOR1=0,N_FLOOR2, N_FLOOR3, N_WALL1, N_WALL2, N_CEIL1, N_CITY2_2, N_CITY3_2, N_CITY4_1, N_METALBOX, N_MAX_TEX};
const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
const float SHADOW_ASPECT = (float)SHADOW_WIDTH/(float)SHADOW_HEIGHT;

struct GLDebugLog {
  GLenum sources[100];
  GLenum types[100];
  GLuint ids[100];
  GLenum severities[100];
  GLsizei lengths[100];
  char buffer[50000];
};

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

struct PointLightArray {
  int numLights;
  std::vector<GLuint> lightTexIds; 
  std::vector<GLuint> lightFBOIds; 
  std::vector<m4x4> perspViews;
  std::vector<v3> positions; 
  std::vector<Color> colors;
};

class Renderer {
public:
  bool initDone;
  Win32WindowDimensions windowDimensions;
  bool clipMouse;
  m4x4 pointShadowProjMat;
  float pointShadowProjMatFarPlane;
  GLuint depthMapFBO;
  GLuint depthMap;
  TextureHandle texTable[MAX_TEX];
  TextureHandle texNormTable[N_MAX_TEX];
  GLuint textureLocation, normTextureLocation, depthMapTextureLocation, depthCubeMapLocation, multisampleTextureLocation;
  GLuint vao, vbo, ebo, uniformBuffer, multisampleRBO, intermediateFBO;
  GLuint mainFrameBuffer, quadFramebuffer, screenQuadTexture;
  GLuint quadVAO, quadVBO;
  GLsizei numMSAASamples;
  GLuint shaderID, shadowShaderID, screenShaderID;
  v3 globalLight;
  PointLightArray pointLights;
  float aspectRatio;
  std::vector<Vertpcnu> level; 
  std::vector<int> levelElements;
  #if DEBUG_BUILD
    bool renderDebug;
    GLuint debugVbo, debugEbo, debugShaderID;
    GLuint debugPlayerVbo, debugPlayerEbo;
    std::vector<v3> debugBoundingVerts;
    std::vector<int> debugBoundingElements;
    std::vector<v3> debugPlayerVerts = std::vector<v3>(8);
    std::vector<int> debugPlayerElements;
  #endif
    
  void initialize();
  void draw();
  void addDebugVolume(const v3& center, const v3& rad);
  void addPlayerDebugVolume(v3& center, v3 axes[3], v3& halfW);
  void loadTextureIntoArray(char* bmpPath, int width, int height);
  GLuint createTextureArrayBuffer(int width, int height);
  void loadTextureArray512(TextureHandle texTable[], int a, char* aPath, int b, char* bPath, int c, char* cPath, int d, char* dPath);
  TextureHandle getGLTexID(int texID);
  TextureHandle getGLNormTexID(int texID);
  void addTri(Vertpcnu& a, Vertpcnu& b, Vertpcnu& c);
  void addDebugVolume(v3& center, v3 axes[3], v3& halfW);
  void debugDrawLine(const v3& start, const v3& end);
  void addPointLight(PointLightArray* array, v3 position);
  void renderPointLights(PointLightArray* plr);
  void reconfigureMSAA();
};


GLuint loadTexture(Texture* texture, char* bmpPath);
int texStrToID(char* str);
int texNormStrToID(char* str);

#endif //__RENDERING__