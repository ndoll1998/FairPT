#include "./ray.hpp"

/*
 *  RayBucket
 */

void RayBucket::push_back(
    const Ray& ray,
    ContribInfo* contrib
) {
    // push the ray and contribution info
    // as a pair on the vector
    pairs.push_back({ ray, contrib });
}

// basic vector operations
bool RayBucket::empty(void) const { return pairs.empty(); }
size_t RayBucket::size(void) const { return pairs.size(); }

RayContribPacket RayBucket::pop_packet(void) {
    // get the number of rays
    // we want to pack together
    size_t n = pairs.size();
    size_t k = (n < 4)? n : 4;
    // pack them together
    // with their contribtion infos
    RayContribPacket packet;
    for (size_t i = 0; i < k; i++) {
        // get the current pair to
        // add to the packet
        RayContribPair pair = pairs.at(n - i - 1);
        // move the positional information
        // to the packet
        packet.rays.x[i] = pair.ray.origin[0];
        packet.rays.y[i] = pair.ray.origin[1];
        packet.rays.z[i] = pair.ray.origin[2];
        // move the directional information
        // to the packet
        packet.rays.u[i] = pair.ray.direction[0];
        packet.rays.v[i] = pair.ray.direction[1];
        packet.rays.w[i] = pair.ray.direction[2];
        // also add the contribution info
        // to the packet
        packet.contribs[i] = pair.contrib;
    }
    // set the number of valid rays
    // in the packet
    packet.n_valids = k;
    // remove the pairs that were just 
    // added to the packet from the vector
    pairs.resize(pairs.size() - k);
    // return the ray packet
    return packet;
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
    ContribInfo* contrib,
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
