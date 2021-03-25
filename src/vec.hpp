#ifndef H_VEC3F
#define H_VEC3F

#include <ostream>
#include <nmmintrin.h>

/*
 *  4-dimensional SIMD Vector
 */

class Vec4f 
{
private:
    union {
        __m128 m_value; // simd memory
        float items[4]; // easy access
    };
public:
    // constructors
    Vec4f(void) = default;
    inline Vec4f(const __m128& p) : m_value(p) {}
    inline Vec4f(const float& val) : m_value(_mm_set1_ps(val)) {}
    inline Vec4f(
        const float& x,
        const float& y,
        const float& z,
        const float& w
    ) : m_value(_mm_set_ps(w, z, y, x)) {}
    // converter and other operators
    inline operator __m128(void) const { return m_value; }
    inline Vec4f& operator=(const __m128& p) { m_value = p; return *this; }
    // easy access helpers
    inline float& operator[](const size_t& i) { return items[i]; }
    inline const float& operator[](const size_t& i) const { return items[i]; }
    // arithmetic members
    inline Vec4f sqrt(void) const { return _mm_sqrt_ps(*this); }
    inline Vec4f dot(const Vec4f& other) const { return _mm_dp_ps(*this, other, 0xff); }
    inline Vec4f sum(void) const {
        __m128 tmp = _mm_hadd_ps(*this, *this);
        return _mm_hadd_ps(tmp, tmp);
    }
    // cross product of the first three entries
    // but ignoring the last one
    inline Vec4f cross(const Vec4f& other) const {
        __m128 tmp0 = _mm_shuffle_ps(other, other, _MM_SHUFFLE(3, 0, 2, 1));
        __m128 tmp1 = _mm_shuffle_ps(*this, *this, _MM_SHUFFLE(3, 0, 2, 1));
        tmp0 = _mm_mul_ps(tmp0, *this);
        tmp1 = _mm_mul_ps(tmp1, other);
        __m128 tmp2 = _mm_sub_ps(tmp0, tmp1);
        return _mm_shuffle_ps(tmp2, tmp2, _MM_SHUFFLE(3, 0, 2, 1));
    }
    // norm
    inline Vec4f sq_norm(void) const { return this->dot(*this); }
    inline Vec4f norm(void) const { return this->sq_norm().sqrt(); }
    inline Vec4f normalize(void) const { return _mm_div_ps(*this, this->norm()); }
    // common vectors
    static const Vec4f zeros;
    static const Vec4f ones;
};

// arithmetic operators
inline Vec4f operator+(const Vec4f& a, const Vec4f& b) { return _mm_add_ps(a, b); } 
inline Vec4f operator-(const Vec4f& a, const Vec4f& b) { return _mm_sub_ps(a, b); } 
inline Vec4f operator*(const Vec4f& a, const Vec4f& b) { return _mm_mul_ps(a, b); } 
inline Vec4f operator/(const Vec4f& a, const Vec4f& b) { return _mm_div_ps(a, b); } 
inline Vec4f operator+(const Vec4f& a, const float& b) { return _mm_add_ps(a, _mm_set1_ps(b)); }
inline Vec4f operator-(const Vec4f& a, const float& b) { return _mm_sub_ps(a, _mm_set1_ps(b)); }
inline Vec4f operator*(const Vec4f& a, const float& b) { return _mm_mul_ps(a, _mm_set1_ps(b)); }
inline Vec4f operator/(const Vec4f& a, const float& b) { return _mm_div_ps(a, _mm_set1_ps(b)); }
inline Vec4f operator+(const float& a, const Vec4f& b) { return _mm_add_ps(_mm_set1_ps(a), b); }
inline Vec4f operator-(const float& a, const Vec4f& b) { return _mm_sub_ps(_mm_set1_ps(a), b); }
inline Vec4f operator*(const float& a, const Vec4f& b) { return _mm_mul_ps(_mm_set1_ps(a), b); }
inline Vec4f operator/(const float& a, const Vec4f& b) { return _mm_div_ps(_mm_set1_ps(a), b); }
// print operator
inline std::ostream& operator<<(std::ostream& os, const Vec4f& v) {
    return os << "Vec4f(" << v[0] << ", " << v[1] << ", " << v[2] << ", " << v[3] << ")";
}


/*
 *  3-dimensional SIMD Vector
 */

class Vec3f : public Vec4f 
{
public:
    // constructors
    Vec3f(void) = default;
    inline Vec3f(const __m128& p) : Vec4f(p) { (*this)[3] = 0.0f; }
    inline Vec3f(const Vec4f& p) : Vec4f(p) {}
    inline Vec3f(
        const float& x,
        const float& y,
        const float& z
    ) : Vec4f(x, y, z, 0.0f) {}
};

// override print operator
inline std::ostream& operator<<(std::ostream& os, const Vec3f& v) {
    return os << "Vec3f(" << v[0] << ", " << v[1] << ", " << v[2] << ")";
}

#endif // H_VEC3F
