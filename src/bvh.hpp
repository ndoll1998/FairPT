#ifndef H_BVH
#define H_BVH

// forward declarations
struct Ray;
struct Ray4;
class RayQueue;
class BVH;
class AABB4;
// includes
#include <array>
#include <vector>
#include "./vec.hpp"

/*
 *  Axis-Aligned Bounding Box
 */

// axis-aligned bounding box needed
// for bounding volume hierarchy
class AABB {
private:
    // minimum and maximum corner of
    // the bounding box
    Vec3f low = Vec3f::zeros;
    Vec3f high = Vec3f::zeros;
public:
    // constructor
    AABB(void) = default;
    AABB(
        const Vec3f& A,
        const Vec3f& B
    );
    // get the center of the bounding box
    Vec3f center(void) const;
    // cast a ray to the bounding box
    bool cast(const Ray& r) const;
    // allow access to private members
    friend BVH;
    friend AABB4;
};

// packet of four axis-aligned bounding boxes
class AABB4 {
private:
    std::array<Vec4f, 3> low;
    std::array<Vec4f, 3> high;
public:
    // constructors
    AABB4(void) = default;
    AABB4(
        const AABB& A,
        const AABB& B,
        const AABB& C,
        const AABB& D
    );
    // cast a ray to the bounding boxes
    // and return a bit-level mask
    unsigned int cast(const Ray4& r) const;
};


/*
 *  Bounding Volume Hierarchy
 */

// abstract class for objects that a
// bounding volume hierarchy can hold
class Boundable {
public:
    // abstract method building a
    // bounding box that completly
    // and tightly contains the object
    virtual AABB bound(void) const = 0;
};

// shortcut for list of boundables
using BoundableList = std::vector<Boundable*>;


class BVH {
private:
    // struct defining a node of
    // the bvh tree
    struct bvh_node {
        AABB4 aabb4;    // bounding boxes of the child nodes
        bool is_leaf;   // is the node a leaf node
        size_t leaf_id; // the id assigned to the leaf
    };
    // collection of vectors of boundables where
    // each vector collection corresponds to a
    // leaf of the bounding volume hierarchy
    std::vector<BoundableList> leaf_objs;
    // basic tree information
    size_t depth;
    size_t n_leaf_nodes;
    size_t n_inner_nodes;
    size_t n_total_nodes;
    // memory to store the nodes of the tree
    bvh_node* tree;
public:
    // constructor and destructor
    BVH(
        const BoundableList& objs,          // objects to sort in the bvh
        const size_t& max_depth,            // maximum depth of the bvh
        const size_t& min_size              // minimum number of primitives per leaf
    );
    ~BVH(void);
    // get the boundable list corresponding
    // the the leaf node with given id
    const BoundableList& get_leaf_objects(const size_t& leaf_id) const;
    // sort rays into buckets where each
    // bucket corresponds to one leaf
    void sort_rays_by_leafs(
        const RayQueue& rays,
        std::vector<RayQueue>& sorted
    ) const;
    // get the number of leaf nodes in
    // the bounding volume hierarchy
    const size_t& num_leafs(void) const;
};

#endif // H_BVH
