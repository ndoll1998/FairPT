#ifndef H_PRIMITIVE
#define H_PRIMITIVE

// forward declarations
struct Ray;
struct Ray4;
class RayQueue;
class BVH;
class AABB4;
class Mesh;
class TriangleCollection;
// includes
#include <array>
#include <vector>
#include "./vec.hpp"
#include "./material.hpp"

// hit record storing infromation
// about the intersection of a ray
// with a primitive
typedef struct HitRecord {
    float t;    // distance to intersection point
    Vec3f p;    // point of intersection
    Vec3f n;    // surface normal at intersection
    Vec3f v;    // direction of incident ray
    bool valid = false;         // is the hit valid
    const mtl::Material* mat;   // surface material
} HitRecord;


// axis-aligned bounding box needed
// for bounding volume hierarchy
class AABB {
private:
    // minimum and maximum corner of
    // the bounding box
    Vec3f min = Vec3f::zeros;
    Vec3f max = Vec3f::zeros;
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
 *  Primitive
 */

// abstract class defining the interface
// a primitive must follow
class Primitive {
public:
    // cast a given ray to the primitive
    // and receive a hitrecord
    virtual bool cast(
        const Ray& ray,
        HitRecord& record
    ) const = 0;
};


/*
 *  Triangle
 */

// triangle class implementing a boundable
// object but not a primitive (rendering
// single triangles is not supported)
class Triangle {
private:
    // save corner points and material
    Vec3f A, B, C;
    const mtl::Material* mtl;
public:
    // constructor
    Triangle(
        const Vec3f& A, // corner points in
        const Vec3f& B, // counter-clockwise
        const Vec3f& C, // order
        const mtl::Material* mtl
    );
    // build bounding box completly
    // containing the triangle
    virtual AABB build_aabb(void) const;
    // allow triangle collection and mesh to
    // access private members of a triangle
    friend TriangleCollection;
    friend Mesh;
};

// combine a number of triangles into a
// single primitive for more efficient
// memory usage and computations
class TriangleCollection : public Primitive {
private:
    // the data of all the triangle packets
    // separated into the single components
    std::vector<std::array<Vec4f, 3>> A, U, V;
    // the normal vectors and materials
    // of all triangle packet
    std::vector<Vec3f> N;
    std::vector<const mtl::Material*> mtls;
    // the size of the collection
    size_t n_triangles = 0;
    // function to cast a ray to a single
    // packet of traingles
    static bool cast_ray_triangle(
        const Ray& ray,
        const std::array<Vec4f, 3>& A,
        const std::array<Vec4f, 3>& U,
        const std::array<Vec4f, 3>& V,
        float& t,   // distance to intersection point
        size_t& j   // index of triangle with closest hit
    );
public:
    // constructors
    TriangleCollection(void) = default;
    TriangleCollection(
        const std::vector<Triangle>::const_iterator& begin,
        const std::vector<Triangle>::const_iterator& end
    );
    virtual ~TriangleCollection(void) = default;
    // override cast function to process
    // multiple triangles at once using
    // simd instructions
    virtual bool cast(
        const Ray& ray,
        HitRecord& record
    ) const;
    // add a triangle to the collection
    // by pushing it into a packet
    void push_back(const Triangle& T);
};



/*
 *  Bounding Volume Hierarchy
 */

class BVH {
private:
    // struct defining a node of
    // the bvh tree
    struct bvh_node {
        AABB4 aabb4;    // bounding boxes of the child nodes
        bool is_leaf;   // is the node a leaf node
        size_t leaf_id; // the id assigned to the leaf
    };
    // list of triangle collections
    // associated with the leafs
    std::vector<TriangleCollection> leaf_tris_col;
    // basic tree information
    size_t depth;
    size_t n_leaf_nodes;
    size_t n_inner_nodes;
    size_t n_total_nodes;
    // memory to store the tree
    bvh_node* tree;
public:
    // constructor and destructor
    BVH(
        const std::vector<Triangle*>& tris, // triangles to store in the bvh
        const size_t& max_depth,            // maximum depth of the bvh
        const size_t& min_size              // minimum number of primitives per leaf
    );
    ~BVH(void);
    // get all leaf ids that
    // intersect the given ray
    void get_intersecting_leafs(
        const Ray& r,                   // ray to test intersection with
        std::vector<size_t>& leaf_ids   // output vector of leaf ids
    ) const;
    // vector over the primitives stored
    // in the leafs of the hierarchy
    const Primitive* get_primitive(const size_t& i) const;
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

#endif // H_PRIMITIVE
