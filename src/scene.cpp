#include "./scene.hpp"
#include "./primitive.hpp"

Scene::Scene(const BVH& bvh) : _bvh(bvh) {}

const BVH& Scene::bvh(void) const { return _bvh; }
