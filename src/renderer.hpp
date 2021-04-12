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
// shortcut for a queue of render buckets
using RenderQueue = std::vector<RenderBucket>;

// struct holding all arguments
// needed to render the contained
// rays and all their scatter rays
typedef struct RenderArgs {
    // the contribution buffer holding the
    // ray contributions of all the primary
    // rays (secondary rays re-use these too)
    RayContrib* contrib_buffer;
    // the length of the contribution buffer
    size_t buffer_length;
    // all primary rays through the pixel
    // and secondary rays generated from them
    RayQueue rays;
    // the rays sorted by the leafs of the
    // bounding volume hierarchy
    std::vector<RayQueue> sorted_rays;
    // the queue of render buckets that
    // are yet to processed by the renderer
    RenderQueue render_buckets;
    // constructor and destructor
    RenderArgs(
        const size_t& n_rays,
        const BVH& bvh
    );
    ~RenderArgs(void);
} RenderArgs;

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

    // steps of the rendering pipeline
    // 1) build all primary camera rays
    // through a specific pixel and push
    // them into the render chunk
    void build_pixel_rays(
        RenderArgs& args,
        const size_t& i,
        const size_t& j,
        const size_t& width,
        const size_t& height,
        const float& vpw,
        const float& vph
    ) const;
    // 2) sort the rays that are currently
    // stored in the ray queue into
    // render buckets
    void sort_rays_into_buckets(
        RenderArgs& args
    ) const;
    // 3) flush the render buckets 
    void flush_buckets(
        RenderArgs& args
    ) const;
    // 4) compute the color of each ray
    //    and build the secondary rays
    void build_secondary_rays(
        RenderArgs& args
    ) const;
public:
    // constructor
    Renderer(
        const Scene& scene,
        const Camera& cam,
        const size_t& rpp,
        const size_t& max_rdepth
    );
    // render pipeline
    void render(FrameBuffer& fb) const;
    // apply the full rendering pipeline
    // to the given render arguments
    void render(RenderArgs& args) const;
};

#endif // H_RENDERER
