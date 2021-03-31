#include "./mesh.hpp"
#include "./primitive.hpp"
#include <fstream>

Mesh::Mesh(
    const Mesh::const_iterator& begin,
    const Mesh::const_iterator& end
) :
    std::vector<Triangle*>(begin, end)
{
}

/*
 *  Obj File Loader
 */

Mesh Mesh::load_obj(
    const char* fpath,
    const mtl::Material* mat
) {
    Mesh mesh;
    // open filestream
    std::ifstream f(fpath);
    // create vertex list        
    std::vector<Vec3f> vs;

    // vertex parser
    auto parse_vertex = [&vs, &f](void) {
        float x, y, z;
        f >> x >> y >> z;
        vs.push_back(Vec3f(x, y, z));
    };
    // face parser
    auto parse_face = [&mesh, &vs, &f, &mat](void) {
        // read first two index
        size_t i, j, k; 
        f >> i >> j;
        do {
            // read next index
            f >> k;
            // build face and add it
            // to the mesh
            mesh.push_back(new Triangle(
                vs[i-1], vs[j-1], vs[k-1], mat
            ));
            // update index
            j = k;
        } while (f.peek() != '\n');
    };

    // read file and build mesh
    char ch;
    while (f >> ch) {
        switch (ch) {
            case 'v':
                parse_vertex();
                break;
            case 'f':
                parse_face();
                break;
            default:
                break;
        }
        // skip to next line
        f.ignore(256, '\n');
    }
    // return the mesh
    return mesh;
}

/*
 *  Helpers
 */

void Mesh::extend(const std::vector<Triangle*>& other) {
    insert(begin(), other.begin(), other.end());
}

Mesh& Mesh::swap_axes(
    const size_t& i,
    const size_t& j
) {
    // swap the dimensions of the vertices of each triangle 
    // (note that Mesh is a friend of triangle)
    for (Triangle* t : *this) {
        std::swap(t->A[i], t->A[j]);
        std::swap(t->B[i], t->B[j]);
        std::swap(t->C[i], t->C[j]);
    }
    return *this;
}

Mesh& Mesh::mirror(
    const size_t& axis
) {
    // swap the dimensions of the vertices of each triangle 
    // (note that Mesh is a friend of triangle)
    for (Triangle* t : *this) {
        t->A[axis] *= -1;
        t->B[axis] *= -1;
        t->C[axis] *= -1;
    }
    return *this;
}

Mesh& Mesh::translate(const Vec3f& off) {
    // translate the vertices of each triangle 
    // (note that Mesh is a friend of triangle)
    for (Triangle* t : *this) {
        t->A = t->A + off;
        t->B = t->B + off;
        t->C = t->C + off;
    }
    return *this;
}

Mesh& Mesh::scale(const float& value) {
    // scale the vertices of each triangle
    // (note that Mesh is a friend of triangle)
    Vec3f vec_value(value);
    for (Triangle* t : *this) {
        t->A = t->A * vec_value;
        t->B = t->B * vec_value;
        t->C = t->C * vec_value;
    }
    return *this;
}

Mesh& Mesh::fit_box(
    const Vec3f& a,
    const Vec3f& b
) {
    // initial mean
    Vec3f mean = Vec3f::zeros;
    // initial low and high    
    Triangle* t = this->at(0);
    Vec3f low = t->A.min(t->B.min(t->C));
    Vec3f high = t->A.max(t->B.max(t->C));
    // compute mean, low and high over all
    // triangles in the mesh
    for (Triangle* t : *this) {
        // sum all together
        mean = mean + t->A + t->B + t->C;
        // update low and high values
        low = low.min(t->A.min(t->B.min(t->C)));
        high = high.max(t->A.max(t->B.max(t->C)));
    }
    // compute mean of mesh and box
    mean = mean / (size() * 3.0f);
    Vec3f box_mean = 0.5f * (a + b);
    // find axis of maximum span   
    Vec3f diff = high - low;
    float x = diff[0], y = diff[1], z = diff[2];
    size_t axis = (x > y)?
                ( (z > x)? 2 : 0) :
                ( (z > y)? 2 : 1) ;
    // compute scaling value such
    // that the axis if maximally
    // filled
    Vec3f box_diff = a.max(b) - a.min(b);
    float scale_val = box_diff[axis] / diff[axis];
    // apply translation and scaling to all triangles
    translate(-1.0f * mean);
    scale(scale_val);
    translate(box_mean);
    // return mesh
    return *this;
}

