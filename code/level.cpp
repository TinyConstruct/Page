#include <vector>
#include "aro_math.h"

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

void Level::initialize() {
  quads = new std::vector<Quad>(100);
  tris = new std::vector<Tri>(100);
}