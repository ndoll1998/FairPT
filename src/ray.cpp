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

RayContribPair RayBucket::pop(void) {
    // get the last element from the
    // vector of pairs and remove it
    RayContribPair pair = pairs.back();
    pairs.pop_back();
    // return the ray-contribution pair
    return pair;
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

size_t RayCache::pop_non_empty(void) {
    // get the last non_empty id and
    // remove it from the vector
    size_t i = non_empty_ids.back();
    non_empty_ids.pop_back();
    return i;
}

void RayCache::sort_into_buckets(
    const Ray& ray,
    ContribInfo* contrib,
    std::vector<size_t> ids
) {
    // push the ray and its contribution
    // info into all buckets with id in
    // the provided list of ids
    for (size_t& i : ids) {
        RayBucket& bucket = buckets[i];
        bool was_empty = bucket.empty();
        // push the ray into the bucket
        bucket.push_back(ray, contrib);
        // add the bucket to the non-empty vector
        // if it is not yet on there, i.e. if the
        // bucket was empty previously
        if (was_empty) { non_empty_ids.push_back(i); }
    }
}

bool RayCache::empty(void) const {
    // the ray cache is empty if there
    // are no non-empty buckets
    return non_empty_ids.empty();
}
