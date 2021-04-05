#ifndef H_SCENE
#define H_SCENE

// includes
#include <vector>
#include "./bvh.hpp"
#include "./mesh.hpp"
#include "./primitive.hpp"

class Scene {
private:
    // the bounding volume hierarchy
    // organizing all boundables in the scene
    BVH* _bvh;
    // the actual primitives
    // note that each primitive corresponds
    // to exactly one leaf node of the bvh
    PrimitiveList _primitives;
    // private method to initialize scene a scene
    void init(const BoundableList& objects);    
public:
    // constructors / destructor
    Scene(const BoundableList& objects);
    Scene(const Mesh& mesh);
    ~Scene(void);
    // getters
    const BVH& bvh(void) const;
    const PrimitiveList& primitives(void) const;
};

#endif // H_SCENE
