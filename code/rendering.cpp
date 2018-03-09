#include <Windows.h>
#include <gl/GL.h>
#include <math.h>
#include <stdint.h>

#include "aro_generic.h"
#include "aro_opengl.h"
#include "aro_math.h"
#include "rendering.h"

const char* vertexSource = R"FOO(
#version 330 core
layout (location = 0) in vec3 position;
uniform mat4 projection;
void main()
{
  gl_Position = projection * vec4(position.x, position.y, position.z, 1.0f);
}
)FOO";

const char* fragmentSource = R"FOO(
#version 330 core
uniform vec3 colorIn;
out vec4 resultColor;
void main()
{
  resultColor = vec4(colorIn.r, colorIn.g, colorIn.b, 1.0);
}
)FOO";

void Renderer::initialize() {
  mesh = new Mesh(1);
  Triangle* tri = new Triangle(V3(-0.5, -0.5, -1.0), V3(0, .5, -1.0), V3(.5,-.5, -1.0));
  mesh->addTri(tri);

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

  GLuint vao;
  
  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);
  glGenBuffers(1, &ebo);
  glBindVertexArray(vao);
  
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, mesh->numTris*sizeof(Triangle), mesh->tris, GL_DYNAMIC_DRAW);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->numTris*3*sizeof(int), mesh->elements, GL_DYNAMIC_DRAW);
  
  GLint posAttrib = glGetAttribLocation(shaderID, "position");
  //GLint color = glGetAttribLocation(shaderID, "color");
  glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), 0);
  glEnableVertexAttribArray(posAttrib);
  //glVertexAttribPointer(color, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)(3*sizeof(float)));
  //glEnableVertexAttribArray(color);
}

void Mesh::addTri(Triangle* tri) {
  Triangle* meshtri = tris+nextTri;
  *meshtri = *tri;
  int* elementsPtr = elements + (nextTri*3);
  int elenumber = nextTri*3;
  *elementsPtr = elenumber++;
  elementsPtr++;
  *elementsPtr =  elenumber++;
  elementsPtr++;
  *elementsPtr =  elenumber;
  elementsPtr++;
  nextTri++;

}

void Renderer::draw() {
  glDrawElements(GL_TRIANGLES, mesh->numTris*3, GL_UNSIGNED_INT, 0);
}