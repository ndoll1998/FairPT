# FairPT
A fairly optimized cpu-only path tracer written in pure C++.

![Cornell](img/cornell.bmp)![Suzanne](img/suzanne.bmp)![Lucy](img/lucy.bmp)

## Fairly Optimized

This implementation is by no means the fasted path tracer out there. It mainly serves an educational purpose. Nevertheless some effort was made to reduce the rendering time. In the following we list the optimization technics.

- ### Iterative Ray Casting
  Most path tracers are implemented in a recursive facion, i.e. one creates a primary ray, cast it to the scene and immediately recurses on the scatter ray created from it. Alternative to this simple approach we first generate multiple primary rays and cast all of them towards the scene. Then all secondary rays are created and casted in the next iteration. 

- ### Bounding Volume Hierarchy (BVH)
  The bounding volume hierarchy is a well known acceleration structure. It is a rooted tree that organizes the primitives of a scene in such a way that a ray only needs to be casted to a subset of the primitives. The tree is constructed by recursively partitioning the set of primitives into k equally large subsets such that the variance of each subset is minimal. Thus each node of the tree is associated to a set of primitives. By casting a ray to the axis-aligned bounding box (AABB) that tightly contains all the primitives of a node, one can check if the ray misses the all the associated primitives. 
  
  In our implementation the tree has degree k = 4. This way a ray can be casted against all four children of a node simultaneously using SIMD instuctions.

- ### Ray Sorting
  Ray Sorting is an open research field with the target of efficiently grouping corherent rays together. Very different from that we use a simple approach to group rays. A ray is sorted into multiple buckets corresponding to leaf nodes of the BVH. Afterwards the buckets are flushed, i.e. all rays in a bucket are casted to the associated primitives. Note that we use an itertive procedure to ray casting which allows us to first sort all rays into buckets before going on. The main advantage from this is that rays are reordered in memory to achive memory coalescing for the casting routine.
  
- ### SIMD instructions (SSE4)
  We heavily use SIMD instructions to reduce the number of cpu instructions. The most straight forward way of using SIMD is to parallelize vector operations. A more involved way is to cast a ray to mulitple primitives simultaneously. Both are implemented in the casting routine.
  
- ### Multiprocessing
  The work of rendering an image is evenly distributed over all cpu-cores. This is done by splitting the full image into smaller chunks which can be processed in parallel. For simplicity we consider these chunks to be single pixels. Note that rendering only one pixel still requires multiple primary rays and thus the performance gain of iterative ray casting and ray sorting is still active. 


## Hello World
  
The API is designed to be easy to use. The following excerpt shows how to render the cornell box (from [`src/main.cpp`](src/main.cpp)):

```C++
  // set camera
  Camera cam(
      Vec3f(0.5, 0.5, 1.35),
      Vec3f(0, 0, -1),
      Vec3f(0, 1, 0)
  );
  cam.fov(40.0f);
  cam.vp_dist(1.35f + 1e-3f);

  // create all materials
  mtl::Material* red = new mtl::Lambertian(new txr::Constant(Vec3f(0.25f, 0.25f, 0.75f)));
  mtl::Material* blue = new mtl::Lambertian(new txr::Constant(Vec3f(0.75f, 0.25f, 0.25f)));
  mtl::Material* white = new mtl::Lambertian(new txr::Constant(Vec3f(0.75f, 0.75f, 0.75f)));
  mtl::Material* light = new mtl::Light(new txr::Constant(Vec3f::ones * 3.0f));

  // build the cornell box mesh
  // a mesh is a collection of triangles with some extra functionality
  Mesh cornell;
  // light
  cornell.push_back(new Triangle(Vec3f(0.2, 0.999, -0.2), Vec3f(0.8, 0.999, -0.2), Vec3f(0.2, 0.999, -0.8), light));
  cornell.push_back(new Triangle(Vec3f(0.8, 0.999, -0.8), Vec3f(0.2, 0.999, -0.8), Vec3f(0.8, 0.999, -0.2), light));
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
  // add two boxes to the scene
  cornell.extend(Mesh::Parallelepiped(Vec3f(0.25, 0, -0.5), Vec3f(0.15, 0, -0.8), Vec3f(0.55, 0, -0.6), Vec3f(0.25, 0.6, -0.5), white));
  cornell.extend(Mesh::Parallelepiped(Vec3f(0.8, 0, -0.15), Vec3f(0.5, 0, -0.25), Vec3f(0.9, 0, -0.45), Vec3f(0.8, 0.3, -0.15), white));

  // build scene and renderer
  Scene scene(cornell);
  Renderer renderer(scene, cam, 64, 10);
  FrameBuffer fb(200, 200);
  renderer.render(fb);

```
