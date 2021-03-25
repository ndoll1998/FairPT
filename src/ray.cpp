#include "./ray.hpp"

/*
 *  RayBucket
 */

void RayBucket::push_back(
    const Ray& ray,
    ContribInfo& contrib
) {
    // push the ray and contribution info
    // as a pair on the vector
    pairs.push_back({ ray, contrib });
}

// basic vector operations
void RayBucket::clear(void) { pairs.clear(); }
bool RayBucket::empty(void) const { return pairs.empty(); }
size_t RayBucket::size(void) const { return pairs.size(); }

RayContribPair RayBucket::pop(void) {
    // get the last element of the pairs
    // vector and remove it
    RayContribPair pair = pairs.back();
    pairs.pop_back();
    return pair;
}

RayPacket RayBucket::pop_packet(void) {
    // TODO
    return RayPacket();
}


/*
 *  RayCache
 */

RayCache::RayCache(const size_t& n_buckets) :
    buckets(new RayBucket[n_buckets]),
    n_buckets(n_buckets)
{}

RayCache::~RayCache(void) {
    // clear memory allocated for buckets
    delete[] buckets;
}

RayBucket& RayCache::get_bucket(
    const size_t& i
) {
    // get the bucket at i-th position
    return buckets[i];
}

void RayCache::sort_into_buckets(
    const Ray& ray,
    ContribInfo& contrib,
    std::vector<size_t> ids
) {
    // push the ray and its contribution
    // info onto all buckets with id in
    // the provided list of ids
    for (size_t& i : ids) {
        buckets[i].push_back(ray, contrib);
    }
}

bool RayCache::empty(void) const {
    // loop through all buckets and
    // check if there is one that is
    // not empty
    for (size_t i = 0; i < n_buckets; i++) {
        if (!buckets[i].empty()) { return false; }
    }
    // if there is no such bucket then
    // the whole cache is empty
    return true; 
}
