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
    rpp(rpp),
    max_rdepth(max_rdepth),
    ray_cache(scene.bvh().n_leafs())
{}

void Renderer::build_init_cache(
    ContribInfo* contrib_buffer,
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
                // get a reference to the
                // contribution info associated
                // with the current primary ray
                ContribInfo* c = contrib_buffer + (idx++);
                // fill a 2x2 sub-pixel grid
                // and add a noise term
                size_t pi = k / 2 % 2, pj = k % 2;                
                float su = (float)(i * 2 + pi + rng::randf()) / (2 * height) - 0.5f;
                float sv = (float)(j * 2 + pj + rng::randf()) / (2 * width) - 0.5f;
                // build the ray with origin on the viewport
                // and direction through the sub-pixel
                Ray r = cam.build_ray_from_uv(su * vph, sv * vpw);
                // get all intersecting leafs and add
                // the ray to all the corresponding buckets
                std::vector<size_t> leaf_ids;
                bvh.get_intersecting_leafs(r, leaf_ids);
                ray_cache.sort_into_buckets(r, c, leaf_ids);
            }
        }
    }
}

void Renderer::flush_cache(void) {
    for (size_t i = 0; i < bvh.n_leafs(); i++) {
        // get the i-th bucket from the cache
        // and clear it in the cache
        RayBucket& bucket = ray_cache.get_bucket(i);
        // check if the bucket is already empty
        if (bucket.empty()) { continue; }
        // get the list of primitives
        // corresponding to the current bucket
        const PrimitiveList& prim_list = bvh.leaf_primitives(i);
        // cast all rays of the bucket to the
        // list of primitives
        while (!bucket.empty()) {
            // get the current ray-contrib pair
            RayContribPacket packet = bucket.pop_packet();
            // cast the packet against all the
            // primitives in the list
            HitRecord4 record_packet;
            prim_list.cast_packet(packet.rays, record_packet, packet.n_valids);
            // split the packet of hitrecords into
            // single records for easier processing
            HitRecord records[4];
            record_packet.split(records);
            // update all the hitrecords 
            for (size_t k = 0; k < packet.n_valids; k++) {
                // get a reference to the contribution
                // info and the new hitrecord of the current ray
                ContribInfo* contrib = packet.contribs[k];
                HitRecord& h = records[k];
                // update the hitrecord
                if (contrib->hit_record.valid) {
                    // in case both hitrecords are valid
                    // keep the one with closer distance
                    contrib->hit_record = (contrib->hit_record.t < h.t)? contrib->hit_record : h;
                } else {
                    // otherwise keep the one that is valid
                    contrib->hit_record = h; 
                }
            }
        }
    }

}

void Renderer::build_next_cache(
    ContribInfo* contrib_buffer,
    const size_t& buffer_length
) {
    // compute all colors
    // and build all scatter rays
    for (size_t i = 0; i < buffer_length; i++) {
        // get the current contribution info
        ContribInfo* contrib = contrib_buffer + i;
        HitRecord& h = contrib->hit_record;
        // check if the hit record is valid, i.e.
        // if the corresponding ray hit anything
        if (h.valid) {
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
                h.valid = false; 
                // sort the scatter ray
                // into the correct buckets
                std::vector<size_t> leaf_ids;
                bvh.get_intersecting_leafs(scatter, leaf_ids);
                ray_cache.sort_into_buckets(scatter, contrib, leaf_ids);
            }
        } else {
            // the ray corresponding to the
            // contribution info did not hit
            // any primitive
            // contrib.color = contrib.color + contrib.albedo;
        }
    }
}

void Renderer::render(FrameBuffer& fb) {
    // create an array that stores
    // all contribution infos of 
    // the primary rays (secondary 
    // rays also re-use these items)
    size_t n_primary_rays = fb.width() * fb.height() * rpp;
    ContribInfo* contrib_buffer = new ContribInfo[n_primary_rays];
    // build the initial rays
    // and sort them into buckets
    build_init_cache(contrib_buffer, fb.width(), fb.height());
    // main rendering loop iterating
    // until the ray cache is empty
    // or the maximum recursion depth
    // is reached
    size_t rdepth = 0;
    while ((!ray_cache.empty()) && (rdepth++ < max_rdepth)) {
        // flush the ray cache to end
        // up with all closest hit-records
        flush_cache();
        // fill the cache with all scatter
        // rays from the current iteration
        build_next_cache(contrib_buffer, n_primary_rays);
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
