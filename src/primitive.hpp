#ifndef H_PRIMITIVE
#define H_PRIMITIVE

// forward declarations
struct Ray;
// includes
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


/*
 *  Primitives
 */

// abstract primitive defining the
// interface that a primitive must
// follow
class Primitive {
public:
    // virtual function to cast a ray to
    // the primitive and receive a hitrecord
    virtual bool cast(
        const Ray& r, 
        HitRecord& h
    ) const = 0;
    // build an axis-aligned bounding box
    // complete surrounding the primitive
    // virtual AABB build_aabb(void) const = 0;
};

// triangle primitive
class Triangle : public Primitive {
private:
    // pointer to the material
    const mtl::Material* mat;
    // the three corner points
    // defining the triangle
    // in counter-clockwise order
    Vec3f A, B, C;
    // the normal of the triangle
    Vec3f N;
public:
    // constructor
    Triangle(
        const Vec3f& A, // corner points in
        const Vec3f& B, // counter-clockwise
        const Vec3f& C, // order
        const mtl::Material* mat
    );
    // override cast function
    virtual bool cast(
        const Ray& r,
        HitRecord& h
    ) const;
};


/*
 *  Collection of Primitives
 */

// primitive list
class PrimitiveList :
    public Primitive,
    public std::vector<const Primitive*>
{
public:
    // constructors
    PrimitiveList(void) = default;
    PrimitiveList(
        const PrimitiveList::const_iterator& begin,
        const PrimitiveList::const_iterator& end
    );
    // cast a ray to each primitive
    // of the primtive list iteratively
    virtual bool cast(
        const Ray& r,
        HitRecord& h
    ) const;
};

// bounding volume hiararchy
class BVH {
private:
    const PrimitiveList prims;
public:
    // constructors
    BVH(const PrimitiveList& pl);
    // get all leaf ids that intersect the given ray
    void get_intersecting_leafs(
        const Ray& r,                   // ray to test intersection with
        std::vector<size_t>& leaf_ids   // output vector of leaf ids
    ) const;    
    // return the list of primitives stored 
    // in a specific leaf of the tree
    const PrimitiveList& leaf_primitives(const size_t& i) const;
    // get the number of leaf nodes
    // in the bvh tree
    size_t n_leafs(void) const;
};

#endif // H_PRIMITIVE
