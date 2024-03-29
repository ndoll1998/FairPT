#include <chrono>
#include <iostream>
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
    // set camera
    Camera cam(
        Vec3f(0.5, 0.5, 1.35) * 20,
        Vec3f(0, 0, -1),
        Vec3f(0, 1, 0)
    );
    cam.fov(40.0f);
    cam.vp_dist(1.35f * 20 + 1e-3f);

    // create all materials
    mtl::Material* light = new mtl::Light(new txr::Constant(Vec3f::ones * 3.0f));
    mtl::Material* red = new mtl::Lambertian(new txr::Constant(Vec3f(0.25f, 0.25f, 0.75f)));
    mtl::Material* blue = new mtl::Lambertian(new txr::Constant(Vec3f(0.75f, 0.25f, 0.25f)));
    mtl::Material* white = new mtl::Lambertian(new txr::Constant(Vec3f(0.75f, 0.75f, 0.75f)));
    mtl::Material* glass = new mtl::Dielectric(new txr::Constant(Vec3f(1.0f, 1.0f, 1.0f)), 1.5f);
    mtl::Material* mirror = new mtl::Metallic(new txr::Constant(Vec3f(1.0f, 1.0f, 1.0f)), 0.0f);

    // create cornell box mesh
    Mesh cornell = Mesh::CornellBox(white, red, blue, light);

    // add two boxes to the scene
    cornell.extend(Mesh::Parallelepiped(Vec3f(0.25, 0, -0.5), Vec3f(0.15, 0, -0.8), Vec3f(0.55, 0, -0.6), Vec3f(0.25, 0.6, -0.5), white));
    cornell.extend(Mesh::Parallelepiped(Vec3f(0.8, 0, -0.15), Vec3f(0.5, 0, -0.25), Vec3f(0.9, 0, -0.45), Vec3f(0.8, 0.3, -0.15), white));
    
    /*
    // load suzanne from obj file
    cornell.extend(
        Mesh::load_obj("obj/suzanne.obj", glass)
        .fit_box(
            Vec3f(0.1f, 0.1f, -0.4f),
            Vec3f(0.9f, 0.9f, -1.0f)
        )
        .scale(20.0f)
    ); 
    
    // load lucy from obj file
    cornell.extend(
        Mesh::load_obj("obj/lucy.obj", white)
        .swap_axes(1, 2)
        .flip_normals()
        .fit_box(
            Vec3f(0.1f, 0.0f, -0.4f),
            Vec3f(0.9f, 0.9f, -1.0f)
        )
    );
    */
    
    // scale up
    cornell.scale(20.0f);
    // check the number of triangles
    cout << "#Triangles: " << cornell.size() << endl;
    
    // convert mesh to boundable list to easily add
    // primitives other than triangles (e.g. spheres)
    BoundableList objects;
    objects.insert(objects.begin(), cornell.begin(), cornell.end());
    // insert sphere
    objects.push_back(new Sphere(Vec3f(0.7, 0.45, -0.3) * 20, 0.15 * 20, glass));
    objects.push_back(new Sphere(Vec3f(0.3, 0.15, -0.3) * 20, 0.15 * 20, mirror));

    // build scene and renderer
    Scene scene(objects);
    Renderer renderer(scene, cam, 32, 10);
    FrameBuffer fb(200, 200);

    cout << "Rendering... " << flush;
    // render the scene
    auto start = chrono::steady_clock::now();
    renderer.render(fb);
    auto stop = chrono::steady_clock::now();
    cout << chrono::duration_cast<chrono::milliseconds>(stop - start).count() / 1000.0f << "s" 
         << endl;
    // save the rendered image to disk 
    fb.save_to_bmp("/mnt/c/users/Nicla/OneDrive/Bilder/cornell.bmp");
}

