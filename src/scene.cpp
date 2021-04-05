#include "./scene.hpp"
#include "./primitive.hpp"

void Scene::init(const BoundableList& objects)
{
    // build the bounding volume hierarchy
    _bvh = new BVH(objects, 16, 8);
    // build the primitives of each leaf node
    // of the bounding volume hierarchy
    for (size_t i = 0; i < _bvh->num_leafs(); i++) {
        // get the list of boundable objects that
        // is assigned to the current leaf
        const BoundableList& objs = _bvh->get_leaf_objects(i);
        // create collections for the different
        // primitive types
        TriangleCollection* tris = new TriangleCollection;
        // sort all objects into their respective
        // primitive collection
        for (const Boundable* obj : objs) {
            // triangle
            if (const Triangle* t = dynamic_cast<const Triangle*>(obj)) {
                tris->push_back(*t);
            }
        }
        // push the primitive
        _primitives.push_back(tris); 
    }
}

Scene::Scene(const BoundableList& objects)
{
    // initialize scene from the given
    // list of objects
    init(objects);
}

Scene::Scene(const Mesh& mesh)
{
    // convert the mesh to a list of boundables
    BoundableList objs;
    objs.insert(objs.begin(), mesh.begin(), mesh.end());
    // initialize the scene from the objects
    init(objs);
}

Scene::~Scene(void)
{
    // delete primitives
    for (Primitive* p : _primitives) { delete p; }
}

// getter functions
const BVH& Scene::bvh(void) const { return *_bvh; }
const PrimitiveList& Scene::primitives(void) const { return _primitives; }
