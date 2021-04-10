#include "./renderer.hpp"
#include "./framebuffer.hpp"
#include <rng.hpp>
#include <math.h>

Renderer::Renderer(
    const Scene& scene,
    const Camera& cam,
    const size_t& rpp,
    const size_t& max_rdepth
) :
    scene(scene),
    cam(cam),
    bvh(scene.bvh()),
    primitives(scene.primitives()),
    rpp(rpp),
    max_rdepth(max_rdepth),
    sorted_rays(bvh.num_leafs())
{
}

void Renderer::build_primary_rays(
    RayContrib* contrib_buffer,
    const size_t& width,        // width of the frame to render
    const size_t& height        // height of the frame to render
) {
    // compute the width and
    // height of the viewport
    float vpw = 2.0f * tanf(0.5f * cam.fov());
    float vph = vpw * (float)height / (float)width;
    // index to access the contribution buffer
    size_t idx = 0;
    // create a ray buffer and add 
    // all primary camera rays to it   
    for (size_t i = 0; i < height; i++) {
        for (size_t j = 0; j < width; j++) {
            // build all rays that go through
            // the current pixel (i, j)
            for (size_t k = 0; k < rpp; k++) {
                // fill a 2x2 sub-pixel grid
                // and add a noise term
                size_t pi = k / 2 % 2, pj = k % 2;                
                float su = (float)(i * 2 + pi + rng::randf()) / (2 * height) - 0.5f;
                float sv = (float)(j * 2 + pj + rng::randf()) / (2 * width) - 0.5f;
                // build the ray with origin on the viewport
                // and direction through the sub-pixel
                Ray r = cam.build_ray_from_uv(su * vph, sv * vpw);
                // set the pointer to the ray contribution
                // of the primary ray and add it to the queue
                r.contrib = contrib_buffer + (idx++);
                rays.push_back(r);
            }
        }
    }
}

void Renderer::sort_rays_into_buckets(void)
{
    // let the bounding volume hierarchy sort
    // the ray queue into the leaf buckets
    bvh.sort_rays_by_leafs(rays, sorted_rays);
    // clear the ray queue since all rays
    // now are sorted into buckets
    rays.clear();
    // build the render buckets combining
    // a queue of rays with the primitive
    // to cast the rays to
    for (size_t i = 0; i < sorted_rays.size(); ++i) {
        RayQueue& queue = sorted_rays[i];
        // add a render bucket from the current
        // sorted queue if it is not empty
        if (!queue.empty()) {
            RenderBucket bucket = { queue, primitives[i] };
            render_buckets.push_back(bucket);
        }
    }
}

void Renderer::flush_buckets(void)
{
    // create a temporary hitrecord to compare
    // to the current best
    HitRecord tmp;
    // process all render buckets
    for (RenderBucket& bucket : render_buckets) {
        // cast each ray against the associated primitive
        // and update the hitrecord to discribe the closest hit
        for (Ray& ray : bucket.rays) {
            // get a reference to the current hitrecord
            HitRecord& record = ray.contrib->hit_record;            
            // cast and update the hitrecord
            if (bucket.prim->cast(ray, tmp) && (
                    (tmp.is_valid && !record.is_valid) ||
                    (tmp.is_valid && record.is_valid && (record.t > tmp.t))
            )) { record = tmp; }
            // reset the temporary hitrecord
            tmp.is_valid = false;
        }
        // clear the rays of the current bucket
        bucket.rays.clear();
    }
}

void Renderer::build_secondary_rays(
    RayContrib* contrib_buffer,
    const size_t& buffer_length
) {
    // compute all colors
    // and build all scatter rays
    for (size_t i = 0; i < buffer_length; i++) {
        // get the current contribution info
        RayContrib* contrib = contrib_buffer + i;
        HitRecord& h = contrib->hit_record;
        // make sure the color is not final
        if (contrib->is_final) { continue; }
        // check if the hit record is valid, i.e.
        // if the corresponding ray hit anything
        if (h.is_valid) {
            // get the attenuation and emittance
            // color of the material at the hit point
            Vec3f att = h.mat->attenuation(h);
            Vec3f emit = h.mat->emittance(h);
            // update the color values
            // in the contribution buffer
            contrib->color = contrib->color + contrib->albedo * emit;
            contrib->albedo = contrib->albedo * att;
            // create the scatter ray
            // from the hit record
            Ray scatter;
            if (h.mat->scatter(h, scatter)) {
                // offset ray origin slightly to avoid 
                // intersecting at the ray origin
                scatter.origin = Vec3f::eps.fmadd(scatter.direction, scatter.origin);
                // reset the hit record to
                // reuse it for the scatter ray
                h.is_valid = false; 
                // set contribution of the scatter ray
                // and push the ray into the queue
                scatter.contrib = contrib;
                rays.push_back(scatter);
            }
        } else {
            // the ray corresponding to the
            // contribution info did not hit
            // any primitive
            // contrib.color = contrib.color + contrib.albedo;
            contrib->is_final = true;
        }
    }
}

void Renderer::render(FrameBuffer& fb) 
{
    // create an array that stores all contribution
    // infos of the primary rays (secondary rays
    // also re-use these items)
    size_t n_primary_rays = fb.width() * fb.height() * rpp;
    RayContrib* contrib_buffer = new RayContrib[n_primary_rays];
    // fill the ray queue with the initial
    // primary camera rays
    build_primary_rays(contrib_buffer, fb.width(), fb.height());
    // main rendering loop iterating until the
    // ray queue is empty or the maximum recursion
    // depth is reached
    size_t rdepth = 0;
    while ((!rays.empty()) && (rdepth++ < max_rdepth)) {
        // sort the rays from the ray queue
        // into render buckets
        sort_rays_into_buckets();
        // flush the render buckets, i.e.
        // compute all closest hit-records
        flush_buckets();
        // fill the queue with scatter
        // rays from the current iteration
        build_secondary_rays(contrib_buffer, n_primary_rays);
    }
    // index to access the contribution
    // buffer with
    size_t idx = 0;
    // write all compute colors to
    // the framebuffer
    for (size_t i = 0; i < fb.height(); i++) {
        for (size_t j = 0; j < fb.width(); j++) {
            // average the color over all rays
            // that go through the current pixel
            Vec3f c = Vec3f::zeros;
            for (size_t k = 0; k < rpp; k++)
                c = c + contrib_buffer[idx++].color;
            c = c / (float)rpp;
            // apply postprocessing including
            // a simple approxiamtion of
            // gamma correction filter
            c = c.min(Vec3f::ones).max(Vec3f::zeros);
            c = c.sqrt() * 255.0f;
            // write the color to the framebuffer
            fb.set_pixel(i, j, c[0], c[1], c[2]);
        }
    }
}
