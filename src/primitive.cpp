#include "./primitive.hpp"
#include "./ray.hpp"

/*
 *  Triangle Primitive
 */

Triangle::Triangle(
    const Vec3f& A,
    const Vec3f& B,
    const Vec3f& C,
    const mtl::Material* mat
) : 
    mat(mat),
    A(A), B(B), C(C),
    N((B - A).cross(C - A).normalize())
{}

bool Triangle::cast(
    const Ray& r,
    HitRecord& record
) const {
    // Müller-Trumbore intersection algorithm
    // build vectors spanning the triangle
    Vec3f e1 = B - A;
    Vec3f e2 = C - A;
    // check if ray is parallel to triangle 
    Vec3f h = r.direction.cross(e2);
    float a = e1.dot(h)[0];
    if ((a > -1e-3) && (a < 1e-3)) { return false; }
    // check if intersection is in 
    // range of the first edge 
    float f = 1.0f/a;
    Vec3f s = r.origin - A;
    float u = f * s.dot(h)[0];
    // ray misses
    if ((u < 0.0f) || (u > 1.0f)) { return false; }
    // check if intersection is
    // in range of both edges
    Vec3f q = s.cross(e1);
    float v = f * r.direction.dot(q)[0];
    // ray misses
    if ((v < 0.0f) || (u + v > 1.0f)) { return false; }
    // at this point we know that
    // the ray does intersect the
    // triangle so we can compute
    // the distance 
    float t = f * e2.dot(q)[0];
    // triangle is behind the ray
    if (t < 1e-3) { return false; }
    // compute the intersection point
    // TODO: this can be done using a single simd instruction
    Vec3f p = r.origin + t * r.direction;
    record = { t, p, N, r.direction, true, mat };
    return true;
}

/*
 *  Primitive List
 */

PrimitiveList::PrimitiveList(
    const PrimitiveList::const_iterator& begin,
    const PrimitiveList::const_iterator& end
) :
    std::vector<const Primitive*>(begin, end)
{}

bool PrimitiveList::cast(
    const Ray& r, 
    HitRecord& h
) const {
    // temporary hitrecord to compare
    // with the current best
    HitRecord tmp = { -1, Vec3f(), Vec3f(), r.direction, false, nullptr };
    // check all primitives in list
    for (const Primitive* p : *this)
        if (p->cast(r, tmp))
            if ((tmp.t < h.t) || (!h.valid)) {
                // update record and note that 
                // the ray hit something
                h = tmp;
            }
    // did we hit anything
    return h.valid;
}

/*
 *  Bounding Volume Hierarchy
 */

BVH::BVH(const PrimitiveList& pl) : prims(pl) {}

void BVH::get_intersecting_leafs(
    const Ray& ray,
    std::vector<size_t>& leaf_ids
) const {
    // TODO: currently 0 for testing
    leaf_ids.push_back(0);
}

const PrimitiveList& BVH::leaf_primitives(
    const size_t& i
) const {
    return prims;
}

size_t BVH::n_leafs(void) const {
    // TODO: for testing
    return 1;
}
