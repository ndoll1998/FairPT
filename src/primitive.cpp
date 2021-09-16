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
 *  Primitive Collection
 */

bool PrimitiveCollection::cast(
    const Ray& ray,
    HitRecord& record
) const {
    // build ray-packet from ray
    Ray4 ray_packet = {
        { Vec4f(ray.origin[0]), Vec4f(ray.origin[1]), Vec4f(ray.origin[2]) },
        { Vec4f(ray.direction[0]), Vec4f(ray.direction[1]), Vec4f(ray.direction[2]) }
    }; 
    // values to mark the current best hit
    size_t i;                   // index of the triangle with the current best hit
    float t = record.t;         // distance to the current closest intersection
    bool hit = record.is_valid; // did the ray hit a triangle yet
    // check all packets in list
    for (size_t k = 0; k < n_packets(); k++) {
        // cast the ray against the primitive packet
        Vec4f ts = cast_ray_packet(ray_packet, k);
        // find the closest primitive in packet
        // intersecting with the ray
        for (size_t j = 0; j < 4; j++) {
            // update the current best if both
            //  - the ray intersects with the current primitive
            //  - the intersection point is closer than the current best
            if ((ts[j] > 0) && ((ts[j] < t) || (!hit))) { 
                t = ts[j];
                i = k * 4 + j;
                hit = true; 
            }
        }
    }
    // check if the ray did hit any of the primitives
    // in the collection
    if (hit) {
        // compute the point of intersection
        Vec3f p = Vec3f(t).fmadd(ray.direction, ray.origin);
        // update the hitrecord accordingly
        record = { t, p, get_normal(i, p), ray.direction, true, get_material(i) };
    }
    // indicate that the ray indeed hit
    // a primitive in the collection
    return hit;
}

/*
 *  Primitive List
 */

bool PrimitiveList::cast(
    const Ray& ray,
    HitRecord& record
) const {
    // values to mark the current best hit
    bool hit = false;
    HitRecord tmp;
    // check all primitives in the list
    for (const Primitive* prim : *this) {
        // cast ray against the primitive
        // and update the hitrecord if neccessary
        if (prim->cast(ray, tmp) && (
                (tmp.is_valid && !record.is_valid) ||
                (tmp.is_valid && record.is_valid && (record.t > tmp.t))
        )) { record = tmp; hit = true; }
        // reset temporary hitrecord
        tmp.is_valid = false;
    }
    return hit;
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

Vec3f TriangleCollection::get_normal(
    const size_t& i,    // index of the primitive
    const Vec3f& p      // point on surface
) const {
    return Ns[i];
}

const mtl::Material* TriangleCollection::get_material(
    const size_t& i
) const {
    return mtls[i];
}

Vec4f TriangleCollection::cast_ray_packet(
    const Ray4& ray,
    const size_t& i
) const {
    // Möller–Trumbore intersection algorithm
    // using simd instructions for parallel
    // processing of triangles rays at once
    
    // gather all information needed to cast the
    // ray against the current triangle packet
    const std::array<Vec4f, 3>& A = As[i];
    const std::array<Vec4f, 3>& U = Us[i];
    const std::array<Vec4f, 3>& V = Vs[i];
    
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
    // mark invalids
    ts = ts.take(-1 * Vec4f::ones, -1 * (mask1 & mask2 & mask3 & mask4));
    return ts;
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
        As.push_back({ Vec4f(T.A[0]), Vec4f(T.A[1]), Vec4f(T.A[2]) });
        Us.push_back({ Vec4f(u[0]), Vec4f(u[1]), Vec4f(u[2]) });
        Vs.push_back({ Vec4f(v[0]), Vec4f(v[1]), Vec4f(v[2]) });
    } else {
        // insert the triangle into the
        // currently last packet
        As.back()[0][i] = T.A[0];
        As.back()[1][i] = T.A[1];
        As.back()[2][i] = T.A[2];
        Us.back()[0][i] = u[0];
        Us.back()[1][i] = u[1];
        Us.back()[2][i] = u[2];
        Vs.back()[0][i] = v[0];
        Vs.back()[1][i] = v[1];
        Vs.back()[2][i] = v[2];
    }
    // push normal and material which are not
    // separated by components
    Ns.push_back(u.cross(v).normalize());
    mtls.push_back(T.mtl);
}

size_t TriangleCollection::n_packets(void) const { return As.size(); }
size_t TriangleCollection::n_primitives(void) const { return n_triangles; }

/*
 * Sphere
 */

Sphere::Sphere(
    const Vec3f& center,
    const float& radius,
    const mtl::Material* mtl
): center(center), radius(radius), mtl(mtl)
{
}

AABB Sphere::bound(void) const {
    return AABB(
        center - radius,
        center + radius
    );
}

/*
 * Sphere Collection
 */

Vec4f SphereCollection::cast_ray_packet(
    const Ray4& ray,
    const size_t& i
) const {
    // gather all properties of the primitives
    // in the packet indicated by the given index
    const std::array<Vec4f, 3>& C = centers[i];
    const Vec4f& R = radii[i];

    // compute the distriminant
    std::array<Vec4f, 3> oc; sub(oc, ray.origin, C);
    Vec4f a = dot(ray.direction, ray.direction);
    Vec4f b = dot(oc, ray.direction);
    Vec4f c = dot(oc, oc) - (R * R);
    Vec4f d = (b * b) - (a * c);
    // compute distances
    Vec4f d_sqrt = d.sqrt();
    Vec4f ts = (d_sqrt - b).max(-1*d_sqrt - b) / a;
    // mark invalids
    ts = ts.take(-1 * Vec4f::ones, d < Vec4f::zeros);
    // return distances
    return ts;
}

Vec3f SphereCollection::get_normal(
    const size_t& i,    // index of the primitive
    const Vec3f& p      // point on surface
) const {
    size_t j = i / 4, k = i % 4;
    Vec3f center(centers[j][0][k], centers[j][1][k], centers[j][2][k]);
    return (p - center) / radii[j][k];
}

const mtl::Material* SphereCollection::get_material(
    const size_t& i
) const {
    return mtls[i];
}

void SphereCollection::push_back(const Sphere& S) {
    // check if a new sphere packet is needed for
    // for the given sphere
    size_t i = n_spheres++ % 4;
    if (i == 0) {
        // add new packet filled with the given sphere
        centers.push_back({ Vec4f(S.center[0]), Vec4f(S.center[1]), Vec4f(S.center[2]) });
        radii.push_back(Vec4f(S.radius));
    } else {
        // insert sphere into existing packet
        centers.back()[0][i] = S.center[0];
        centers.back()[1][i] = S.center[1];
        centers.back()[2][i] = S.center[2];
        radii.back()[i] = S.radius;
    } 
    // push matrial to list
    mtls.push_back(S.mtl);
}

size_t SphereCollection::n_packets(void) const { return centers.size(); }
size_t SphereCollection::n_primitives(void) const { return n_spheres; }
