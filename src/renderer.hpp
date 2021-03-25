#ifndef H_RENDERER
#define H_RENDERER

// forward declarations
class FrameBuffer;
// includes
#include "./ray.hpp"
#include "./scene.hpp"
#include "./camera.hpp"
#include "./primitive.hpp"

class Renderer {
private:
    // references to the scene, camera
    // and the bounding volume hierarchy
    const Scene& scene;
    const Camera& cam;
    const BVH& bvh;
    // number of rays per pixel
    size_t rpp;
    // maximum number of secondary
    // rays per primary ray
    size_t max_rdepth;
    // ray cache storing buckets that are filled 
    // and cleared during the rendering process
    RayCache ray_cache;
    // steps of the rendering pipeline
    // 1) build all primary rays and
    //    sort them into the buckets
    void build_init_cache(
        ContribInfo* contrib_buffer,
        const size_t& width,
        const size_t& height
    );
    // 2) flush the ray buckets by 
    //    casting each ray to the 
    //    associated primitives
    void flush_cache(void);
    // 3) compute the color of
    //    each ray and build
    //    secondary rays
    void build_next_cache(
        ContribInfo* contrib_buffer,
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
