#ifndef H_MESH
#define H_MESH

// forward declarations
class Triangle;
// includes
#include <vector>
#include "./vec.hpp"
#include "./material.hpp"

class Mesh : public std::vector<Triangle*> {
public:
    // constructor
    Mesh(void) = default;
    Mesh(
        const Mesh::const_iterator& begin,
        const Mesh::const_iterator& end
    );
    // initializers
    static Mesh load_obj(
        const char* fpath,
        const mtl::Material* mat
    );
    static Mesh CornellBox(
        const mtl::Material* base_mtl,
        const mtl::Material* left_mtl,
        const mtl::Material* right_mtl,
        const mtl::Material* light_mtl
    );
    static Mesh Parallelogram(
        const Vec3f& A,
        const Vec3f& B,
        const Vec3f& C,
        const mtl::Material* mat
    );
    static Mesh Parallelepiped(
        const Vec3f& A,
        const Vec3f& B,
        const Vec3f& C,
        const Vec3f& D,
        const mtl::Material* mat
    );
    // some helpers
    void extend(const std::vector<Triangle*>& other);
    Mesh& swap_axes(const size_t& i, const size_t& j);
    Mesh& flip_normals(void);
    Mesh& mirror(const size_t& axis);
    Mesh& translate(const Vec3f& off);
    Mesh& scale(const float& value);
    Mesh& fit_box(
        const Vec3f& a,
        const Vec3f& b
    );
};

#endif // H_MESH
