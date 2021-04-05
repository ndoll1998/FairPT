#ifndef H_MATERIAL
#define H_MATERIAL

// forward declarations
struct Ray;
struct HitRecord;
// includes
#include "./texture.hpp"

namespace mtl {

class Material {
private:
    // material properties
    const txr::Texture* att;
    const txr::Texture* emit;
    const bool is_fuzzy;
    const Vec3f fuzz;
    const float refl;
    const float ior;
    const bool transparent;
public:
    // constructor
    Material(
        const txr::Texture* att,    // attenuation color
        const txr::Texture* emit,   // emittance color
        const float& fuzz,          // fuzz of reflections
        const float& refl,          // reflectance probability
        const float& ior,           // index of refraction
        const bool& transparent     // is the material transparent
    );
    // build the scatter ray and return
    // false when there is no scatter ray
    virtual bool scatter(
        const HitRecord& h, // the hitrecord of the in ray
        Ray& scatter        // output scatter ray
    ) const;
    // method that returns the 
    // attenuation color of a ray
    virtual Vec3f attenuation(const HitRecord& h) const;
    // method that returns the
    // emittance color
    virtual Vec3f emittance(const HitRecord& h) const;
};

// lambertian material has
// perfect diffuse properties
class Lambertian : public Material {
public:
    Lambertian(const txr::Texture* att);
};

// specular material
class Specular : public Material {
public:
    Specular(
        const txr::Texture* att, 
        const float& index
    );
};

// specular material
class Metallic : public Material {
public:
    Metallic(
        const txr::Texture* att, 
        const float& fuzz
    );
};

// transparent material
class Dielectric : public Material {
public:
    Dielectric(
        const txr::Texture* att, 
        const float& index
    );
};

// light material
class Light : public Material {
public:
    Light(const txr::Texture* emit);
    // light material never scatter
    // thus the scatter function should
    // always return false
    virtual bool scatter(
        const HitRecord& h,
        Ray& scatter
    ) const;
};


/*
 *  Debug Materials
 */

class Debug : public Material {
public:
    // constructor
    Debug(void);
    // override scatter function
    // and make debugging materials
    // never generate secondary rays
    virtual bool scatter(
        const HitRecord& h,
        Ray& scatter
    ) const;
    // debug materials only need to
    // define an emittance function 
    virtual Vec3f emittance(const HitRecord& h) const = 0;
};

// normal material visualizing the
// surface normal
class Normal : public Debug {
public:
    // emittance color is proportional to the
    // surface normal vector at intersection
    virtual Vec3f emittance(const HitRecord& h) const;
};

// depth material showing the
// distance to the intersection
// point as gray-scale color value
class Depth : public Debug {
private:
    float min_dist, max_dist;
public:
    // constructor
    Depth(
        const float& min_dist,
        const float& max_dist
    );
    // emittance is the distance to intersection
    virtual Vec3f emittance(const HitRecord& h) const;
};

// cosine material visualizing the angle
// between incident ray and surface normal
class Cosine : public Debug {
public:
    // return cosine angle between incident ray direction
    // and surface normal as gray-scale color vector
    virtual Vec3f emittance(const HitRecord& h) const;
};

};

#endif // H_MATERIAL
