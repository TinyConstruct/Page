#include <Windows.h>
#include <gl/GL.h>
#include <math.h>
#include <stdint.h>
#include <vector>

#include "aro_generic.h"
#include "aro_opengl.h"
#include "aro_math.h"
#include "rendering.h"

const char* vertexSource = R"FOO(
#version 330 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
layout (location = 2) in vec3 normal;
out vec3 colorOutFromVert;
out vec3 normalOutFromVert;
out vec3 fragPos;
uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

void main()
{
  gl_Position =  projection * view * vec4(position.x, position.y, position.z, 1.0f);
  fragPos = (model * vec4(position.x, position.y, position.z, 1.0f)).xyz;
  colorOutFromVert = color;
  normalOutFromVert = normal;
}
)FOO";

const char* fragmentSource = R"FOO(
#version 330 core
in vec3 colorOutFromVert;
in vec3 normalOutFromVert;
in vec3 fragPos;
out vec4 resultColor;
void main()
{
  vec3 lightColor = vec3(1.0,1.0,1.0);
  float ambientIntensity= .1;
  vec3 norm = normalize(normalOutFromVert);
  vec3 lightPos = vec3(-4,3,5.0);
  vec3 lightDir = normalize(lightPos - fragPos);
  float diff = max(dot(norm, lightDir), 0.0);
  vec3 diffuse = diff * lightColor;
  vec3 ambient = lightColor*ambientIntensity;
  resultColor = vec4(((ambient + diffuse) * colorOutFromVert), 1.0);
}
)FOO";

void Renderer::initialize() {
  meshes.reserve(100);
  level.reserve(100);
  levelElements.reserve(100);

  /*
  Vertp3c a;
  Vertp3c b;
  Vertp3c c;
  a.position = V3(-1.0f, 0.0f, -1.0f);
  b.position = V3(1.0f, 0.0f, -1.0f);
  c.position = V3(0.0f, 1.0f, -1.0f);
  a.color = V3(0.0, 1.0, 0.0);
  b.color = V3(1.0, 0.0, 0.0);
  c.color = V3(0.0, 0.0, 1.0);

  test->addTri(a,b,c);

  a.position = V3(-1.0f, 0.0f, 1.0f);
  b.position = V3(1.0f, 0.0f, 1.0f);
  c.position = V3(0.0f, 1.0f, 1.0f);
  a.color = V3(0.0, 1.0, 0.0);
  b.color = V3(1.0, 0.0, 0.0);
  c.color = V3(0.0, 0.0, 1.0);

  test->addTri(a,b,c);
  */

  Mesh* test2 = new Mesh(12, &level, &levelElements);
  test2->addCube(V3(-.5,3,5.0), .25f, V3(1.0,1.0,1.0));
  meshes.push_back(test2);

  Mesh* test = new Mesh(12, &level, &levelElements);
  test->addCube(V3(0.0,1.0,0.0), 2.0f, V3(1.0,0.0,0.0));
  meshes.push_back(test);
// compile shaders///////////////////////////////////
  GLuint vertShader, fragShader;
  int success;
  vertShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertShader, 1, &vertexSource, NULL);
  glCompileShader(vertShader);
  // print compile errors if any
  glGetShaderiv(vertShader, GL_COMPILE_STATUS, &success);
  if(!success) {
    char buffer[512];
    glGetShaderInfoLog(vertShader, 512, NULL, buffer);
    OutputDebugString(buffer);
    InvalidCodePath;
  }

  fragShader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragShader, 1, &fragmentSource, NULL);
  glCompileShader(fragShader);
  // print compile errors if any
  glGetShaderiv(fragShader, GL_COMPILE_STATUS, &success);
  if(!success) {
    char buffer[512];
    glGetShaderInfoLog(fragShader, 512, NULL, buffer);
    OutputDebugString(buffer);
    InvalidCodePath;
  }

  // shader Program
  shaderID = glCreateProgram();
  glAttachShader(shaderID, vertShader);
  glAttachShader(shaderID, fragShader);
  glLinkProgram(shaderID);
  // print linking errors if any
  glGetProgramiv(shaderID, GL_LINK_STATUS, &success);
  if(!success)
  {
    char buffer[512];
    glGetProgramInfoLog(shaderID, 512, NULL, buffer);
    OutputDebugString(buffer);
    InvalidCodePath;
  }
  // delete the shaders as they're linked into our program now and no longer necessery
  glDeleteShader(vertShader);
  glDeleteShader(fragShader);
// compile shaders done ///////////////////////////////////
  
  GLuint vao;
  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);
  glGenBuffers(1, &ebo);
  glBindVertexArray(vao);
  
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, level.size()*sizeof(Vertpcn), level.data(), GL_DYNAMIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, levelElements.size()*sizeof(int), levelElements.data(), GL_DYNAMIC_DRAW);
  
  GLint posAttrib = glGetAttribLocation(shaderID, "position");
  glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(Vertpcn), 0);
  glEnableVertexAttribArray(posAttrib);
  GLint color = glGetAttribLocation(shaderID, "color");
  glVertexAttribPointer(color, 3, GL_FLOAT, GL_FALSE, sizeof(Vertpcn), (void*)(sizeof(v3)));
  glEnableVertexAttribArray(color);
  GLint normal = glGetAttribLocation(shaderID, "normal");
  glVertexAttribPointer(normal, 3, GL_FLOAT, GL_FALSE, sizeof(Vertpcn), (void*)(sizeof(v3)*2));
  glEnableVertexAttribArray(normal);

  glEnable(GL_DEPTH_TEST);
  glCullFace(GL_BACK);
  glEnable(GL_CULL_FACE);
}

void Mesh::addTri(Vertpcn a, Vertpcn b, Vertpcn c) {
  v3 normal;
  v3 first = a.position - b.position;
  v3 second = c.position - b.position;
  normal = normalize(cross(second, first));
  a.normal = normal;
  b.normal = normal;
  c.normal = normal;
  int index = elements->size();
  verts->push_back(a);
  elements->push_back(index++);
  verts->push_back(b);
  elements->push_back(index++);
  verts->push_back(c);
  elements->push_back(index++);
}

void Mesh::addCube(v3 center, float width, v3 color) {
  meshCenter = center;
  float widthOffset = width/2.0f;
  Vertpcn a, b, c, d;
  v3 xOff, yOff, zOff;
  xOff = V3(widthOffset, 0.0, 0.0);
  yOff = V3(0.0, widthOffset, 0.0);
  zOff = V3(0.0, 0.0, widthOffset);
  
  a.color = color;
  b.color = color;
  c.color = color;
  d.color = color;

  //top
  a.position = (((center - zOff) + yOff) - xOff);
  c.position = (((center + zOff) + yOff) - xOff);
  b.position = (((center - zOff) + yOff) + xOff);
  d.position = (((center + zOff) + yOff) + xOff);
  addTri(c,b,a);
  addTri(b,c,d);
  //bottom
  a.position = (((center - zOff) - yOff) - xOff);
  c.position = (((center + zOff) - yOff) - xOff);
  b.position = (((center - zOff) - yOff) + xOff);
  d.position = (((center + zOff) - yOff) + xOff);
  addTri(a,b,c);
  addTri(d,c,b);
  //front
  a.position = (((center + zOff) + yOff) - xOff);
  c.position = (((center + zOff) - yOff) - xOff);
  b.position = (((center + zOff) + yOff) + xOff);
  d.position = (((center + zOff) - yOff) + xOff);
  addTri(a,c,b);
  addTri(b,c,d);
  //back
  a.position = (((center - zOff) + yOff) - xOff);
  c.position = (((center - zOff) - yOff) - xOff);
  b.position = (((center - zOff) + yOff) + xOff);
  d.position = (((center - zOff) - yOff) + xOff);
  addTri(b,c,a);
  addTri(d,c,b);
  //left
  a.position = (((center + zOff) + yOff) - xOff);
  b.position = (((center + zOff) - yOff) - xOff);
  c.position = (((center - zOff) + yOff) - xOff);
  d.position = (((center - zOff) - yOff) - xOff);
  addTri(c,b,a);
  addTri(b,c,d);
  //right
  a.position = (((center + zOff) + yOff) + xOff);
  b.position = (((center + zOff) - yOff) + xOff);
  c.position = (((center - zOff) + yOff) + xOff);
  d.position = (((center - zOff) - yOff) + xOff);
  addTri(a,b,c);
  addTri(d,c,b);
}

void Renderer::draw() {
   glDrawElements(GL_TRIANGLES, level.size(), GL_UNSIGNED_INT, 0);
}