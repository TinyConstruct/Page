#include <vector>
#include <Windows.h>
#include <gl/gl.h>
#include "aro_math.h"
#include "texture.h"
#include "rendering.h"

#include "level.h"
void LevelGeometry::addAABB(v3 center, v3 rad) {
  AABB box;
  box.center = center;
  box.rad = rad;
  boundingBoxes.push_back(box);
}

void LevelGeometry::initialize() {
  boundingBoxes.reserve(100);
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

void LevelData::finalizeQuads() {
  for(int i = 0; i < quads->size(); i++){
    Vertpcnu a, b, c, d;
    a.position = (*quads)[i].a;
    a.uv = V3(0.0, 1.0, (*quads)[i].texHandle.texLayer);
    b.position = (*quads)[i].b;
    b.uv = V3(0.0, 0.0, (*quads)[i].texHandle.texLayer);
    c.position = (*quads)[i].c;
    c.uv = V3(1.0, 0.0, (*quads)[i].texHandle.texLayer);
    d.position = (*quads)[i].d;
    d.uv = V3(1.0, 1.0, (*quads)[i].texHandle.texLayer);
    renderer->addTri(a,b,c);
    renderer->addTri(c, d, a);
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
    //level->addAABB(center, rad);
  }
}

void LevelData::bakeTestLevel() {
  //add a bunch of geometry to a buffer: (quad,TEX_NUM)...
  addTexturedQuad(V3(-20.0, 0.0, -20.0), V3(-20.0, 0.0, 20.0), V3(20.0, 0.0, 20.0), V3(20.0, 0.0, -20.0),  FLOOR1);
  addTexturedQuad(V3(-20.0, 6.0, -20.0), V3(20.0, 6.0, -20.0), V3(20.0, 6.0, 20.0), V3(-20.0, 6.0, 20.0),  CEIL1);
  addTexturedQuad(V3(-1.5, 3.0, -5.0), V3(-1.5, 0.0, -5.0), V3(1.5, 0.0, -5.0), V3(1.5, 3.0, -5.0),  WALL2);
  //other buffer: (tri, TEX_NUM)
  //sort on getGLTexID(TEX_NUM) for both buffers
  //send quads/tris to renderer
  finalizeQuads();
  //send quads/tris to LevelGeometry for bounding boxes

}