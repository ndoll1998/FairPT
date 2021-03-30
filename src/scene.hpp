#ifndef H_SCENE
#define H_SCENE

// includes
#include <vector>
#include "./primitive.hpp"

class Scene {
private:
    // the bounding volume hierarchy
    // storing all primitives of the scene
    const BVH _bvh;
public:
    // constructors
    Scene(
        const std::vector<Triangle>& triangles  // vector of all triangles in the scene
    );
    // getter
    const BVH& bvh(void) const;
};

#endif // H_SCENE
