#ifndef H_CAMERA
#define H_CAMERA

// forward declarations
struct Ray;
class Scene;
class FrameBuffer;
// includes
#include "./vec.hpp"

class Camera 
{
private:
    // basic camera properties
    Vec3f origin;               // position
    Vec3f view, u_dir, v_dir;   // orientation
    float _fov = 1.05f;         // field of view (~60°)
    float _vp_dist = 0.0f;      // distance to viewport
public:
    // constructors
    Camera(void) = default;
    Camera(
        const Vec3f& origin,
        const Vec3f& view,
        const Vec3f& up
    );
    static Camera LookAt(
        const Vec3f& origin,
        const Vec3f& target,
        const Vec3f& up
    );
    // getters & setters
    const float& fov(void) const;
    const float& vp_dist(void) const;
    void fov(const float& new_fov);
    void vp_dist(const float& new_vp_dist);
    // build camera ray
    Ray build_ray_from_uv(
        const float& u,
        const float& v
    ) const;
};

#endif // H_CAMERA
