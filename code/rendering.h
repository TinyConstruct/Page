#ifndef __RENDERING__
#define __RENDERING__
class Cube {
public: 
  v3 center; 
  int width;
  Cube(v3 c, int w) {
    center = c;
    width = w;
  }
};

class Triangle {
private:
  v3 vert1, vert2, vert3;
public: 
  Triangle(){}
  Triangle(const v3 vin1, const v3 vin2, const v3 vin3) {
    vert1 = vin1;
    vert2 = vin2;
    vert3 = vin3;
  }
};

class Color {
public:
  float r,g,b;
};

class Mesh {
private:
  int nextTri;
public:
  Triangle* tris;
  Color color;
  int* elements;
  int numTris;
  Mesh(int numTrisInMesh) {
    numTris = numTrisInMesh;
    tris = new Triangle[numTrisInMesh];
    elements = new int[numTrisInMesh*3];
    nextTri = 0;
    color.r = 1.0f;
    color.g = 0.0f;
    color.b = 0.0f;
  }
  void addTri(Triangle* tri);
};

class Renderer {
  public:
  GLuint vbo, ebo;
  GLuint shaderID;
  float aspectRatio;
  std::vector<Mesh*> meshes;

  void initialize();
  void draw();
};

#endif //__RENDERING__