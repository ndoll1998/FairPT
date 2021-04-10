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
    max_rdepth(max_rdepth)
{
}

void Renderer::build_pixel_rays(
    RenderArgs& args,
    const size_t& i,
    const size_t& j,
    const size_t& width,
    const size_t& height,
    const float& vpw,
    const float& vph
) const {
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
        r.contrib = args.contrib_buffer + k;
        args.rays.push_back(r);
    }
}

void Renderer::sort_rays_into_buckets(
    RenderArgs& args
) const {
    // let the bounding volume hierarchy sort
    // the ray queue into the leaf buckets
    bvh.sort_rays_by_leafs(args.rays, args.sorted_rays);
    // clear the ray queue since all rays
    // now are sorted into buckets
    args.rays.clear();
    // build the render buckets combining
    // a queue of rays with the primitive
    // to cast the rays to
    for (size_t i = 0; i < args.sorted_rays.size(); ++i) {
        RayQueue& queue = args.sorted_rays[i];
        // add a render bucket from the current
        // sorted queue if it is not empty
        if (!queue.empty()) {
            RenderBucket bucket = { queue, primitives[i] };
            args.render_buckets.push_back(bucket);
        }
    }
}

void Renderer::flush_buckets(
    RenderArgs& args
) const {
    // create a temporary hitrecord to compare
    // to the current best
    HitRecord tmp;
    // process all render buckets
    for (RenderBucket& bucket : args.render_buckets) {
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
    // clear render buckets
    args.render_buckets.clear();
}

void Renderer::build_secondary_rays(
    RenderArgs& args
) const {
    // compute all colors
    // and build all scatter rays
    for (size_t i = 0; i < args.buffer_length; i++) {
        // get the current contribution info
        RayContrib* contrib = args.contrib_buffer + i;
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
                args.rays.push_back(scatter);
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

void Renderer::render(FrameBuffer& fb) const 
{
    // initialize a render args object
    RenderPixelArgs args;
    args.contrib_buffer = new RayContrib[rpp];
    args.buffer_length = rpp;
    args.sorted_rays = std::vector<RayQueue>(bvh.num_leafs());
    // compute the width and height of the viewport
    // to easily build the primary camera rays
    float vpw = 2.0f * tanf(0.5f * cam.fov());
    float vph = vpw * (float)fb.height() / (float)fb.width();    
    // render all pixels
    for (size_t i = 0; i < fb.height(); i++) {
        for (size_t j = 0; j < fb.width(); j++) {
            // add all the primary rays through
            // the current pixel to the render args
            build_pixel_rays(args, i, j, fb.width(), fb.height(), vpw, vph);
            // render the pixel and reset the args
            // to reuse them for the next pixel
            render(args);
            args.rays.clear();
            // average the color over all rays
            // that go through the current pixel
            Vec3f c = Vec3f::zeros;
            for (size_t k = 0; k < rpp; k++) {
                RayContrib& contrib = args.contrib_buffer[k];
                // reset contribution for the upcoming rays
                c = c + contrib.color;
                contrib = RayContrib();
            }
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
    // free memory
    delete[] args.contrib_buffer;
}

void Renderer::render(
    RenderArgs& args
) const {
    // main rendering loop iterating until the
    // ray queue is empty or the maximum recursion
    // depth is reached
    size_t rdepth = 0;
    while ((!args.rays.empty()) && (rdepth++ < max_rdepth)) {
        // sort the rays from the ray queue
        // into render buckets
        sort_rays_into_buckets(args);
        // flush the render buckets, i.e.
        // compute all closest hit-records
        flush_buckets(args);
        // fill the queue with scatter
        // rays from the current iteration
        build_secondary_rays(args);
    }
}
