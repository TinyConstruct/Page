#include <vector>
#include <Windows.h>
#include <gl/gl.h>
#include <cstring>
#include "aro_math.h"
#include "texture.h"
#include "rendering.h"
#include "aro_generic.h"
#include "aro_platform_win32.h"

#include "level.h"
void LevelGeometry::addAABB(v3& center, v3& rad) {
  AABB box;
  box.center = center;
  box.rad = rad;
  AABBs.push_back(box);
}

void LevelGeometry::initialize() {
  
}

void LevelGeometry::addOOB(v3& center, v3 axes[3], v3& halfW) {
  OBB box;
  box.c = center;
  box.u[0] = axes[0];
  box.u[1] = axes[1];
  box.u[2] = axes[2];
  box.width = halfW;
  OBBs.push_back(box);
}

void LevelData::initialize(LevelGeometry* g, Renderer* r) {
  quads = new std::vector<TexturedQuad>;
  tris = new std::vector<Tri>;
  renderer = r;
  geo = g;
}

void LevelData::addTexturedQuad(v3& vert1, v3& vert2, v3& vert3, v3& vert4, int texID) {
  TexturedQuad q;
  q.a = vert1;
  q.b = vert2;
  q.c = vert3;
  q.d = vert4;
  q.texHandle = renderer->getGLTexID(texID);
  quads->push_back(q);
}
void LevelData::addTexturedQuad(TexturedQuad& q) {
  quads->push_back(q);
}

void LevelData::addTexturedQuad(v3& vert1, v3& vert2, v3& vert3, v3& vert4, int texID, LevelGeometry* level) {
  TexturedQuad q;
  q.a = vert1;
  q.b = vert2;
  q.c = vert3;
  q.d = vert4;
  q.texHandle = renderer->getGLTexID(texID);
  quads->push_back(q);
  /*
  v3 center = .5*(q.a - q.c) + q.c;
  v3 rad;
  //note: currently assuming axis-aligned, either wall or floor
  if(q.a.y == q.b.y) {//floor or ceiling
    rad.x = magnitude(q.a - q.b)/2.0;
    rad.y = 0.2;
    rad.z = magnitude(q.b- q.c)/2.0;
  }
  else {
    rad.x = max(0.2, 1.05*(abs(q.b.x - q.c.x)/2.0));
    rad.y = 1.05*(abs(q.b.y - q.a.y) / 2.0);
    rad.z = max(0.2, (abs(q.b.z - q.a.z)/2.0));
  }
  level->addAABB(center, rad);*/
}

void LevelData::finalizeQuads() {
  for(size_t i = 0; i < quads->size(); i++){

  //add rendering information: 
    Vertpcnu a, b, c, d;
    v2 uvScale = (*quads)[i].texScale;
    a.position = (*quads)[i].a;
    a.uv = V3(0.0f, 1.0f * uvScale.y, (float) (*quads)[i].texHandle.texLayer);
    b.position = (*quads)[i].b;
    b.uv = V3(0.0f, 0.0f, (float)(*quads)[i].texHandle.texLayer);
    c.position = (*quads)[i].c;
    c.uv = V3(1.0f*uvScale.x, 0.0f, (float)(*quads)[i].texHandle.texLayer);
    d.position = (*quads)[i].d;
    d.uv = V3(1.0f*uvScale.x, 1.0f*uvScale.y, (float)(*quads)[i].texHandle.texLayer);
    renderer->addTri(a,b,c);
    renderer->addTri(c, d, a);
    v3 center = .5f*(a.position - c.position) + c.position;
    v3 rad;

  //add collision geometry:
    //test if axis aligned
    if( ((a.position.x == b.position.x) && (b.position.x == c.position.x) &&
          (c.position.x == d.position.x)) || 
        ((a.position.y == b.position.y) && (b.position.y == c.position.y) &&
          (c.position.y == d.position.y)) ||
        ((a.position.z == b.position.z) && (b.position.z == c.position.z) &&
          (c.position.z == d.position.z)) ){
      if(a.position.y == b.position.y) {//floor or ceiling
        rad.x = magnitude(a.position - b.position)/2.0f;
        rad.y = 0.01f;
        rad.z = magnitude(b.position - c.position)/2.0f;
      }
      else {
        rad.x = max(0.125f, 1.05f*(abs(b.position.x - c.position.x)/2.0f));
        rad.y = 1.05f*(abs(b.position.y - a.position.y) / 2.0f);
        rad.z = max(0.125f, (abs(b.position.z - a.position.z)/2.0f));
      }
      geo->addAABB(center, rad);
    }
    //if not axis aligned, create a bounding box along local axes
    //note: assume rectangle for now
    else {
      v3 axes[3];
      v3 dir1, dir2;
      dir1 = d.position - a.position;
      dir2 = a.position - b.position;

      axes[0] = normalize(dir1);
      axes[1] = normalize(dir2);
      axes[2] = cross(axes[0], axes[1]);

      //renderer->debugDrawLine(center, center + 20*rad.x*axes[0]);
      //renderer->debugDrawLine(center, center + 20*rad.y*axes[1]);
      //renderer->debugDrawLine(center, center + 20*rad.z*axes[2]);

      rad.x = magnitude(dir1)*1.1f/2.0f;
      rad.y = magnitude(dir2)*1.1f/2.0f;
      rad.z = 0.125f;

      geo->addOOB(center, axes, rad);
    }
  }
}

void LevelData::bakeTestLevel() {
  //add a bunch of geometry to a buffer: (quad,TEX_NUM)...
  
  //addTexturedQuad(V3(-20.0, 0.0, -20.0), V3(-20.0, 0.0, 20.0), V3(20.0, 0.0, 20.0), V3(20.0, 0.0, -20.0),  FLOOR1);
  //addTexturedQuad(V3(-20.0, 6.0, -20.0), V3(20.0, 6.0, -20.0), V3(20.0, 6.0, 20.0), V3(-20.0, 6.0, 20.0),  CEIL1);
  //addTexturedQuad(V3(-1.5, 3.0, -5.0), V3(-1.5, 0.0, -5.0), V3(1.5, 0.0, -5.0), V3(1.5, 3.0, -5.0),  WALL2);
  //addTexturedQuad(V3(-3.5, 3.0, -3.0), V3(-3.5, 3.0, 0.0), V3(0.0, 1.0, 0.0), V3(0.0, 1.0, -3.0),  WALL2);
  loadLevelFromTextFile("data/dungeon1.dat");
  //other buffer: (tri, TEX_NUM)
  //sort on getGLTexID(TEX_NUM) for both buffers
  //send quads/tris to renderer
  finalizeQuads();
  //send quads/tris to LevelGeometry for bounding boxes
}

void LevelData::preLoadTextureArray512(int numTexes, char* tex1, char* tex2, char* tex3, char* tex4) {
  int id1, id2, id3, id4;
  char *p1, *p2, *p3, *p4;
  char path1[255] = "data/";
  char path2[255] = "data/";
  char path3[255] = "data/";
  char path4[255] = "data/";
  id1 = texStrToID(tex1);
  strcat_s(path1, tex1);
  p1 = path1;
  if(numTexes > 1) {
    id2 = texStrToID(tex2);
    strcat_s(path2, tex2);
    p2 = path2;
  }
  else {
    p2 = NULL;
  }
  if(numTexes > 2) {
    id3 = texStrToID(tex3);
    strcat_s(path3, tex3);
    p3 = path3;
  }
  else {
    p3 = NULL;
  }
  if(numTexes > 3) {
    id4 = texStrToID(tex4);
    strcat_s(path4, tex4);
    p4 = path4;
  }
  else {
    p4 = NULL;
  }
  renderer->loadTextureArray512(id1, p1, id2, p2, id3, p3, id4, p4);
}
char* LevelData::processQuadTripple(char* str, v3* v) {
  char buffer[255];
  char* readPtr = str;
  char* writePtr = buffer;
  while(*readPtr != ','){
    *writePtr++ = *readPtr++;
  }
  readPtr++;
  *writePtr = '\0';
  writePtr = buffer;
  v->x = (float) atof(buffer);
  while(*readPtr != ','){
    *writePtr++ = *readPtr++;
  }
  readPtr++;
  *writePtr = '\0';
  writePtr = buffer;
  v->y = (float) atof(buffer);
  while(*readPtr != ')'){
    *writePtr++ = *readPtr++;
  }
  readPtr++;
  *writePtr = '\0';
  v->z = (float) atof(buffer);
  return readPtr;
}

char* LevelData::processUV2(char* str, v2* v) {
  char buffer[255];
  char* readPtr = str;
  char* writePtr = buffer;
  while(*readPtr != ','){
    *writePtr++ = *readPtr++;
  }
  readPtr++;
  *writePtr = '\0';
  writePtr = buffer;
  v->x = (float) atof(buffer);
  while(*readPtr != ')'){
    *writePtr++ = *readPtr++;
  }
  *writePtr = '\0';
  v->y = (float) atof(buffer);
  return readPtr;
}

char* LevelData::processQuadTextLine(char* str, TexturedQuad* q) {
  char buffer[255];
  char* readPtr = str;
  char* writePtr = buffer;
  
  readPtr = advancePointerToOnePastChar(readPtr, '(');
  readPtr = processQuadTripple(readPtr, &q->b);
  readPtr = advancePointerToOnePastChar(readPtr, '(');
  readPtr = processQuadTripple(readPtr, &q->d);
  readPtr = advancePointerToOnePastChar(readPtr, '(');
  readPtr = processQuadTripple(readPtr, &q->c);
  readPtr = advancePointerToOnePastChar(readPtr, '(');
  readPtr = processQuadTripple(readPtr, &q->a);
  readPtr = advancePointerToOnePastChar(readPtr, '(');
  readPtr = processUV2(readPtr, &q->texScale);
  return readPtr;
}

void LevelData::loadLevelFromTextFile(char* path) {
  //First important line of file must start with "**"
  char texes[4][255];
  int texNum = -1;
  char* writePtr;
  FileReadResult r = readWholeFile(path);
  char* fileText = (char*) r.contents;
  char* readPtr = fileText;
  char* eof = readPtr + r.contentsSize;
  std::vector<TexturedQuad>* qs = new std::vector<TexturedQuad>;
  while(readPtr != eof) {
    if(*readPtr=='*' && *(readPtr + 1)=='*'){
      texNum++;
      if(texNum > 3) {
        //load textures
      }
      readPtr+=3;
      writePtr = &texes[texNum][0];
      while(*readPtr != '\n' && *readPtr != '\r') {
        *writePtr++ = *readPtr++;
      }
      *writePtr = '\0';
    }
    else if(*readPtr == '(') {
      TexturedQuad q;
       readPtr = processQuadTextLine(readPtr, &q);
       q.texHandle.texLayer = texNum;
       qs->push_back(q);
    }
    readPtr++;
  }
  //load textures
  preLoadTextureArray512(texNum + 1, texes[0], texes[1], texes[2], texes[3]);
  for(size_t i = 0; i < qs->size(); i++) {
    TexturedQuad q = (*qs)[i];
    addTexturedQuad(q);
  }
}

int isColliding(OBB& a, OBB& b) {
  float ra, rb;
  float EPSILON = .001f;
  m3x3 R, AbsR;
  // Compute rotation matrix expressing b in a’s coordinate frame
  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
      R.n[i][j] = dot(a.u[i], b.u[j]);

  // Compute translation vector t
  v3 t = b.c - a.c;
  // Bring translation into a’s coordinate frame
  t = V3(dot(t, a.u[0]), dot(t, a.u[2]), dot(t, a.u[2]));
  // Compute common subexpressions. Add in an epsilon term to
  // counteract arithmetic errors when two edges are parallel and
  // their cross product is (near) null (see text for details)
  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
      AbsR.n[i][j] = abs(R.n[i][j]) + EPSILON;
  // Test axes L = A0, L = A1, L = A2
  for (int i = 0; i < 3; i++) {
    ra = a.width.n[i];
    rb = b.width.n[0] * AbsR.n[i][0] + b.width.n[1] * AbsR.n[i][1] + b.width.n[2] * AbsR.n[i][2];
    if (abs(t.n[i]) > ra + rb) return 0;
  }
  // Test axes L = B0, L = B1, L = B2
  for (int i = 0; i < 3; i++) {
    ra = a.width.n[0] * AbsR.n[0][i] + a.width.n[1] * AbsR.n[1][i] + a.width.n[2] * AbsR.n[2][i];
    rb = b.width.n[i];
    if (abs(t.n[0] * R.n[0][i] + t.n[1] * R.n[1][i] + t.n[2] * R.n[2][i]) > ra + rb) return 0;
  }
  // Test axis L = A0 x B0
  ra = a.width.n[1] * AbsR.n[2][0] + a.width.n[2] * AbsR.n[1][0];
  rb = b.width.n[1] * AbsR.n[0][2] + b.width.n[2] * AbsR.n[0][1];
  if (abs(t.n[2] * R.n[1][0] - t.n[1] * R.n[2][0]) > ra + rb) return 0;
  // Test axis L = A0 x B1
  ra = a.width.n[1] * AbsR.n[2][1] + a.width.n[2] * AbsR.n[1][1];
  rb = b.width.n[0] * AbsR.n[0][2] + b.width.n[2] * AbsR.n[0][0];
  if (abs(t.n[2] * R.n[1][1] - t.n[1] * R.n[2][1]) > ra + rb) return 0;
  // Test axis L = A0 x B2
  ra = a.width.n[1] * AbsR.n[2][2] + a.width.n[2] * AbsR.n[1][2];
  rb = b.width.n[0] * AbsR.n[0][1] + b.width.n[1] * AbsR.n[0][0];
  if (abs(t.n[2] * R.n[1][2] - t.n[1] * R.n[2][2]) > ra + rb) return 0;
  // Test axis L = A1 x B0
  ra = a.width.n[0] * AbsR.n[2][0] + a.width.n[2] * AbsR.n[0][0];
  rb = b.width.n[1] * AbsR.n[1][2] + b.width.n[2] * AbsR.n[1][1];
  if (abs(t.n[0] * R.n[2][0] - t.n[2] * R.n[0][0]) > ra + rb) return 0;
  // Test axis L = A1 x B1
  ra = a.width.n[0] * AbsR.n[2][1] + a.width.n[2] * AbsR.n[0][1];
  rb = b.width.n[0] * AbsR.n[1][2] + b.width.n[2] * AbsR.n[1][0];
  if (abs(t.n[0] * R.n[2][1] - t.n[2] * R.n[0][1]) > ra + rb) return 0;
  // Test axis L = A1 x B2
  ra = a.width.n[0] * AbsR.n[2][2] + a.width.n[2] * AbsR.n[0][2];
  rb = b.width.n[0] * AbsR.n[1][1] + b.width.n[1] * AbsR.n[1][0];
  if (abs(t.n[0] * R.n[2][2] - t.n[2] * R.n[0][2]) > ra + rb) return 0;
  // Test axis L = A2 x B0
  ra = a.width.n[0] * AbsR.n[1][0] + a.width.n[1] * AbsR.n[0][0];
  rb = b.width.n[1] * AbsR.n[2][2] + b.width.n[2] * AbsR.n[2][1];
  if (abs(t.n[1] * R.n[0][0] - t.n[0] * R.n[1][0]) > ra + rb) return 0;
  // Test axis L = A2 x B1
  ra = a.width.n[0] * AbsR.n[1][1] + a.width.n[1] * AbsR.n[0][1];
  rb = b.width.n[0] * AbsR.n[2][2] + b.width.n[2] * AbsR.n[2][0];
  if (abs(t.n[1] * R.n[0][1] - t.n[0] * R.n[1][1]) > ra + rb) return 0;
  // Test axis L = A2 x B2
  ra = a.width.n[0] * AbsR.n[1][2] + a.width.n[1] * AbsR.n[0][2];
  rb = b.width.n[0] * AbsR.n[2][1] + b.width.n[1] * AbsR.n[2][0];
  if (abs(t.n[1] * R.n[0][2] - t.n[0] * R.n[1][2]) > ra + rb) return 0;
  // Since no separating axis is found, the OBBs must be intersecting
  return 1;
}

int isColliding(AABB& a, AABB& b) {
  if (abs(a.center.x - b.center.x) > (a.rad.x + b.rad.x)) return 0;
  if (abs(a.center.y - b.center.y) > (a.rad.y + b.rad.y)) return 0;
  if (abs(a.center.z - b.center.z) > (a.rad.z + b.rad.z)) return 0;
  return 1;
}

void LevelGeometry::movePlayer(Player& player, KeyboardState& keyboardState, float pitchDif, float yRotationDif, float lastFrameSec) {
  player.pitch += pitchDif;
    player.yRotation += yRotationDif;
    if(player.pitch > 89){ 
      player.pitch = 89;
    }
    else if(player.pitch < -89){
      player.pitch = -89;
    }
    float pitch = degToRad(player.pitch);
    float yaw = normalizeDeg(degToRad(player.yRotation));
    player.viewDir.x = cos(yaw) * cos(pitch);
    player.viewDir.y = sin(pitch);
    player.viewDir.z = sin(yaw) * cos(pitch);
    player.viewDir = normalize(player.viewDir);
    v3 side = cross(player.viewDir, V3(0.0, 1.0, 0.0));
    v3 upDir = cross(normalize(side), player.viewDir);
    v3 movementDir = V3(0.0, 0.0, 0.0);
    if (keyboardState.moveForward) {
      movementDir.x = movementDir.x + player.viewDir.x;
      movementDir.z = movementDir.z + player.viewDir.z;
    }
    if (keyboardState.moveBackward) {
      movementDir.x = movementDir.x - player.viewDir.x;
      movementDir.z = movementDir.z - player.viewDir.z;
    }
    if (keyboardState.strafeRight) {
      movementDir = movementDir + side;
    }
    if (keyboardState.strafeLeft) {
      movementDir = movementDir - side;
    }
    if (keyboardState.jump && player.onGround) {
      player.vel.y = sqrt(-.6f*gravity.y); // vf*vf = -g*.6
      player.onGround = false;
    }
    if (keyboardState.duck) {
      //movementDir = movementDir;
    }

    player.acc.y +=  lastFrameSec*gravity.y;
    player.vel = player.vel + lastFrameSec*player.acc;
    player.center = lastFrameSec*player.vel + player.center;
    //player.viewDir = viewDir;
    
    //create player collision geometry
    static AABB playerAABB;
    static OBB playerOBB;
    movementDir = normalize(movementDir);
    playerAABB.center.x = player.center.x + (player.walkingSpeed * lastFrameSec * movementDir.x);
    playerAABB.center.y = player.center.y + (player.walkingSpeed * lastFrameSec * movementDir.y);
    playerAABB.center.z = player.center.z + (player.walkingSpeed * lastFrameSec * movementDir.z);
    playerAABB.rad = V3(.125,.125,.125);
    playerOBB.c = playerAABB.center;
    playerOBB.u[0] = player.viewDir;
    playerOBB.u[1] = V3(0.0f,1.0f,0.0f);
    playerOBB.u[2] = side;
    playerOBB.width = V3(.125,.125,.125);
    bool noCols = true;

    for(size_t i = 0; i < AABBs.size(); i++) {
      if(isColliding(playerAABB, AABBs[i])) {
        noCols = false;
        break;
      }
    }
    if(noCols){
      for (size_t i = 0; i < OBBs.size(); i++) {
        if(isColliding(playerOBB, OBBs[i])) {
          noCols = false;
          break;
        }
      }
    }
    if(noCols) {
      player.center = playerAABB.center;
    }
    if(player.center.y < .5) {
      player.center.y = .5;
      player.acc.y = 0.0;
      player.vel.y = 0.0;
      player.onGround = true;
    }
}
