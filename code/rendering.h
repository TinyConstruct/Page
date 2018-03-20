#ifndef __RENDERING__
#define __RENDERING__

enum {LEVEL_BUFFER};

struct Vertpcn {
  v3 position;
  v3 color;
  v3 normal;
};

struct Color {
  float r,g,b;
};

class Mesh {
public:
  Vertpcn* firstVert;
  std::vector<Vertpcn>* verts; //Vertp3c
  std::vector<int>* elements;
  int numTris;
  v3 meshCenter;
  Mesh(int numTrisInMesh, std::vector<Vertpcn>* vertArray, std::vector<int>* indexArray) {
    numTris = numTrisInMesh;
    verts = vertArray;
    elements = indexArray;
    //firstVert = level[level.size()];
  }
  void addTri(Vertpcn a, Vertpcn b, Vertpcn c);
  void addCube(v3 center, float width, v3 color);
};

class Renderer {
public:
  GLuint vbo, ebo;
  GLuint shaderID;
  float aspectRatio;
  std::vector<Mesh*> meshes;
  std::vector<Vertpcn> level; //Vertp3c
  std::vector<int> levelElements;
  void initialize();
  void draw();
};

#endif //__RENDERING__