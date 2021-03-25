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
    Vec3f attenuation(const HitRecord& h) const;
    // method that returns the
    // emittance color
    Vec3f emittance(const HitRecord& h) const;
};

// lambertian material has
// perfect diffuse properties
class Lambertian : public Material {
public:
    Lambertian(const txr::Texture* att);
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

};

#endif // H_MATERIAL
