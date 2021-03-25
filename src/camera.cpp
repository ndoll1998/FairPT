#include "./camera.hpp"
#include "./scene.hpp"
#include "./ray.hpp"
#include "./framebuffer.hpp"

// constructors
Camera::Camera(
    const Vec3f& origin,
    const Vec3f& view,
    const Vec3f& up
) :
    origin(origin),
    view(view.normalize()),
    u_dir(up.normalize()),
    v_dir(view.cross(up).normalize())
{}

Camera Camera::LookAt(
    const Vec3f& origin,
    const Vec3f& target,
    const Vec3f& up
) {
    // compute view and up direction
    Vec3f view = (target - origin).normalize();
    Vec3f v = up.cross(view).normalize();
    Vec3f u = view.cross(v).normalize();
    // build camera
    return Camera(origin, view, u);
}

// getters
const float& Camera::fov(void) const { return _fov; }
const float& Camera::vp_dist(void) const { return _vp_dist; }
// setters
void Camera::fov(const float& new_fov) { _fov = new_fov; }
void Camera::vp_dist(const float& new_vp_dist) { _vp_dist = new_vp_dist; }

Ray Camera::build_ray_from_uv(
    const float& u,
    const float& v
) const {
    // build ray through position (u, v) on viewport
    Vec3f pix_off = view + (v * v_dir) - (u * u_dir);
    return { origin + _vp_dist * pix_off, pix_off.normalize() };
}

