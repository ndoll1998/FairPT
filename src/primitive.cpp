#include "./primitive.hpp"
#include "./ray.hpp"
#include <math.h>

// helper functions for packet vectors
void cross(Vec4f result[3], const Vec4f a[3], const Vec4f b[3]) {
    result[0] = (a[1] * b[2]) - (a[2] * b[1]);
    result[1] = (a[2] * b[0]) - (a[0] * b[2]);
    result[2] = (a[0] * b[1]) - (a[1] * b[0]);
}

Vec4f dot(const Vec4f a[3], const Vec4f b[3]) {
    return a[0].fmadd(b[0], a[1].fmadd(b[1], a[2] * b[2]));
}

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
    A(A),
    U(B - A),
    V(C - A),
    N(U.cross(V).normalize())
{}

bool Triangle::cast(
    const Ray& r,
    HitRecord& record
) const {
    // Müller-Trumbore intersection algorithm
    // check if ray is parallel to triangle 
    Vec3f h = r.direction.cross(V);
    Vec3f a = U.dot(h);
    if ((a[0] > -1e-3) && (a[0] < 1e-3)) { return false; }
    // check if intersection is in 
    // range of the first edge 
    Vec3f f = Vec3f::ones/a;
    Vec3f s = r.origin - A;
    Vec3f u = f * s.dot(h);
    // ray misses
    if ((u[0] < 0.0f) || (u[0] > 1.0f)) { return false; }
    // check if intersection is
    // in range of both edges
    Vec3f q = s.cross(U);
    Vec3f v = f * r.direction.dot(q);
    // ray misses
    if ((v[0] < 0.0f) || (u[0] + v[0] > 1.0f)) { return false; }
    // at this point we know that
    // the ray does intersect the
    // triangle so we can compute
    // the distance 
    Vec3f t = f * V.dot(q);
    // triangle is behind the ray
    if (t[0] < 1e-3) { return false; }
    // compute the intersection point
    Vec3f p = t.fmadd(r.direction, r.origin);
    record = { t[0], p, N, r.direction, true, mat };
    return true;
}

/*
 *  Triangle Packet Primitive
 */


TrianglePacket::TrianglePacket(
    const Triangle& T1,
    const Triangle& T2,
    const Triangle& T3,
    const Triangle& T4
) :
    mats{ T1.mat, T2.mat, T3.mat, T4.mat },
    A{
        Vec4f(T1.A[0], T2.A[0], T3.A[0], T4.A[0]),
        Vec4f(T1.A[1], T2.A[1], T3.A[1], T4.A[1]),
        Vec4f(T1.A[2], T2.A[2], T3.A[2], T4.A[2])
    },
    U{
        Vec4f(T1.U[0], T2.U[0], T3.U[0], T4.U[0]),
        Vec4f(T1.U[1], T2.U[1], T3.U[1], T4.U[1]),
        Vec4f(T1.U[2], T2.U[2], T3.U[2], T4.U[2])
    },
    V{
        Vec4f(T1.V[0], T2.V[0], T3.V[0], T4.V[0]),
        Vec4f(T1.V[1], T2.V[1], T3.V[1], T4.V[1]),
        Vec4f(T1.V[2], T2.V[2], T3.V[2], T4.V[2]),
    },
    N{ T1.N, T2.N, T3.N, T4.N }
{}


bool TrianglePacket::cast(
    const Ray& ray,
    HitRecord& record
) const {
    // Möller–Trumbore intersection algorithm
    // using simd instructions for parallel
    // processing of triangles rays at once
    // TODO: share these along all triangles
    Vec4f ray_orig[3] = { 
        Vec4f(ray.origin[0]), 
        Vec4f(ray.origin[1]),
        Vec4f(ray.origin[2])
    };
    Vec4f ray_dir[3] = {
        Vec4f(ray.direction[0]), 
        Vec4f(ray.direction[1]),
        Vec4f(ray.direction[2])
    };

    // check if the ray is parallel to triangle 
    Vec4f h[3]; cross(h, ray_dir, V); 
    Vec4f a = dot(U, h);
    Vec4f mask1 = (a < Vec4f::neps) | (Vec4f::eps < a);
    // check if intersection in
    // range of first edge
    Vec4f f = Vec4f::ones / a;
    Vec4f s[3] = {
        ray_orig[0] - A[0],
        ray_orig[1] - A[1],
        ray_orig[2] - A[2]
    };
    Vec4f u = dot(s, h) * f;
    Vec4f mask2 = (Vec4f::zeros < u) & (u < Vec4f::ones);
    // check if intersection is
    // in range of both edges
    Vec4f q[3]; cross(q, s, U);
    Vec4f v = dot(ray_dir, q) * f;
    Vec4f mask3 = (Vec4f::zeros < v) & ((u + v) < Vec4f::ones);
    // compute the distance between the origin
    // of the ray and the intersection point
    // and make sure it is in front of the ray
    Vec4f t = dot(V, q) * f;
    Vec4f mask4 = (Vec4f::eps < t);
    // find the closest triangle
    // intersecting with the ray
    size_t idx = -1;
    unsigned int mask = _mm_movemask_ps(mask1 & mask2 & mask3 & mask4);
    for (size_t i = 0; i < 4; i++) {
        // make sure that
        //  - the ray intersects with the current triangle
        //  - the intersection point is closer than the current best
        if ((mask & 1) && ((t[i] < record.t) || (!record.valid))) {
            // update the index and the
            // current best distance
            idx = i;
            record.t = t[i];             
            record.valid = true;
        }
        // go to the next triangle
        mask >>= 1;
    }
    // update the hit record with
    // the values of the closest triangle
    if (idx >= 0) {
        // compute the hitpoint and update all
        // values that were not set previously
        record.p = Vec3f(record.t).fmadd(ray.direction, ray.origin);
        record.n = N[idx];
        record.v = ray.direction;
        record.mat = mats[idx];
    }
    // indicate that the ray intersected
    // with any of the triangles
    return record.valid;
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
    const Ray& ray,
    HitRecord& record
) const {
    // temporary hitrecord to compare
    // with the current best
    HitRecord tmp;
    // check all primitives in list
    for (const Primitive* p : *this) {
        // cast the ray packet to the
        // current primitive and update
        // the hitrecords
        if (p->cast(ray, tmp)) {
            // update the hit records stored
            // in the contribution info if neccessary
            if ((record.t > tmp.t) || (!record.valid)) {
                // keep the valid hitrecord with
                // smaller distance 
                record = tmp; 
            }
        }
    }
    // indicate that the ray did
    // hit some primitive in the list
    return record.valid;
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
