#include "./vec.hpp"
#include <math.h>

// initialize common vectors
const Vec4f Vec4f::zeros = Vec4f(0.0f);
const Vec4f Vec4f::ones = Vec4f(1.0f);
const Vec4f Vec4f::eps = Vec4f(1e-4);
const Vec4f Vec4f::neps = Vec4f(-1e-4);
const Vec4f Vec4f::inf = Vec4f(-logf(0));
const Vec4f Vec4f::ninf = Vec4f(logf(0));
