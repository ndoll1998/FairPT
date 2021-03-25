#ifndef H_TEXTURE
#define H_TEXTURE

// includes
#include "./vec.hpp"

namespace txr {

class Texture {
public:
    // abstract function to get the 
    // color value of the texture at
    // a specific point in space
    virtual Vec3f color(const Vec3f& p) const = 0;
};

// Texture with constant color 
// value at any point 
class Constant : public Texture {
private:
    // the color value of
    // the texture
    Vec3f c;
public:
    // constructor
    Constant(const Vec3f& color);
    // get the constant color value
    virtual Vec3f color(const Vec3f& p) const;
};

};

#endif // H_TEXTURE


