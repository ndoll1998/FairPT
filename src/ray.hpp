#ifndef H_RAY
#define H_RAY

// includes
#include <vector>
#include "./vec.hpp"
#include "./primitive.hpp"

// contribution information needed to
// interatively update the color of a
// primary ray when tracing a scatter ray
typedef struct ContribInfo {
    Vec3f color = Vec3f::zeros;     // color after i (scatter-) rays
    Vec3f albedo = Vec3f::ones;     // color influence of the current ray
    HitRecord hit_record;           // hit-record of the currentl ray
} ContribInfo;

// ray structure combining positional
// and directional information
typedef struct Ray {
    Vec3f origin;
    Vec3f direction;
} Ray;

// a packet of 4 rays
typedef struct Ray4 {
    Vec4f x, y, z;  // all positional information
    Vec4f u, v, w;  // all directional information
} Ray4;

// struct combining a single ray
// with it's contribution info
typedef struct RayContribPair {
    Ray ray;
    ContribInfo* contrib;
} RayContribPair;

// struct combining a ray packet
// with a references to their contribution info
typedef struct RayContribPacket {
    Ray4 rays;                  // the rays of the packet
    ContribInfo* contribs[4];   // contribution infos of the rays
    size_t n_valids;            // number of valid rays on the packet
} RayContribPacket;


// Ray Buckets store a vector of rays
// that are yet to be casted against the 
// a common set of primitives
class RayBucket {
private:
    // vector to store all rays with references to 
    // their contribution infos of the bucket
    std::vector<RayContribPair> pairs;
public:
    // add a ray and its contribution info
    // to the bucket
    void push_back(
        const Ray& r,
        ContribInfo* contrib
    );
    // functions to access some of the
    // basic vector properties
    void clear(void);
    bool empty(void) const;
    size_t size(void) const; 
    // build a ray-packet of the
    // first 4 rays in the bucket
    // and remove them
    RayContribPacket pop_packet(void);
};


// A Ray Cache manages a set of ray buckets
// where each ray bucket corresponds to exactly
// one leaf of the bounding volume hierarchy
class RayCache {
private:
    // array of ray buckets
    RayBucket* buckets;
    size_t n_buckets;
public: 
    // constructor
    RayCache(void) = default;
    RayCache(const size_t& n_buckets);
    ~RayCache(void);
    // sort a ray into multiple buckets
    void sort_into_buckets(
        const Ray& ray,             // ray to push into buckets
        ContribInfo* contrib,       // contribution info associated with the ray
        std::vector<size_t> ids     // ids of buckets to push the ray to
    );
    // functions to get the bucket
    // at a specific index
    RayBucket& get_bucket(const size_t& i);
    // check if the cache is empty, i.e.
    // contains any rays at all
    bool empty(void) const;
};

#endif // H_RAY
