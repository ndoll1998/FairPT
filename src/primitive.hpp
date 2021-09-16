#ifndef H_PRIMITIVE
#define H_PRIMITIVE

// forward declarations
struct Ray;
struct Ray4;
class Mesh;
class TriangleCollection;
class SphereCollection;
// includes
#include <array>
#include <vector>
#include "./vec.hpp"
#include "./bvh.hpp"
#include "./material.hpp"

// hit record storing infromation
// about the intersection of a ray
// with a primitive
typedef struct HitRecord {
    float t;    // distance to intersection point
    Vec3f p;    // point of intersection
    Vec3f n;    // surface normal at intersection
    Vec3f v;    // direction of incident ray
    bool is_valid = false;      // is the hit valid
    const mtl::Material* mat;   // surface material
} HitRecord;


/*
 *  Primitive
 */

// abstract class defining the interface
// a primitive must follow
class Primitive {
public:
    // cast a given ray to the primitive
    // and receive a hitrecord
    virtual bool cast(
        const Ray& ray,
        HitRecord& record
    ) const = 0;
    // virtual destructor
    virtual ~Primitive(void) = default;
};


// abstract primitive class handeling a collection
// of the same primitive type (e.g. triangle, sphere)
class PrimitiveCollection : public Primitive {
private:
    // cast a ray against a packet of
    // primitives in the collection
    virtual Vec4f cast_ray_packet(
        const Ray4& ray,    // packet of the same ray
        const size_t& i     // index of the primitive packet
    ) const = 0;
    // get the normal of a primitive at
    // the given point on its surface
    virtual Vec3f get_normal(
        const size_t& i,    // index of the primitive
        const Vec3f& p      // point on surface
    ) const = 0;
    // get the material of a primitive
    virtual const mtl::Material* get_material(const size_t& i) const = 0;
public:
    // override cast function to process
    // multiple triangles at once using
    // simd instructions
    bool cast(
        const Ray& ray,
        HitRecord& record
    ) const;
    // total number of primitive packets
    // currently stored in the collection
    virtual size_t n_packets(void) const = 0;
    // total number of primtives stored
    virtual size_t n_primitives(void) const = 0; 
};

// shortcut for a list of primitives
class PrimitiveList : 
    public std::vector<Primitive*>, 
    public Primitive
{
public:
    // cast ray against all primitives in
    // the list and return the closest hit
    bool cast(
        const Ray& ray,
        HitRecord& record
    ) const;
};

/*
 *  Triangle
 */

// triangle class implementing a boundable
// object but not a primitive (rendering
// single triangles is not supported)
class Triangle : public Boundable {
private:
    // save corner points and material
    Vec3f A, B, C;
    const mtl::Material* mtl;
public:
    // constructor
    Triangle(
        const Vec3f& A, // corner points in
        const Vec3f& B, // counter-clockwise
        const Vec3f& C, // order
        const mtl::Material* mtl
    );
    // build bounding box completly
    // containing the triangle
    virtual AABB bound(void) const;
    // allow triangle collection and mesh to
    // access private members of a triangle
    friend TriangleCollection;
    friend Mesh;
};

// combine a number of triangles into a
// single primitive for more efficient
// memory usage and computations
class TriangleCollection : public PrimitiveCollection {
private:
    // the data of all the triangle packets
    // separated into the single components
    std::vector<std::array<Vec4f, 3>> As, Us, Vs;
    // the normal vectors and materials
    // of all triangle packet
    std::vector<Vec3f> Ns;
    std::vector<const mtl::Material*> mtls;
    // the size of the collection
    size_t n_triangles = 0;
    // function to cast a ray to a single
    // packet of traingles
    Vec4f cast_ray_packet(
        const Ray4& ray,
        const size_t& i
    ) const;
    // get the normal of a primitive at
    // the given point on its surface
    Vec3f get_normal(
        const size_t& i,    // index of the primitive
        const Vec3f& p      // point on surface
    ) const;
    // get the material of a primitive
    const mtl::Material* get_material(const size_t& i) const;
public:
    // constructors
    TriangleCollection(void) = default;
    TriangleCollection(
        const std::vector<Triangle>::const_iterator& begin,
        const std::vector<Triangle>::const_iterator& end
    );
    // add a triangle to the collection
    // by pushing it into a packet
    void push_back(const Triangle& T);
    // total number of primitive packets
    // currently stored in the collection
    size_t n_packets(void) const;
    // total number of primtives stored
    size_t n_primitives(void) const; 
};


/*
 *  Sphere
 */

// sphere class implementing a boundable
// but not a primitive
class Sphere : public Boundable {
private:
    // center position and radius
    Vec3f center;
    float radius;
    // material
    const mtl::Material* mtl;
public:
    // constructor
    Sphere(
        const Vec3f& center,
        const float& radius,
        const mtl::Material* mtl
    );
    // build bounding box containing the sphere
    virtual AABB bound(void) const;
    // allow sphere collection to access
    // private members
    friend SphereCollection;
};

// combine a number of spheres into a single
// primitive
class SphereCollection : public PrimitiveCollection {
private:
    // data of all the sphere packets
    // separated into single components
    std::vector<std::array<Vec4f, 3>> centers;
    std::vector<Vec4f> radii;
    std::vector<const mtl::Material*> mtls;
    // size of the collection
    size_t n_spheres = 0;
    // function to cast a ray to a single
    // packer of spheres
    Vec4f cast_ray_packet(
        const Ray4& ray,
        const size_t& i
    ) const;
    // get the normal of a primitive at
    // the given point on its surface
    Vec3f get_normal(
        const size_t& i,    // index of the primitive
        const Vec3f& p      // point on surface
    ) const;
    // get the material of a primitive
    const mtl::Material* get_material(const size_t& i) const;
public:
    // constructors
    SphereCollection(void) = default;
    // add sphere to collection
    void push_back(const Sphere& S);
    // total number of primitive packets
    // currently stored in the collection
    size_t n_packets(void) const;
    // total number of primtives stored
    size_t n_primitives(void) const; 
};

#endif // H_PRIMITIVE
