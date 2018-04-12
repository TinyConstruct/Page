#include <Windows.h>
#include <gl/gl.h>
#include <math.h>
#include <stdint.h>
#include <vector>
#include <memory.h>  

#include "aro_platform_win32.h"
#include "aro_generic.h"
#include "aro_opengl.h"
#include "texture.h"
#include "aro_math.h"
#include "gamestate.h"

#include "rendering.h"
const char* vertexSource = R"FOO(
#version 450 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec3 texCoordIn;
out vec3 colorOutFromVert;
out vec3 normalOutFromVert;
out vec3 fragPos;
out vec3 texCoordOutFromVert;
uniform mat4 projection;
uniform mat4 view;

void main()
{
  gl_Position =  projection * view * vec4(position.x, position.y, position.z, 1.0f);
  fragPos = position.xyz;
  colorOutFromVert = color;
  normalOutFromVert = normal;
  texCoordOutFromVert = texCoordIn;
}
)FOO";

const char* vertexSourceUBUF = R"FOO(
#version 450 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec3 texCoordIn;
layout(std140) uniform GlobalMatrices {
  mat4 view;
  mat4 projection;
  vec3 playerPos;
};
out vec3 colorOutFromVert;
out vec3 normalOutFromVert;
out vec3 fragPos;
out vec3 texCoordOutFromVert;

void main()
{
  gl_Position =  projection * view * vec4(position.x, position.y, position.z, 1.0f);
  fragPos = position.xyz;
  colorOutFromVert = color;
  normalOutFromVert = normal;
  texCoordOutFromVert = texCoordIn;
}
)FOO";

const char* fragmentSource = R"FOO(
#version 450 core
uniform sampler2DArray textureData;
in vec3 colorOutFromVert;
in vec3 normalOutFromVert;
in vec3 fragPos;
in vec3 texCoordOutFromVert;
layout(std140) uniform GlobalMatrices {
  mat4 view;
  mat4 projection;
  vec3 playerPos;
};
out vec4 resultColor;
void main()
{
  vec3 lightPos = playerPos;
  vec4 texColor = texture(textureData, vec3(texCoordOutFromVert.xyz));
  vec3 lightColor = vec3(1.0,1.0,1.0);
  float ambientIntensity= .1;
  float specularIntensity = 1.0;
  vec3 viewerDir = normalize(playerPos - fragPos);
  vec3 lightDir = normalize(lightPos - fragPos);
  vec3 norm = normalize(normalOutFromVert);
  vec3 reflectedDir = reflect(-lightDir, norm);

  float spec = pow(max(dot(viewerDir, reflectedDir), 0.0), 32);

  float diff = max(dot(norm, lightDir), 0.0);
  vec3 ambient = lightColor*ambientIntensity;
  vec3 diffuse = diff * lightColor;
  vec3 specular = specularIntensity * spec * lightColor;  
  resultColor = (vec4((ambient + diffuse + specular), 1.0) * vec4(texColor.rgb, 1.0));
}
)FOO";

const char* debugFragmentSource = R"FOO(
#version 450 core
in vec3 colorOutFromVert;
in vec3 normalOutFromVert;
in vec3 fragPos;
in vec3 texCoordOutFromVert;
uniform vec3 lightPos;
uniform sampler2DArray textureData;
uniform vec3 playerPos;
out vec4 resultColor;
void main()
{
  vec4 texColor = texture(textureData, vec3(texCoordOutFromVert.xyz));
  vec3 lightColor = vec3(1.0,1.0,1.0);
  float ambientIntensity= .1;
  float specularIntensity = 1.0;
  vec3 viewerDir = normalize(playerPos - fragPos);
  vec3 lightDir = normalize(lightPos - fragPos);
  vec3 norm = normalize(normalOutFromVert);
  vec3 reflectedDir = reflect(-lightDir, norm);

  float spec = pow(max(dot(viewerDir, reflectedDir), 0.0), 32);

  float diff = max(dot(norm, lightDir), 0.0);
  vec3 ambient = lightColor*ambientIntensity;
  vec3 diffuse = diff * lightColor;
  vec3 specular = specularIntensity * spec * lightColor;  
  resultColor = (vec4((ambient + diffuse + specular), 1.0) * vec4(texColor.rgb, 0.5));
}
)FOO";

int texStrToID(char* str) {
  if(strcmpAny(str, "ceil1.bmp")) {
    return CEIL1;
  }
  else if(strcmpAny(str, "city2_2.bmp")) {
    return CITY2_2;
  }
  else if(strcmpAny(str, "city3_2.bmp")) {
    return CITY3_2;
  }
  else if(strcmpAny(str, "city4_1.bmp")) {
    return CITY4_1;
  }
  else if(strcmpAny(str, "floor1.bmp")) {
    return FLOOR1;
  }
  else if(strcmpAny(str, "floor2.bmp")) {
    return FLOOR2;
  }
  else if(strcmpAny(str, "floor3.bmp")) {
    return FLOOR3;
  }
  else if(strcmpAny(str, "metalbox.bmp")) {
    return METALBOX;
  }
  else if(strcmpAny(str, "wall1.bmp")) {
    return WALL1;
  }
  else if(strcmpAny(str, "wall2.bmp")) {
    return WALL2;
  }
  else {
    InvalidCodePath;
  }
}

void compileShader(GLuint* shaderID, const char* vSrc, const char* fSrc) {
  GLuint vertShader, fragShader;
  int success;
  vertShader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertShader, 1, &vSrc, NULL);
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
  glShaderSource(fragShader, 1, &fSrc, NULL);
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
  *shaderID = glCreateProgram();
  glAttachShader(*shaderID, vertShader);
  glAttachShader(*shaderID, fragShader);
  glLinkProgram(*shaderID);
  // print linking errors if any
  glGetProgramiv(*shaderID, GL_LINK_STATUS, &success);
  if(!success)
  {
    char buffer[512];
    glGetProgramInfoLog(*shaderID, 512, NULL, buffer);
    OutputDebugString(buffer);
    InvalidCodePath;
  }
  // delete the shaders as they're linked into our program now and no longer necessery
  glDeleteShader(vertShader);
  glDeleteShader(fragShader);
  GLuint uniformBlockIndex = glGetUniformBlockIndex(*shaderID, "GlobalMatrices");
  glUniformBlockBinding(*shaderID, uniformBlockIndex, 0);
}

void Renderer::initialize() {
  //meshes.reserve(100);
  level.reserve(100);
  levelElements.reserve(100);

  compileShader(&shaderID, vertexSourceUBUF, fragmentSource);
  GLuint vao;
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, level.size()*sizeof(Vertpcnu), level.data(), GL_DYNAMIC_DRAW);
  glGenBuffers(1, &ebo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, levelElements.size()*sizeof(int), levelElements.data(), GL_DYNAMIC_DRAW);
  glGenBuffers(1, &uniformBuffer);
  glBindBuffer(GL_UNIFORM_BUFFER, uniformBuffer);
  glBufferData(GL_UNIFORM_BUFFER, sizeof(m4x4) * 2 + sizeof(v3), NULL, GL_STREAM_DRAW);
  glBindBufferRange(GL_UNIFORM_BUFFER, 0, uniformBuffer, 0, sizeof(m4x4) * 2 + sizeof(v3));
  
  GLint posAttrib = glGetAttribLocation(shaderID, "position");
  glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(Vertpcnu), 0);
  glEnableVertexAttribArray(posAttrib);
  GLint color = glGetAttribLocation(shaderID, "color");
  glVertexAttribPointer(color, 3, GL_FLOAT, GL_FALSE, sizeof(Vertpcnu), (void*)(sizeof(v3)));
  glEnableVertexAttribArray(color);
  GLint normal = glGetAttribLocation(shaderID, "normal");
  glVertexAttribPointer(normal, 3, GL_FLOAT, GL_FALSE, sizeof(Vertpcnu), (void*)(sizeof(v3)*2));
  glEnableVertexAttribArray(normal);
  GLint texCoord = glGetAttribLocation(shaderID, "texCoordIn");
  glVertexAttribPointer(texCoord, 3, GL_FLOAT, GL_FALSE, sizeof(Vertpcnu), (void*)(sizeof(v3)*3));
  glEnableVertexAttribArray(texCoord);

  #if DEBUG_BUILD
    compileShader(&debugShaderID, vertexSource, debugFragmentSource);
    debugBoundingVerts.reserve(100);
    debugBoundingElements.reserve(100);
    renderDebug = false;
    glGenBuffers(1, &debugVbo);
    glGenBuffers(1, &debugEbo);
    glBindBuffer(GL_ARRAY_BUFFER, debugVbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, debugEbo);
    glBufferData(GL_ARRAY_BUFFER, debugBoundingVerts.size()*sizeof(v3), debugBoundingVerts.data(), GL_DYNAMIC_DRAW);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, debugBoundingElements.size()*sizeof(int), debugBoundingElements.data(), GL_DYNAMIC_DRAW);
  #endif
  
  glEnable(GL_DEPTH_TEST);
  glCullFace(GL_BACK);
  glEnable(GL_CULL_FACE);
}

/*
void Mesh::addTri(Vertpcnu a, Vertpcnu b, Vertpcnu c) {
  v3 normal;
  v3 first = a.position - b.position;
  v3 second = c.position - b.position;
  normal = normalize(cross(second, first));
  a.normal = normal;
  b.normal = normal;
  c.normal = normal;
  int index = verts->size();
  verts->push_back(a);
  elements->push_back(index++);
  verts->push_back(b);
  elements->push_back(index++);
  verts->push_back(c);
  elements->push_back(index++);
}

void Mesh::addCube(v3 center, float width, v3 color) {
  float widthOffset = width/2.0f;
  Vertpcnu a, b, c, d;
  v3 xOff, yOff, zOff;
  xOff = V3(widthOffset, 0.0, 0.0);
  yOff = V3(0.0, widthOffset, 0.0);
  zOff = V3(0.0, 0.0, widthOffset);
  
  a.color = color;
  b.color = color;
  c.color = color;
  d.color = color;

  //top
  a.uv = V3(0.0,1.0,0.0);
  b.uv = V3(0.0, 0.0, 0.0);
  c.uv = V3(1.0, 0.0, 0.0);
  d.uv = V3(1.0, 1.0, 0.0);
  a.position = (((center - zOff) + yOff) - xOff);
  b.position = (((center + zOff) + yOff) - xOff);
  c.position = (((center + zOff) + yOff) + xOff);
  d.position = (((center - zOff) + yOff) + xOff);
  addTri(a,b,d);
  addTri(b,c,d);
  //bottom
  a.position = (((center + zOff) - yOff) - xOff);
  b.position = (((center - zOff) - yOff) - xOff);
  c.position = (((center - zOff) - yOff) + xOff);
  d.position = (((center + zOff) - yOff) + xOff);
  addTri(a,b,d);
  addTri(b,c,d);
  //front
  a.position = (((center + zOff) + yOff) - xOff);
  b.position = (((center + zOff) - yOff) - xOff);
  c.position = (((center + zOff) - yOff) + xOff);
  d.position = (((center + zOff) + yOff) + xOff);
  addTri(a,b,d);
  addTri(b,c,d);
  //back
  a.position = (((center - zOff) + yOff) + xOff);
  b.position = (((center - zOff) - yOff) + xOff);
  c.position = (((center - zOff) - yOff) - xOff);
  d.position = (((center - zOff) + yOff) - xOff);
  addTri(a,b,d);
  addTri(b,c,d);
  //left
  a.position = (((center - zOff) + yOff) - xOff);
  b.position = (((center - zOff) - yOff) - xOff);
  c.position = (((center + zOff) - yOff) - xOff);
  d.position = (((center + zOff) + yOff) - xOff);
  addTri(a,b,d);
  addTri(b,c,d);
  //right
  a.position = (((center + zOff) + yOff) + xOff);
  b.position = (((center + zOff) - yOff) + xOff);
  c.position = (((center - zOff) - yOff) + xOff);
  d.position = (((center - zOff) + yOff) + xOff);
  addTri(a,b,d);
  addTri(b,c,d);
}

void Mesh::addQuad(v3 vert1, v3 vert2, v3 vert3, v3 vert4, v3 color) {
  Vertpcnu a, b, c, d;
  a.position = vert1;
  a.uv = V3(0.0, 0.0, 0.0);
  b.position = vert2;
  b.uv = V3(0.0, 1.0, 0.0);
  c.position = vert3;
  c.uv = V3(1.0, 1.0, 0.0);
  d.position = vert4;
  d.uv = V3(1.0, 0.0, 0.0);
  a.color = color;
  b.color = color;
  c.color = color;
  d.color = color;
  addTri(a,b,c);
  addTri(c,d,a);
}

void Mesh::addQuad(v3 vert1, v3 vert2, v3 vert3, v3 vert4, v3 color, LevelGeometry* level) {
  Vertpcnu a, b, c, d;
  a.position = vert1;
  a.uv = V3(0.0, 0.0, 0.0);
  b.position = vert2;
  b.uv = V3(0.0, 1.0, 0.0);
  c.position = vert3;
  c.uv = V3(1.0, 1.0, 0.0);
  d.position = vert4;
  d.uv = V3(1.0, 0.0, 0.0);
  a.color = color;
  b.color = color;
  c.color = color;
  d.color = color;
  addTri(a,b,c);
  addTri(c,d,a);
  v3 center = .5*(a.position - c.position) + c.position;
  v3 rad;
  //note: currently assuming axis-aligned, either wall or floor
  if(a.position.y == b.position.y) {//floor or ceiling
    rad.x = magnitude(a.position - b.position)/2.0;
    rad.y = 0.2;
    rad.z = magnitude(b.position - c.position)/2.0;
  }
  else {
    rad.x = max(0.2, 1.05*(abs(b.position.x - c.position.x)/2.0));
    rad.y = 1.05*(abs(b.position.y - a.position.y) / 2.0);
    rad.z = max(0.2, (abs(b.position.z - a.position.z)/2.0));
  }
  level->addAABB(center, rad);
}

void Mesh::addCircle(v3 cirCenter, float rad, float numPts) {
  Vertpcnu oldVert, newVert, center;
  v3 normal = V3(0.0,0.1,0.0);
  oldVert.normal = normal;
  newVert.normal = normal;
  center.normal = normal;
  center.position = cirCenter;
  center.uv = V3(.5, .5, 0.0);
  int centerIndex = elements->size();
  int index = centerIndex+1;
  float aCos = cos(0.0f);
  float aSin = sin(0.0f);
  float rot = 2*M_PI/numPts;
  oldVert.uv = V3((aCos + 1) / 2, (aSin + 1) / 2, 0.0);
  oldVert.position = V3(aCos*rad, center.position.y, aSin*rad);

  verts->push_back(center);
  verts->push_back(oldVert);
  for(float i = 1.0; i <= numPts; i++) {
    float bCos = cos((i)*rot);
    float bSin = sin((i)*rot);
    
    newVert.uv = V3((bCos + 1) / 2, (bSin + 1) / 2, 0.0);
    newVert.position = V3(bCos*rad, center.position.y, bSin*rad);
    verts->push_back(newVert);
    
    elements->push_back(centerIndex);
    elements->push_back(index+1);
    elements->push_back(index);
    index += 1;
  }
}
*/


void Renderer::draw() {
  //glEnable(GL_FRAMEBUFFER_SRGB); 
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  GLint posAttrib = glGetAttribLocation(shaderID, "position");
  glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(Vertpcnu), 0);
  glEnableVertexAttribArray(posAttrib);
  GLint color = glGetAttribLocation(shaderID, "color");
  glVertexAttribPointer(color, 3, GL_FLOAT, GL_FALSE, sizeof(Vertpcnu), (void*)(sizeof(v3)));
  glEnableVertexAttribArray(color);
  GLint normal = glGetAttribLocation(shaderID, "normal");
  glVertexAttribPointer(normal, 3, GL_FLOAT, GL_FALSE, sizeof(Vertpcnu), (void*)(sizeof(v3)*2));
  glEnableVertexAttribArray(normal);
  GLint texCoord = glGetAttribLocation(shaderID, "texCoordIn");
  glVertexAttribPointer(texCoord, 3, GL_FLOAT, GL_FALSE, sizeof(Vertpcnu), (void*)(sizeof(v3)*3));
  glEnableVertexAttribArray(texCoord);
  glDrawElements(GL_TRIANGLES, levelElements.size(), GL_UNSIGNED_INT, 0);

  #if DEBUG_BUILD
    if(renderDebug){
      //glUseProgram(debugShaderID);
      glEnable(GL_LINE_SMOOTH);
      glLineWidth(50.0);
      glBindBuffer(GL_ARRAY_BUFFER, debugVbo);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, debugEbo);
      posAttrib = glGetAttribLocation(shaderID, "position");
      glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(v3), 0);
      glEnableVertexAttribArray(posAttrib);
      glDrawElements(GL_LINES, debugBoundingElements.size(), GL_UNSIGNED_INT, 0);
      glLineWidth(1.0);
    }
  #endif
}

void Renderer::addDebugVolume(const v3& center, const v3& rad) {
  v3 newVert, offset;
  int startingIndex = debugBoundingVerts.size();
  offset = rad;
  //front plane
  newVert = V3(center.x - offset.x, center.y + offset.y, center.z + offset.z);//0
  debugBoundingVerts.push_back(newVert);
  newVert = V3(center.x - offset.x, center.y - offset.y, center.z + offset.z);//1
  debugBoundingVerts.push_back(newVert);
  newVert = V3(center.x + offset.x, center.y - offset.y, center.z + offset.z);//2
  debugBoundingVerts.push_back(newVert);
  newVert = V3(center.x + offset.x, center.y + offset.y, center.z + offset.z);//3
  debugBoundingVerts.push_back(newVert);
  //back plane
  newVert = V3(center.x - offset.x, center.y + offset.y, center.z - offset.z);//4
  debugBoundingVerts.push_back(newVert);
  newVert = V3(center.x - offset.x, center.y - offset.y, center.z - offset.z);//5
  debugBoundingVerts.push_back(newVert);
  newVert = V3(center.x + offset.x, center.y - offset.y, center.z - offset.z);//6
  debugBoundingVerts.push_back(newVert);
  newVert = V3(center.x + offset.x, center.y + offset.y, center.z - offset.z);//7
  debugBoundingVerts.push_back(newVert);
  //front
  debugBoundingElements.push_back(startingIndex);
  debugBoundingElements.push_back(startingIndex+1);
  debugBoundingElements.push_back(startingIndex+1);
  debugBoundingElements.push_back(startingIndex+2);
  debugBoundingElements.push_back(startingIndex+2);
  debugBoundingElements.push_back(startingIndex+3);
  debugBoundingElements.push_back(startingIndex+3);
  debugBoundingElements.push_back(startingIndex+0);
  //back
  debugBoundingElements.push_back(startingIndex+4);
  debugBoundingElements.push_back(startingIndex+5);
  debugBoundingElements.push_back(startingIndex+5);
  debugBoundingElements.push_back(startingIndex+6);
  debugBoundingElements.push_back(startingIndex+6);
  debugBoundingElements.push_back(startingIndex+7);
  debugBoundingElements.push_back(startingIndex+7);
  debugBoundingElements.push_back(startingIndex+4);
  //top
  debugBoundingElements.push_back(startingIndex+4);
  debugBoundingElements.push_back(startingIndex);
  debugBoundingElements.push_back(startingIndex+7);
  debugBoundingElements.push_back(startingIndex+3);
  //bottom
  debugBoundingElements.push_back(startingIndex+5);
  debugBoundingElements.push_back(startingIndex+1);
  debugBoundingElements.push_back(startingIndex+6);
  debugBoundingElements.push_back(startingIndex+2);
}

void Renderer::addDebugVolume(v3& center, v3 axes[3], v3& halfW) {

  v3 newVert, offset;
  int startingIndex = debugBoundingVerts.size();
  offset = halfW;

  //front plane
  //newVert = V3(center.x - offset.x, center.y + offset.y, center.z + offset.z);//0
  newVert = center - halfW.x*axes[0] + halfW.y*axes[1] + halfW.z*axes[2];
  debugBoundingVerts.push_back(newVert);
  //newVert = V3(center.x - offset.x, center.y - offset.y, center.z + offset.z);//1
  newVert = center - halfW.x*axes[0] - halfW.y*axes[1] + halfW.z*axes[2];
  debugBoundingVerts.push_back(newVert);
  //newVert = V3(center.x + offset.x, center.y - offset.y, center.z + offset.z);//2
  newVert = center + halfW.x*axes[0] - halfW.y*axes[1] + halfW.z*axes[2];
  debugBoundingVerts.push_back(newVert);
  //newVert = V3(center.x + offset.x, center.y + offset.y, center.z + offset.z);//3
  newVert = center + halfW.x*axes[0] + halfW.y*axes[1] + halfW.z*axes[2];
  debugBoundingVerts.push_back(newVert);
  
  //back plane
  //newVert = V3(center.x - offset.x, center.y + offset.y, center.z - offset.z);//4
  newVert = center - halfW.x*axes[0] + halfW.y*axes[1] - halfW.z*axes[2];
  debugBoundingVerts.push_back(newVert);
  //newVert = V3(center.x - offset.x, center.y - offset.y, center.z - offset.z);//5
  newVert = center - halfW.x*axes[0] - halfW.y*axes[1] - halfW.z*axes[2];
  debugBoundingVerts.push_back(newVert);
  //newVert = V3(center.x + offset.x, center.y - offset.y, center.z - offset.z);//6
  newVert = center + halfW.x*axes[0] - halfW.y*axes[1] - halfW.z*axes[2];
  debugBoundingVerts.push_back(newVert);
  //newVert = V3(center.x + offset.x, center.y + offset.y, center.z - offset.z);//7
  newVert = center + halfW.x*axes[0] + halfW.y*axes[1] - halfW.z*axes[2];
  debugBoundingVerts.push_back(newVert);

  debugBoundingElements.push_back(startingIndex);
  debugBoundingElements.push_back(startingIndex+1);
  debugBoundingElements.push_back(startingIndex+1);
  debugBoundingElements.push_back(startingIndex+2);
  debugBoundingElements.push_back(startingIndex+2);
  debugBoundingElements.push_back(startingIndex+3);
  debugBoundingElements.push_back(startingIndex+3);
  debugBoundingElements.push_back(startingIndex+0);
  //back
  debugBoundingElements.push_back(startingIndex+4);
  debugBoundingElements.push_back(startingIndex+5);
  debugBoundingElements.push_back(startingIndex+5);
  debugBoundingElements.push_back(startingIndex+6);
  debugBoundingElements.push_back(startingIndex+6);
  debugBoundingElements.push_back(startingIndex+7);
  debugBoundingElements.push_back(startingIndex+7);
  debugBoundingElements.push_back(startingIndex+4);
  //top
  debugBoundingElements.push_back(startingIndex+4);
  debugBoundingElements.push_back(startingIndex);
  debugBoundingElements.push_back(startingIndex+7);
  debugBoundingElements.push_back(startingIndex+3);
  //bottom
  debugBoundingElements.push_back(startingIndex+5);
  debugBoundingElements.push_back(startingIndex+1);
  debugBoundingElements.push_back(startingIndex+6);
  debugBoundingElements.push_back(startingIndex+2);  
}

GLuint loadTexture(Texture* texture, char* bmpPath) {
  GLuint texID;
  glGenTextures(1, &texID);
  glBindTexture(GL_TEXTURE_2D, texID);
  //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  texture->data = loadBMP(bmpPath);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texture->data.width, texture->data.height, 0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, texture->data.contents);
  if(!freeBMP(&texture->data)) {
    InvalidCodePath; //Failed to free bitmap
  }
  return texID;
}

void loadTextureIntoArray(char* bmpPath, TextureHandle* th, int width, int height) {
  Texture texture;
  texture.data = loadBMP(bmpPath);
  assert(height == texture.data.height);
  assert(width == texture.data.width);
  
  if(!freeBMP(&texture.data)) {
    InvalidCodePath; //Failed to free bitmap
  }
}

GLuint Renderer::createTextureArrayBuffer(int width, int height) {
  /*
  GLint maxTexDim, maxTexLayers, maxTexUnits;
  glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTexDim);
  glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &maxTexLayers);
  glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTexUnits);
  int layerCount = maxTexDim/4/texDim;
  //TODO: this probably wrong??? maxIndex/4 != maxBytes????
  assert(maxTexDim/4 >= 2048);
  assert(maxTexUnits >= 32); //Note: we know that this is just for the frag shader
*/
  assert(width == height);
  int layerCount = 2048/height;
  GLuint texID;
  glGenTextures(1, &texID);
  glBindTexture(GL_TEXTURE_2D_ARRAY, texID);
  glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA8, width, height, layerCount);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  return texID;
}

void Renderer::loadTextureArray512(int aID, char* aPath, int bID, char* bPath, int cID, char* cPath, int dID, char* dPath){
  GLuint texID = createTextureArrayBuffer(512,512);
  Bitmap aBMP, bBMP, cBMP, dBMP;
  int cpyOffset = 512*512*4;
  char* buffer = (char*)VirtualAlloc(0, 512*512*4*4, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
  char* cpyDest = buffer;
  if(buffer && aPath!=NULL) {
    aBMP = loadBMP(aPath);
    memcpy(cpyDest, (char*)aBMP.contents, cpyOffset);
    cpyDest += cpyOffset;
    freeBMP(&aBMP);
    texTable[aID].glTextureNum = texID;
    texTable[aID].texLayer = 0;
  }
  else {
    InvalidCodePath;
  }
  if(bPath!=NULL) {
    bBMP = loadBMP(bPath);
    memcpy(cpyDest, bBMP.contents, cpyOffset);
    cpyDest += cpyOffset;
    freeBMP(&bBMP);
    texTable[bID].glTextureNum = texID;
    texTable[bID].texLayer = 1;
  }
  if(cPath!=NULL) {
    cBMP = loadBMP(cPath);
    memcpy(cpyDest, cBMP.contents, cpyOffset);
    cpyDest += cpyOffset;
    freeBMP(&cBMP);
    texTable[cID].glTextureNum = texID;
    texTable[cID].texLayer = 2;
  }
  if(dPath!=NULL) {
    dBMP = loadBMP(dPath);
    memcpy(cpyDest, dBMP.contents, cpyOffset);
    freeBMP(&dBMP);
    texTable[dID].glTextureNum = texID;
    texTable[dID].texLayer = 3;
  }
  glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, 512, 512, 4, GL_BGRA_EXT, GL_UNSIGNED_BYTE, buffer);
  VirtualFree(buffer, 0, MEM_RELEASE);
}

TextureHandle Renderer::getGLTexID(int texID) {
  TextureHandle th = texTable[texID];
  return th;
}

void Renderer::addTri(Vertpcnu& a, Vertpcnu& b, Vertpcnu& c) {
  v3 normal;
  v3 first = a.position - b.position;
  v3 second = c.position - b.position;
  normal = normalize(cross(second, first));
  a.normal = normal;
  b.normal = normal;
  c.normal = normal;
  int index = levelElements.size();
  level.push_back(a);
  levelElements.push_back(index++);
  level.push_back(b);
  levelElements.push_back(index++);
  level.push_back(c);
  levelElements.push_back(index++);
}

void Renderer::debugDrawLine(v3& start, v3& end) {
  int startingIndex = debugBoundingVerts.size();
  debugBoundingVerts.push_back(start);
  debugBoundingVerts.push_back(end);
  debugBoundingElements.push_back(startingIndex);
  debugBoundingElements.push_back(startingIndex+1);
}