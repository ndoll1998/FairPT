#include <chrono>
#include <iostream>
#include "./vec.hpp"
#include "./camera.hpp"
#include "./scene.hpp"
#include "./primitive.hpp"
#include "./texture.hpp"
#include "./material.hpp"
#include "./renderer.hpp"
#include "./framebuffer.hpp"

using namespace std;

int main(void) {

    Camera cam(
        Vec3f(0.5, 0.5, 0.8),
        Vec3f(0, 0, -1),
        Vec3f(0, 1, 0)
    );
    cam.fov(45);
    cam.vp_dist(0.8f + 1e-4f);

    // create all materials
    mtl::Material* red = new mtl::Lambertian(
        new txr::Constant(Vec3f(0.25f, 0.25f, 0.75f))
    );
    mtl::Material* blue = new mtl::Lambertian(
        new txr::Constant(Vec3f(0.75f, 0.25f, 0.25f))
    );
    mtl::Material* white = new mtl::Lambertian(
        new txr::Constant(Vec3f(0.75f, 0.75f, 0.75f))
    );
    mtl::Material* light = new mtl::Light(
        new txr::Constant(Vec3f::ones * 3.0f)
    );

    // build the scene
    PrimitiveList pl;
    // light
    pl.push_back(new Triangle(Vec3f(0.3, 0.999, -0.3), Vec3f(0.7, 0.999, -0.3), Vec3f(0.3, 0.999, -0.7), light));
    pl.push_back(new Triangle(Vec3f(0.7, 0.999, -0.7), Vec3f(0.3, 0.999, -0.7), Vec3f(0.7, 0.999, -0.3), light));
    // ceiling
    pl.push_back(new Triangle(Vec3f(0, 1, 0), Vec3f(0, 1, -1), Vec3f(1, 1, 0), white));
    pl.push_back(new Triangle(Vec3f(1, 1, -1), Vec3f(1, 1, 0), Vec3f(0, 1, -1), white));
    // floor
    pl.push_back(new Triangle(Vec3f(0, 0, 0), Vec3f(1, 0, 0), Vec3f(0, 0, -1), white));
    pl.push_back(new Triangle(Vec3f(1, 0, -1), Vec3f(0, 0, -1), Vec3f(1, 0, 0), white));
    // back
    pl.push_back(new Triangle(Vec3f(0, 0, -1), Vec3f(1, 0, -1), Vec3f(0, 1, -1), white));
    pl.push_back(new Triangle(Vec3f(1, 1, -1), Vec3f(0, 1, -1), Vec3f(1, 0, -1), white));
    // front
    pl.push_back(new Triangle(Vec3f(1, 1, 0), Vec3f(1, 0, 0), Vec3f(0, 1, 0), white));
    pl.push_back(new Triangle(Vec3f(0, 0, 0), Vec3f(0, 1, 0), Vec3f(1, 0, 0), white));
    // left
    pl.push_back(new Triangle(Vec3f(0, 0, 0), Vec3f(0, 0, -1), Vec3f(0, 1, 0), red));
    pl.push_back(new Triangle(Vec3f(0, 1, -1), Vec3f(0, 1, 0), Vec3f(0, 0, -1), red));
    // right
    pl.push_back(new Triangle(Vec3f(1, 0, 0), Vec3f(1, 1, 0), Vec3f(1, 0, -1), blue));
    pl.push_back(new Triangle(Vec3f(1, 1, -1), Vec3f(1, 0, -1), Vec3f(1, 1, 0), blue));
    
    BVH bvh(pl);
    Scene scene(bvh);

    Renderer renderer(scene, cam, 32, 10); 
    FrameBuffer fb(200, 200);
    
    auto start = chrono::steady_clock::now();
    renderer.render(fb);
    auto stop = chrono::steady_clock::now();
    cout << "Rendering time: " 
         << chrono::duration_cast<chrono::milliseconds>(stop - start).count() / 1000.0f << "s" 
         << endl;
    
    fb.save_to_bmp("/mnt/c/users/Nicla/OneDrive/Bilder/new_img.bmp");
}

