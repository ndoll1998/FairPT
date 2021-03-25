#include "./texture.hpp"

// use texture namespace
using namespace txr;

/*
 *  Constant Texture
 */

Constant::Constant(const Vec3f& color) : 
    c(color) {}

Vec3f Constant::color(const Vec3f& p) const {
    // return the constant color value
    // assigned to the texture
    return c;
}

