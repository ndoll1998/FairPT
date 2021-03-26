#include "./primitive.hpp"
#include "./ray.hpp"
#include <math.h>

void HitRecord4::update(const HitRecord4& other) {
    // take the records that are valid in
    // other and invalid in here
    // valid_mask = (~valids) & other.valids
    Vec4f valid_mask = _mm_andnot_ps(valids, other.valids);
    // where both records are valid
    // keep the one with smaller distance value
    Vec4f both_valid_mask = other.valids & valids;
    Vec4f smaller_mask = (other.t < t) & both_valid_mask;
    // compute final mask
    Vec4f mask = valid_mask | smaller_mask;
    // update distance values
    t = t.take(other.t, mask);
    // update positional values
    px = px.take(other.px, mask);
    py = py.take(other.py, mask);
    pz = pz.take(other.pz, mask);
    // update normal values
    nx = nx.take(other.nx, mask);
    ny = ny.take(other.ny, mask);
    nz = nz.take(other.nz, mask);
    // update incident direction values
    vx = vx.take(other.vx, mask);
    vy = vy.take(other.vy, mask);
    vz = vz.take(other.vz, mask);
    // update valids
    valids = other.valids | valids;
    // update materials
    mats[0] = (mask[0])? other.mats[0] : mats[0];
    mats[1] = (mask[1])? other.mats[1] : mats[1];
    mats[2] = (mask[2])? other.mats[2] : mats[2];
    mats[3] = (mask[3])? other.mats[3] : mats[3];
}

void HitRecord4::split(HitRecord* records) const {
    // separate the packet of hitrecords
    // into single records
    for (size_t i = 0; i < 4; i++) {
        records[i] = {
            t[i],
            Vec3f(px[i], py[i], pz[i]),
            Vec3f(nx[i], ny[i], nz[i]),
            Vec3f(vx[i], vy[i], vz[i]),
            std::isnan(valids[i]),
            mats[i]
        };
    }
};

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
    Ax(A[0]), Ay(A[1]), Az(A[2])
{
    // compute edges of
    // the triangle
    Vec3f U = B - A;
    Vec4f V = C - A;
    // set the edge vectors
    Ux = Vec4f(U[0]); Uy = Vec4f(U[1]); Uz = Vec4f(U[2]);    
    Vx = Vec4f(V[0]); Vy = Vec4f(V[1]); Vz = Vec4f(V[2]);
    // compute the normal vector
    Vec3f N = U.cross(V).normalize();
    Nx = Vec4f(N[0]); Ny = Vec4f(N[1]); Nz = Vec4f(N[2]);
}

void Triangle::cast_packet(
    const Ray4& rays,
    HitRecord4& records,
    const size_t& n_valids
) const {
    // Möller–Trumbore intersection algorithm
    // using simd instructions for parallel
    // processing of multiple rays
    
    // compute cross products between ray 
    // directions and one edge of the triangle
    Vec4f hx = (rays.v * Vz) - (rays.w * Vy);
    Vec4f hy = (rays.w * Vx) - (rays.u * Vz);
    Vec4f hz = (rays.u * Vy) - (rays.v * Vx);
    // compute dot-product with the other edge 
    Vec4f a = (Ux * hx) + (Uy * hy) + (Uz * hz);
    // build the fist mask that checks if
    // the ray is parallel to the triangle
    Vec4f mask1 = (a < Vec4f::neps) | (Vec4f::eps < a);
    // compute the (approximative) inverse
    Vec4f f = _mm_rcp_ps(a);
    // compute the vector from the triangle
    // corner to the ray origin
    Vec4f sx = rays.x - Ax;
    Vec4f sy = rays.y - Ay;
    Vec4f sz = rays.z - Az;
    // compute the first intersection scalar
    // and check if it is bounds
    Vec4f u = ((sx * hx) + (sy * hy) + (sz * hz)) * f;
    Vec4f mask2 = (Vec4f::zeros < u) & (u < Vec4f::ones);
    // compute cross product
    Vec4f qx = (sy * Uz) - (sz * Uy);
    Vec4f qy = (sz * Ux) - (sx * Uz);
    Vec4f qz = (sx * Uy) - (sy * Ux);
    // compute the second intersection scalar
    // and again check if it is in bounds
    Vec4f v = ((rays.u * qx) + (rays.v * qy) + (rays.w * qz)) * f;
    Vec4f mask3 = (Vec4f::zeros < v) & ((u + v) < Vec4f::ones);
    // at this stage we know that the ray intersects
    // with the triangle and we can compute the distance
    // between them
    Vec4f t = ((Vx * qx) + (Vy * qy) + (Vz * qz)) * f;
    // make sure that only intersections that are
    // in front of the ray are counted
    Vec4f mask4 = (Vec4f::eps < t);
    // combine all maskes into one
    Vec4f mask = mask1 & mask2 & mask3 & mask4;

    // set all values of the
    // hit record packet
    records.t = t;
    // hit points
    records.px = rays.x + t * rays.u;
    records.py = rays.y + t * rays.v;
    records.pz = rays.z + t * rays.w;
    // normals
    records.nx = Nx;
    records.ny = Ny;
    records.nz = Nz;
    // incident ray directions
    records.vx = rays.u;
    records.vy = rays.v;
    records.vz = rays.w;
    // hit records of rays that actually do
    // not hit the triangle are set invalid
    // and thus ignored by the renderer
    records.valids = mask;
    // update all materials
    // to the triangle material
    records.mats[0] = mat;
    records.mats[1] = mat;
    records.mats[2] = mat;
    records.mats[3] = mat;
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

void PrimitiveList::cast_packet(
    const Ray4& rays,
    HitRecord4& records,
    const size_t& n_valids
) const {
    // temporary hitrecord to compare
    // with the current best
    HitRecord4 tmp;
    // check all primitives in list
    for (const Primitive* p : *this) {
        // cast the ray packet to the
        // current primitive and update
        // the hitrecords
        p->cast_packet(rays, tmp, n_valids);
        records.update(tmp); 
    }
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
