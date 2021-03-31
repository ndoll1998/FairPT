#include "./scene.hpp"
#include "./primitive.hpp"

Scene::Scene(
    const std::vector<Triangle*>& triangles
) : _bvh(triangles, 16, 8) {}

const BVH& Scene::bvh(void) const { return _bvh; }
