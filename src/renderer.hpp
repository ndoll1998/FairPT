#ifndef H_RENDERER
#define H_RENDERER

// forward declarations
class FrameBuffer;
// includes
#include <vector>
#include "./ray.hpp"
#include "./bvh.hpp"
#include "./scene.hpp"
#include "./camera.hpp"
#include "./primitive.hpp"

// structure holding the rays and the
// primitive of a render bucket
typedef struct RenderBucket {
    RayQueue& rays;
    const Primitive* prim;
} RenderBucket;


class Renderer {
private:
    // references to objects that are heavily
    // used during the rendering process
    const Scene& scene;
    const Camera& cam;
    const BVH& bvh;
    const PrimitiveList& primitives; 
    // number of rays per pixel
    size_t rpp;
    // maximum number of secondary
    // rays per primary ray
    size_t max_rdepth;
    // ray queue storing rays that are created
    // during the rendering process
    RayQueue rays;
    // ray buckets that rays get sorted into
    // during the rendering process
    std::vector<RayQueue> sorted_rays;
    // queue of render buckets that are yet
    // to be processed
    std::vector<RenderBucket> render_buckets;

    // steps of the rendering pipeline
    // 1) build all primary camera rays
    void build_primary_rays(
        RayContrib* contrib_buffer,
        const size_t& width,
        const size_t& height
    );
    // 2) sort the rays that are currently
    //    stored in the ray queue into
    //    render buckets
    void sort_rays_into_buckets(void);
    // 3) flush the render buckets 
    void flush_buckets(void);
    // 4) compute the color of each ray
    //    and build the secondary rays
    void build_secondary_rays(
        RayContrib* contrib_buffer,
        const size_t& buffer_length
    );
public:
    // constructor
    Renderer(
        const Scene& scene,
        const Camera& cam,
        const size_t& rpp,
        const size_t& max_rdepth
    );
    // render pipeline
    void render(FrameBuffer& fb);
};

#endif // H_RENDERER
