#include <chrono>
#include <iostream>
#include <vector>
#include "./vec.hpp"
#include "./camera.hpp"
#include "./scene.hpp"
#include "./primitive.hpp"
#include "./mesh.hpp"
#include "./texture.hpp"
#include "./material.hpp"
#include "./renderer.hpp"
#include "./framebuffer.hpp"

using namespace std;

int main(void) {

    Camera cam(
        Vec3f(0.5, 0.5, 1.35),
        Vec3f(0, 0, -1),
        Vec3f(0, 1, 0)
    );
    cam.fov(40.0f);
    cam.vp_dist(1.35f + 1e-3f);

    // create all materials
    mtl::Material* red = new mtl::Lambertian(
        new txr::Constant(Vec3f(0.25f, 0.25f, 0.75f))
    );
    mtl::Material* blue = new mtl::Lambertian(
        new txr::Constant(Vec3f(0.75f, 0.25f, 0.25f))
    );
    mtl::Material* gray = new mtl::Lambertian(
        new txr::Constant(Vec3f(0.25f, 0.25f, 0.25f))
    );
    mtl::Material* white = new mtl::Lambertian(
        new txr::Constant(Vec3f(0.75f, 0.75f, 0.75f))
    );
    mtl::Material* light = new mtl::Light(
        new txr::Constant(Vec3f::ones * 3.0f)
    );

    // build the cornell box mesh
    Mesh cornell;
    // light
    cornell.push_back(new Triangle(Vec3f(0.3, 0.999, -0.3), Vec3f(0.7, 0.999, -0.3), Vec3f(0.3, 0.999, -0.7), light));
    cornell.push_back(new Triangle(Vec3f(0.7, 0.999, -0.7), Vec3f(0.3, 0.999, -0.7), Vec3f(0.7, 0.999, -0.3), light));
    // ceiling
    cornell.push_back(new Triangle(Vec3f(0, 1, 0), Vec3f(0, 1, -1), Vec3f(1, 1, 0), white));
    cornell.push_back(new Triangle(Vec3f(1, 1, -1), Vec3f(1, 1, 0), Vec3f(0, 1, -1), white));
    // floor
    cornell.push_back(new Triangle(Vec3f(0, 0, 0), Vec3f(1, 0, 0), Vec3f(0, 0, -1), white));
    cornell.push_back(new Triangle(Vec3f(1, 0, -1), Vec3f(0, 0, -1), Vec3f(1, 0, 0), white));
    // back
    cornell.push_back(new Triangle(Vec3f(0, 0, -1), Vec3f(1, 0, -1), Vec3f(0, 1, -1), white));
    cornell.push_back(new Triangle(Vec3f(1, 1, -1), Vec3f(0, 1, -1), Vec3f(1, 0, -1), white));
    // front
    cornell.push_back(new Triangle(Vec3f(1, 1, 0), Vec3f(1, 0, 0), Vec3f(0, 1, 0), white));
    cornell.push_back(new Triangle(Vec3f(0, 0, 0), Vec3f(0, 1, 0), Vec3f(1, 0, 0), white));
    // left
    cornell.push_back(new Triangle(Vec3f(0, 0, 0), Vec3f(0, 0, -1), Vec3f(0, 1, 0), red));
    cornell.push_back(new Triangle(Vec3f(0, 1, -1), Vec3f(0, 1, 0), Vec3f(0, 0, -1), red));
    // right
    cornell.push_back(new Triangle(Vec3f(1, 0, 0), Vec3f(1, 1, 0), Vec3f(1, 0, -1), blue));
    cornell.push_back(new Triangle(Vec3f(1, 1, -1), Vec3f(1, 0, -1), Vec3f(1, 1, 0), blue));

    // load mesh from obj file
    Mesh suzanne = Mesh::load_obj("obj/suzanne.obj", gray);
    // format and add to cornell box
    cornell.extend(suzanne.fit_box(
        Vec3f(0.1f, 0.1f, -0.4f),
        Vec3f(0.9f, 0.9f, -1.0f)
    ));

    // check the number of triangles
    cout << "#Triangles: " << cornell.size() << endl;

    // build scene and renderer
    Scene scene(cornell);
    Renderer renderer(scene, cam, 64, 10); 
    FrameBuffer fb(200, 200);
 
    // render the scene
    auto start = chrono::steady_clock::now();
    renderer.render(fb);
    auto stop = chrono::steady_clock::now();
    cout << "Rendering time: " 
         << chrono::duration_cast<chrono::milliseconds>(stop - start).count() / 1000.0f << "s" 
         << endl;
    // save the rendered image to disk 
    fb.save_to_bmp("/mnt/c/users/Nicla/OneDrive/Bilder/new_img.bmp");
}

