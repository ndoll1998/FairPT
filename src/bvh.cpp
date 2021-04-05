#include "./bvh.hpp"
#include "./ray.hpp"
#include <math.h>
#include <numeric>
#include <algorithm>
#include <queue>

/*
 *  Axis-Aligned Bounding Box
 */

AABB::AABB(
    const Vec3f& A,
    const Vec3f& B
) :
    low(A.min(B)),
    high(A.max(B))
{
}

Vec3f AABB::center(void) const {
    return (low + high) * 0.5f;
}

bool AABB::cast(const Ray& r) const
{
    const Vec3f l1 = (low - r.origin) / r.direction;
    const Vec3f l2 = (high - r.origin) / r.direction;   
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
 * Axis-Aligned Bounding Box Packet
 */

AABB4::AABB4(
        const AABB& A,
        const AABB& B,
        const AABB& C,
        const AABB& D
) :
    // set all low coordinates
    low{
        Vec4f(A.low[0], B.low[0], C.low[0], D.low[0]),
        Vec4f(A.low[1], B.low[1], C.low[1], D.low[1]),
        Vec4f(A.low[2], B.low[2], C.low[2], D.low[2])
    },
    // set all high coordinates
    high{
        Vec4f(A.high[0], B.high[0], C.high[0], D.high[0]),
        Vec4f(A.high[1], B.high[1], C.high[1], D.high[1]),
        Vec4f(A.high[2], B.high[2], C.high[2], D.high[2])
    }
{
}

unsigned int AABB4::cast(const Ray4& ray) const
{
    Vec4f t0x = (low[0] - ray.origin[0]) / ray.direction[0];
    Vec4f t0y = (low[1] - ray.origin[1]) / ray.direction[1];
    Vec4f t0z = (low[2] - ray.origin[2]) / ray.direction[2];
    Vec4f t1x = (high[0] - ray.origin[0]) / ray.direction[0];
    Vec4f t1y = (high[1] - ray.origin[1]) / ray.direction[1];
    Vec4f t1z = (high[2] - ray.origin[2]) / ray.direction[2];
    Vec4f minx = t0x.min(t1x);
    Vec4f miny = t0y.min(t1y);
    Vec4f minz = t0z.min(t1z);
    Vec4f maxx = t0x.max(t1x);
    Vec4f maxy = t0y.max(t1y);
    Vec4f maxz = t0z.max(t1z);
    Vec4f tmin_max = minx.max(miny.max(minz));
    Vec4f tmax_min = maxx.min(maxy.min(maxz));
    return _mm_movemask_ps(tmin_max < tmax_min);
}


/*
 *  Bounding Volume Hierarchy
 */

BVH::BVH(
    const BoundableList& objs,
    const size_t& max_depth,
    const size_t& min_size
) {
    // compute the depth of the tree
    depth = ceil(log2f((float)objs.size()) / log2f(4.0f));
    depth = (depth > max_depth)? max_depth : depth;
    // compute the number of inner and total nodes
    n_inner_nodes = pow(4, depth) - 1;
    n_total_nodes = pow(4, depth+1) - 1;
    // the number of leaf nodes is initially
    // set to zero but incremented whenever
    // a leaf node is created
    n_leaf_nodes = 0;
    // allocate memory for the binary tree
    tree = new bvh_node[n_total_nodes];
    // allocate memory to store
    //  - the primitive assignment of inner nodes
    //  - if the node is a valid inner node
    BoundableList tmp_objs_assign[n_inner_nodes];
    bool valid_inner[n_inner_nodes];
    
    // helper function to set the value of
    // a node during construction of the tree
    auto set_node = [this, &tmp_objs_assign, &valid_inner, &min_size](
        const size_t& i,
        const BoundableList::const_iterator& begin,
        const BoundableList::const_iterator& end
    ) -> AABB {
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
            tree[i].is_leaf = true;
            tree[i].leaf_id = (size_t)-1;
            return AABB();
        }
        // create a vector storing the
        // triangles in the given iterator
        BoundableList node_objs(begin, end);
        // check if the node is a leaf node, i.e.
        //  - the node is at maximum depth or
        //  - the minumum number of primitives would be 
        //    violated by splitting the node again
        if ((i >= n_inner_nodes) || (d < min_size * 2)) {
            // set the node of the tree to be a leaf node
            tree[i].is_leaf = true;
            tree[i].leaf_id = n_leaf_nodes++;
            // push the boundable list of the leaf node
            leaf_objs.push_back(node_objs);
        } else {
            // if none of the above statements hold true
            // then the current node is an inner node
            tree[i].is_leaf = false;
            tree[i].leaf_id = (size_t)-1;
            tmp_objs_assign[i] = node_objs;
            valid_inner[i] = true;
		}
        // build the axis aligned bounding box
        // that contains all triangles assigned
        // to the current node i
        AABB aabb = node_objs[0]->bound();
        for (const Boundable* t : node_objs) {
            AABB tmp = t->bound();
            aabb.low = aabb.low.min(tmp.low);
            aabb.high = aabb.high.max(tmp.high);
        }
        return aabb;
    };

    // set root of the tree
    set_node(0, objs.begin(), objs.end());
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
        BoundableList node_objs = tmp_objs_assign[i];
        
        // collect the center points of all
        // primitves in the current node
        std::vector<Vec3f> vecs; vecs.reserve(node_objs.size());
        for (const Boundable* p : node_objs)
            vecs.push_back(p->bound().center());
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
            const Boundable* a, 
            const Boundable* b
        ) -> bool {
            return a->bound().center()[axis] < b->bound().center()[axis]; 
        };
        // find median along choosen axis
        BoundableList::iterator begin = node_objs.begin();
        BoundableList::iterator splitA = begin + node_objs.size() / 4;
        BoundableList::iterator median = begin + node_objs.size() / 2;
        BoundableList::iterator splitB = median + node_objs.size() / 4;
        BoundableList::iterator end = node_objs.end();
        std::nth_element(begin, median, end, comp);
        std::nth_element(begin, splitA, median, comp);
        std::nth_element(median, splitB, end, comp);
        // build children nodes by splitting the
        // primitive list at the median
        tree[i].aabb4 = AABB4(
            set_node(4 * i + 1, begin, splitA),
            set_node(4 * i + 2, splitA, median),
            set_node(4 * i + 3, median, splitB),
            set_node(4 * i + 4, splitB, end)
        );
    }
}

BVH::~BVH(void)
{
    // free the memory allocated
    // to store the tree
    delete[] tree;
}

const BoundableList& BVH::get_leaf_objects(const size_t& leaf_id) const
{
    // return a pointer to the primitive
    // of the given leaf referenced by id
    return leaf_objs[leaf_id];
}

void BVH::sort_rays_by_leafs(
    const RayQueue& rays,
    std::vector<RayQueue>& sorted
) const {
    // create a queue to hold all inner nodes
    // that need to be checked during traversal
    std::queue<size_t> q;
    for (const Ray& ray : rays) {
        // build ray packet from ray
        Ray4 ray_packet = {
            { Vec4f(ray.origin[0]), Vec4f(ray.origin[1]), Vec4f(ray.origin[2]) },
            { Vec4f(ray.direction[0]), Vec4f(ray.direction[1]), Vec4f(ray.direction[2]) }
        };
        // start with the leaf node
        q.push(0);
        // traverse the tree
        while (!q.empty()) {
            // get the next node to process
            // and remove it from the queue
            size_t i = q.front(); q.pop();
            bvh_node node = tree[i];
            // check if the node is a valid leaf
            if (node.is_leaf && (node.leaf_id < (size_t)-1)) {
                // push the ray into the queue
                // that corresponds to the leaf node
                sorted[node.leaf_id].push_back(ray);
                continue;
            }
            // cast the ray to the bounding box packet
            unsigned int mask = node.aabb4.cast(ray_packet);
            // for each box that intersect with
            // the ray add the corresponding child
            // to the queue
            for (size_t j = 0; j < 4; j++) {
                // check if the box intersects with the ray
                if (mask & 1u) { q.push(4 * i + j + 1); }
                // go on with the next box
                mask >>= 1;
            }
        }
    }
}

const size_t& BVH::num_leafs(void) const {
    // return the number of leaf nodes that were
    // created during construction of the tree
    return n_leaf_nodes;
}

