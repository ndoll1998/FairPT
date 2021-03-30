#ifndef H_PRIMITIVE
#define H_PRIMITIVE

// forward declarations
struct Ray;
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
typedef struct AABB {
    Vec3f min;
    Vec3f max;
} AABB;


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

// forward declaration
class TriangleCollection;
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
    // allow triangle collection to
    // access private members of a triangle
    friend TriangleCollection;
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
    TriangleCollection tri_collection;
public:
    // constructor
    BVH(const std::vector<Triangle>& triangles);
    // get all leaf ids that
    // intersect the given ray
    void get_intersecting_leafs(
        const Ray& r,                   // ray to test intersection with
        std::vector<size_t>& leaf_ids   // output vector of leaf ids
    ) const;
    // return the primitive stored 
    // in a specific leaf of the tree
    const Primitive* leaf_primitive(const size_t& i) const;
    // get the number of leaf nodes
    // in the bvh tree
    size_t n_leafs(void) const;
};

#endif // H_PRIMITIVE
