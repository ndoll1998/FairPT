#include "./primitive.hpp"
#include "./ray.hpp"
#include "./mesh.hpp"
#include <algorithm>
#include <numeric>
#include <queue>
#include <math.h>

// helper functions for packet vectors
inline void cross(
    std::array<Vec4f, 3>& result, 
    const std::array<Vec4f, 3>& a, 
    const std::array<Vec4f, 3>& b
) {
    result[0] = (a[1] * b[2]) - (a[2] * b[1]);
    result[1] = (a[2] * b[0]) - (a[0] * b[2]);
    result[2] = (a[0] * b[1]) - (a[1] * b[0]);
}

inline Vec4f dot(
    const std::array<Vec4f, 3>& a, 
    const std::array<Vec4f, 3>& b
) {
    return a[0].fmadd(b[0], a[1].fmadd(b[1], a[2] * b[2]));
}

inline void sub(
    std::array<Vec4f, 3>& result,
    const std::array<Vec4f, 3>& a,
    const std::array<Vec4f, 3>& b
) {
    result[0] = a[0] - b[0];
    result[1] = a[1] - b[1];
    result[2] = a[2] - b[2];
}


/*
 *  Triangle
 */

Triangle::Triangle(
    const Vec3f& A,
    const Vec3f& B,
    const Vec3f& C,
    const mtl::Material* mtl
) : A(A), B(B), C(C), mtl(mtl)
{
}

AABB Triangle::bound(void) const
{
    // build a bounding box containing all
    // three corner points of the triangles
    return AABB(
        A.min(B.min(C)),
        A.max(B.max(C))
    );
}


/*
 * Triangle Collection
 */

TriangleCollection::TriangleCollection(
    const std::vector<Triangle>::const_iterator& begin,
    const std::vector<Triangle>::const_iterator& end
) {
    // push each triangle in the iterator
    // onto the collection
    std::for_each(begin, end, [this](const Triangle& t) { push_back(t); });
}


bool TriangleCollection::cast_ray_triangle(
    const Ray4& ray,
    const std::array<Vec4f, 3>& A,
    const std::array<Vec4f, 3>& U,
    const std::array<Vec4f, 3>& V,
    float& t,   // distance to intersection point
    size_t& j   // index of triangle with closest hit
) {
    // Möller–Trumbore intersection algorithm
    // using simd instructions for parallel
    // processing of triangles rays at once
    
    // check if the ray is parallel to triangle 
    std::array<Vec4f, 3> h; cross(h, ray.direction, V); 
    Vec4f a = dot(U, h);
    Vec4f mask1 = (a < Vec4f::neps) | (Vec4f::eps < a);
    // check if intersection in
    // range of first edge
    Vec4f f = Vec4f::ones / a;
    std::array<Vec4f, 3> s; sub(s, ray.origin, A);
    Vec4f u = dot(s, h) * f;
    Vec4f mask2 = (Vec4f::zeros < u) & (u < Vec4f::ones);
    // check if intersection is
    // in range of both edges
    std::array<Vec4f, 3> q; cross(q, s, U);
    Vec4f v = dot(ray.direction, q) * f;
    Vec4f mask3 = (Vec4f::zeros < v) & ((u + v) < Vec4f::ones);
    // compute the distance between the origin
    // of the ray and the intersection point
    // and make sure it is in front of the ray
    Vec4f ts = dot(V, q) * f;
    Vec4f mask4 = (Vec4f::eps < ts);
    // find the closest triangle
    // intersecting with the ray
    bool hit = false;
    unsigned int mask = _mm_movemask_ps(mask1 & mask2 & mask3 & mask4);
    for (size_t i = 0; i < 4; i++) {
        // update the current best if both
        //  - the ray intersects with the current triangle
        //  - the intersection point is closer than the current best
        if ((mask & 1u) && ((ts[i] < t) || (!hit))) { j = i; t = ts[i]; hit = true; }
        // go to the next triangle
        mask >>= 1;
    }
    // indicate that the ray intersected with at
    // least one of the 
    return hit;
}

bool TriangleCollection::cast(
    const Ray& ray,
    HitRecord& record
) const {
    // build ray-packet from ray
    Ray4 ray_packet = {
        { Vec4f(ray.origin[0]), Vec4f(ray.origin[1]), Vec4f(ray.origin[2]) },
        { Vec4f(ray.direction[0]), Vec4f(ray.direction[1]), Vec4f(ray.direction[2]) }
    }; 
    // values to mark the current best hit
    float t;    // distance to the current closest intersection
    size_t i;   // index of the triangle with the current best hit
    bool hit = false;   // did the ray hit a triangle yet
    // check all triangle packets in list
    for (size_t k = 0; k < A.size(); k++) {
        // gather all information needed to cast the
        // ray against the current triangle packet
        const std::array<Vec4f, 3>& a = A[k];
        const std::array<Vec4f, 3>& u = U[k];
        const std::array<Vec4f, 3>& v = V[k];
        // cast the ray against the traingle packet
        float tmp_t = t; size_t tmp_j;
        if (TriangleCollection::cast_ray_triangle(ray_packet, a, u, v, tmp_t, tmp_j)) {
            // update the current best if neccessary
            if ((t > tmp_t) || (!hit)) {
                // keep the smaller distance and update the
                // index to point to the triangle and packet
                t = tmp_t;
                i = tmp_j + k * 4;
                hit = true;
            }
        }
    }
    // check if the ray did hit any of the triangles
    // in the collection
    if (hit) {
        // compute the point of intersection
        Vec3f p = Vec3f(t).fmadd(ray.direction, ray.origin);
        // update the hitrecord accordingly
        record = { t, p, N[i], ray.direction, true, mtls[i] };
    }
    // indicate that the ray indeed hit
    // a triangle of the collection
    return hit;
}

void TriangleCollection::push_back(const Triangle& T) 
{
    // compute the spanning vectors
    Vec3f u = T.B - T.A;
    Vec3f v = T.C - T.A;
    // check if a new triangle packet is
    // needed for the given triangle
    size_t i = n_triangles++ % 4;
    if (i == 0) {
        // add a new packet filled with
        // the same triangle
        A.push_back({ Vec4f(T.A[0]), Vec4f(T.A[1]), Vec4f(T.A[2]) });
        U.push_back({ Vec4f(u[0]), Vec4f(u[1]), Vec4f(u[2]) });
        V.push_back({ Vec4f(v[0]), Vec4f(v[1]), Vec4f(v[2]) });
    } else {
        // insert the triangle into the
        // currently last packet
        A.back()[0][i] = T.A[0];
        A.back()[1][i] = T.A[1];
        A.back()[2][i] = T.A[2];
        U.back()[0][i] = u[0];
        U.back()[1][i] = u[1];
        U.back()[2][i] = u[2];
        V.back()[0][i] = v[0];
        V.back()[1][i] = v[1];
        V.back()[2][i] = v[2];
    }
    // push normal and material which are not
    // separated by components
    N.push_back(u.cross(v).normalize());
    mtls.push_back(T.mtl);
}


