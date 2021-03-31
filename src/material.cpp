#include "./material.hpp"
#include "./primitive.hpp"
#include "./ray.hpp"
#include <rng.hpp>
#include <math.h>

// use material namespace
using namespace mtl;

// helper function returning a random 
// unit vector sampled unitformly from 
// the surface of a unit hemisphere
Vec3f rand_unit_vec(void) {
    float z = rng::randf() * 2.0f - 1.0f;
    float a = rng::randf() * 2.0f * M_PI;
    float r = sqrtf(1.0f - z * z);
    return Vec3f(r * cosf(a), r * sinf(a), z);
}

/*
 *  Material
 */

Material::Material(
    const txr::Texture* att,    // attenuation color
    const txr::Texture* emit,   // emittance color
    const float& refl,          // reflectivity
    const float& fuzz,          // fuzz of reflections
    const float& ior,           // index of refraction
    const bool& transparent     // is the material transparent
) :
    att(att), emit(emit),
    is_fuzzy(fuzz > 0.0f), fuzz(fuzz),
    refl(refl), ior(ior), 
    transparent(transparent)
{
}

bool Material::scatter(
    const HitRecord& h,
    Ray& scatter
) const {
    // check if the incident ray
    // faces in the same direction as
    // the surface normal
    Vec3f dt = h.v.dot(h.n);
    bool face_in = (dt[0] > 0.0f);
    // compute values needed for schlick
    // approximationa and transparent materials
    float nr = (face_in)? ior : 1.0f / ior;
    Vec3f c = (face_in)? dt * nr : -1 * dt;
    // compute reflectance probability
    // including schlick approximation
    // for transparent materials
    float refl_p = refl;
    if (transparent) { 
        // keep the maximum value
        // of both probabilities
        // TODO: verify that 1 - schlick is actually correct
        float p = 0.0f; // 1.0f - schlick(c[0], nr);
        refl_p = (refl_p > p)? refl_p : p; 
    }
    // test if the scatter ray
    // should come from reflection
    if (rng::randf() < refl_p) {
        // scatter by reflection
        scatter.direction = h.v - ((dt + dt) * h.n);
        // add randomness
        if (is_fuzzy) {
            scatter.direction = scatter.direction + fuzz * rand_unit_vec();
            scatter.direction = scatter.direction.normalize();
        }
    } else if (transparent) {
        // scatter by refraction
        // compute the outwards normal, i.e.
        // the normal that faces the other
        // side as the incident ray
        const Vec3f out_n = (face_in)? Vec3f(-1.0f * h.n) : h.n;
        // compute determinant and check
        // for total internal reflection
        Vec3f d = Vec3f::ones - (nr * nr) * (Vec3f::ones - dt * dt);
        if (d[0] > 0) {
            // refract incident ray
            dt = (face_in)? -1.0f * dt : dt;
            scatter.direction = nr * (h.v - out_n * dt) - out_n * d.sqrt(); 
        } else {
            // total internal reflection
            scatter.direction = h.v - ((dt + dt) * h.n);
        }
        // add some randomness to the direction
        if (is_fuzzy) {
            scatter.direction = scatter.direction + fuzz * rand_unit_vec();
            scatter.direction = scatter.direction.normalize();
        }
    } else {
        // scatter by hemisphere sampling
        scatter.direction = h.n + rand_unit_vec();
        scatter.direction = scatter.direction.normalize();
    }
    // origin is always the
    // intersection point
    scatter.origin = h.p;
    // always scatter
    return true;
}

Vec3f Material::attenuation(const HitRecord& h) const {
    // return the attenuation color value 
    // of the texture at the hitpoint
    // and zeros if the texture is not set
    return (att)? att->color(h.p) : Vec3f::zeros;
}

Vec3f Material::emittance(const HitRecord& h) const {
    // return the emittance color value 
    // of the texture at the hitpoint
    // and zeros if the texture is not set
    return (emit)? emit->color(h.p) : Vec3f::zeros;
}

/*
 *  Specific Materials
 */

Lambertian::Lambertian(
    const txr::Texture* att
) : 
    Material(
        att,        // attenuation
        nullptr,    // emittance
        -1.0f,      // fuzzyness
        -1.0f,      // reflectance
        1.0f,       // index of refraction
        false       // transparent
    )
{
}

Specular::Specular(
    const txr::Texture* att,
    const float& index
) :
    Material(
        att,        // attenuation
        nullptr,    // emittance
        -1.0f,      // fuzzyness
        -1.0f,      // reflectance
        index,      // index of refraction
        false       // transparent
    )
{
}

Light::Light(
    const txr::Texture* emit
) :
    Material(
        nullptr,    // attenuation
        emit,       // emittance
        0.0f,       // all of the
        0.0f,       // below are
        0.0f,       // unused in
        false       // light materials
    )
{
}

bool Light::scatter(
    const HitRecord& h,
    Ray& scatter
) const {
    // light materials do not
    // generate secondary rays
    return false;
}


/*
 *  Debugging Materials
 */

// set all material property values
// to zeros since none of them are used
// in debugging materials
Debug::Debug(void) :
    Material(
        nullptr,
        nullptr,
        0.0f,
        0.0f,
        0.0f,
        false
    )
{
}

bool Debug::scatter(const HitRecord& h, Ray& scatter) const 
{
    // debugging materials do not
    // generate secondary rays
    return false; 
}

// normal materials visualizing the surface normal
// at the intersection point as color
Vec3f Normal::emittance(const HitRecord& h) const 
{
    // return the (normalized) surface normal
    // vector as the color value
    return 0.5f * (h.n + Vec3f::ones);
}

// depth material showing the distance to the
// intersection point as gray-scale color value
Depth::Depth(
    const float& min_dist,
    const float& max_dist
) : 
    min_dist(min_dist),
    max_dist(max_dist)
{
}

Vec3f Depth::emittance(const HitRecord& h) const {
    return Vec3f((h.t - min_dist) / max_dist);
}

// cosine material returning the cosine angle
// between the incident ray and the surface normal
// as gray-scale color value
Vec3f Cosine::emittance(const HitRecord& h) const {
    return h.v.dot(h.n);
}
