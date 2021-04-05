#ifndef H_RAY
#define H_RAY

// includes
#include <array>
#include <vector>
#include "./vec.hpp"
#include "./primitive.hpp"

// contribution information needed to
// interatively update the color of a
// primary ray when tracing a scatter ray
typedef struct RayContrib {
    Vec3f color = Vec3f::zeros;     // color after i (scatter-) rays
    Vec3f albedo = Vec3f::ones;     // color influence of the current ray
    HitRecord hit_record;           // hit-record of the currentl ray
} RayContrib;

// ray structure combining positional
// and directional information
typedef struct Ray {
    // positional and directional
    // information of the ray
    Vec3f origin;
    Vec3f direction;
    // reference to the contribution
    // info of the ray
    RayContrib* contrib;
} Ray;

// a packet of four rays usually
// holding copies of the same ray
typedef struct Ray4 {
    std::array<Vec4f, 3> origin;
    std::array<Vec4f, 3> direction;
} Ray4;

// shortcur for a vector of rays
class RayQueue : public std::vector<Ray> {};

#endif // H_RAY
