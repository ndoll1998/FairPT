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
 *  Axis-Aligned Bounding Box
 */

AABB::AABB(
    const Vec3f& A,
    const Vec3f& B
) :
    min(A.min(B)),
    max(A.max(B))
{
}

Vec3f AABB::center(void) const {
    return (min + max) * 0.5f;
}

bool AABB::cast(const Ray& r) const
{
    const Vec3f l1 = (min - r.origin) / r.direction;
    const Vec3f l2 = (max - r.origin) / r.direction;   
    const Vec4f filtered_l1a = l1.min(Vec3f::inf);
    const Vec4f filtered_l2a = l2.min(Vec3f::inf);
    const Vec4f filtered_l1b = l1.max(Vec3f::ninf);
    const Vec4f filtered_l2b = l2.max(Vec3f::ninf);
    Vec4f lmax = filtered_l1a.max(filtered_l2a);
    Vec4f lmin = filtered_l1b.min(filtered_l2b);
    lmax = lmax.min(lmax.rotate());
    lmin = lmin.max(lmin.rotate());
    lmax = lmax.min(_mm_movehl_ps(lmax, lmax));
    lmin = lmin.max(_mm_movehl_ps(lmin, lmin));
    return _mm_comige_ss(lmax, Vec4f::zeros) & _mm_comige_ss(lmax, lmin);
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

AABB Triangle::build_aabb(void) const
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
    const Ray& ray,
    const std::array<Vec4f, 3>& A,
    const std::array<Vec4f, 3>& U,
    const std::array<Vec4f, 3>& V,
    float& t,   // distance to intersection point
    size_t& j   // index of triangle with closest hit
) {
    // Möller–Trumbore intersection algorithm
    // using simd instructions for parallel
    // processing of triangles rays at once
    // TODO: share these along all triangles
    std::array<Vec4f, 3> ray_orig = { 
        Vec4f(ray.origin[0]), 
        Vec4f(ray.origin[1]),
        Vec4f(ray.origin[2])
    };
    std::array<Vec4f, 3> ray_dir = {
        Vec4f(ray.direction[0]), 
        Vec4f(ray.direction[1]),
        Vec4f(ray.direction[2])
    };

    // check if the ray is parallel to triangle 
    std::array<Vec4f, 3> h; cross(h, ray_dir, V); 
    Vec4f a = dot(U, h);
    Vec4f mask1 = (a < Vec4f::neps) | (Vec4f::eps < a);
    // check if intersection in
    // range of first edge
    Vec4f f = Vec4f::ones / a;
    std::array<Vec4f, 3> s; sub(s, ray_orig, A);
    Vec4f u = dot(s, h) * f;
    Vec4f mask2 = (Vec4f::zeros < u) & (u < Vec4f::ones);
    // check if intersection is
    // in range of both edges
    std::array<Vec4f, 3> q; cross(q, s, U);
    Vec4f v = dot(ray_dir, q) * f;
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
        if (TriangleCollection::cast_ray_triangle(ray, a, u, v, tmp_t, tmp_j)) {
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


/*
 *  Bounding Volume Hierarchy
 */

BVH::BVH(
    const std::vector<Triangle*>& tris,
    const size_t& max_depth,
    const size_t& min_size
) {
    // compute the depth of the tree
    depth = ceil(log2f((float)tris.size()));
    depth = (depth > max_depth)? max_depth : depth;
    // compute the number of inner and total nodes
    n_inner_nodes = pow(2, depth) - 1;
    n_total_nodes = pow(2, depth+1) - 1;
    // the number of leaf nodes is initially
    // set to zero but incremented whenever
    // a leaf node is created
    n_leaf_nodes = 0;
    // allocate memory for the binary tree
    tree = new bvh_node[n_total_nodes];
    // allocate memory to store
    //  - the primitive assignment of inner nodes
    //  - if the node is a valid inner node
    std::vector<Triangle*> tmp_tris_assign[n_inner_nodes];
    bool valid_inner[n_inner_nodes];
    
    // helper function to set the value of
    // a node during construction of the tree
    auto set_node = [this, &tmp_tris_assign, &valid_inner, &min_size](
        const size_t& i,
        const std::vector<Triangle*>::const_iterator& begin,
        const std::vector<Triangle*>::const_iterator& end
    ) {
        // initially mark the current node as invalid
        // provided that it is not in the last layer
        // this will be overriden if the node
        // turns out to be an inner node
        if (i < n_inner_nodes) { valid_inner[i] = false; }
        // get the number of elements stored
        // in the subtree of the current node
        size_t d = std::distance(begin, end);
        // make sure the subtree rooted at i
        // stores at least one primitive
        // this is only violated if the initial
        // primitive list that is passed to the
        // bounding Volume Hierarchy is empty
        if (d == 0) {
            // note that in this case the node
            // is a leaf node with invalid id 
            tree[i] = { AABB(), true, (size_t)-1 };
            return;
        }
        // create a vector storing the
        // triangles in the given iterator
        std::vector<Triangle*> cur_tris(begin, end);
        // build the axis aligned bounding box
        // that contains all triangles in the vector
        AABB aabb = cur_tris[0]->build_aabb();
        for (const Triangle* t : cur_tris) {
            AABB tmp = t->build_aabb();
            aabb.min = aabb.min.min(tmp.min);
            aabb.max = aabb.max.max(tmp.max);
        }
        // check if the node is a leaf node
        // which means that either
        //  - the node is at maximum depth
        //  - the minumum number of primitives would be 
        //    violated by splitting the node again
        if ((i >= n_inner_nodes) || (d < min_size * 2)) {
            // set the node of the tree to be a leaf node
            tree[i] = { aabb, true, n_leaf_nodes++ };
            // pack triangles from vector into a collection
            TriangleCollection tris_col;
            std::for_each(begin, end, [&tris_col](const Triangle* t) { tris_col.push_back(*t); });
            // push the triangle collection
            leaf_tris_col.push_back(tris_col);
        } else {
            // if none of the above statements hold true
            // then the current node is an inner node
            tree[i] = { aabb, false, (size_t)-1 };
            tmp_tris_assign[i] = cur_tris;
            valid_inner[i] = true;
		}
    };

    // set root of the tree
    set_node(0, tris.begin(), tris.end());
    // build binary tree in top-down fashion
    // starting at the root and splitting it up
    for (size_t i = 0; i < n_inner_nodes; i++) {

        // make sure the current node is a valid 
        // inner node, i.e. it is not in the 
        // subtree rooted at a node that was 
        // previously declared to be a leaf node 
        // (see set_node function)
        if (!valid_inner[i]) { continue; }
        
        // get the list of triangles assigned
        // to the current inner node
        std::vector<Triangle*> node_tris = tmp_tris_assign[i];
        
        // collect the center points of all
        // primitves in the current node
        std::vector<Vec3f> vecs; vecs.reserve(node_tris.size());
        for (const Triangle* p : node_tris)
            vecs.push_back(p->build_aabb().center());
        // compute their variance for each dimension
        Vec3f inv = Vec3f(1.0f / vecs.size());
        Vec3f mean = std::accumulate(vecs.begin(), vecs.end(), Vec3f::zeros) * inv;
        for (Vec3f& v : vecs) { v = v - mean; v = v * v; }
        Vec3f var = std::accumulate(vecs.begin(), vecs.end(), Vec3f::zeros) * inv;
        // choose dimension with maximum variance
        // as the split axis for the current node
        float x = var[0], y = var[1], z = var[2];
        short axis = (x > y)? 
                    ( (z > x)? 2 : 0 ) : 
                    ( (z > y)? 2 : 1 ) ;
        // create comparator for choosen axis
        auto comp = [&axis](
            const Triangle* t1, 
            const Triangle* t2
        ) -> bool {
            return t1->build_aabb().center()[axis] < t2->build_aabb().center()[axis]; 
        };
        // find median along choosen axis
        std::vector<Triangle*>::iterator begin = node_tris.begin();
        std::vector<Triangle*>::iterator median = node_tris.begin() + node_tris.size() / 2;
        std::vector<Triangle*>::iterator end = node_tris.end();
        std::nth_element(begin, median, end, comp);
        // build children nodes by splitting the
        // primitive list at the median
        set_node(2 * i + 1, begin, median);
        set_node(2 * i + 2, median, end);
    }
}

BVH::~BVH(void)
{
    // free the memory allocated
    // to store the tree
    delete[] tree;
}

void BVH::get_intersecting_leafs(
    const Ray& ray,
    std::vector<size_t>& leaf_ids
) const {
    // make sure the leaf ids vector
    // is initially empty
    leaf_ids.clear();
    // create a queue to hold all inner nodes
    // that need to be checked during traversal
    std::queue<size_t> q;
    q.push(0);      // add root node
    // traverse the tree
    while (!q.empty()) {
        // get the next node to process
        // and remove it from the queue
        size_t i = q.front();
        bvh_node node = tree[i];
        q.pop();
        // check if the ray intersects with
        // the current node
        if (node.aabb.cast(ray)) {
            // check if the node is a leaf node
            // by testing if it has any triangles
            // assigned to it
            if (node.is_leaf) {
                // add the node to the leaf ids vector
                leaf_ids.push_back(node.leaf_id);
            } else if (i < n_inner_nodes) {
                // add all children of the current node
                // to the queue to check them later
                q.push(2 * i + 1);
                q.push(2 * i + 2);
            }
        }
    }
}

const Primitive* BVH::leaf_primitive(
    const size_t& i
) const {
    // return the triangle collection assigned
    // to the node with the given leaf index
    return &leaf_tris_col[i];
}

size_t BVH::n_leafs(void) const {
    // return the number of leaf nodes
    // that were created during construction
    // of the tree
    return n_leaf_nodes;
}

