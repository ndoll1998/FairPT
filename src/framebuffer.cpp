#include "./framebuffer.hpp"

#include <fstream>
#include <stdexcept>

unsigned int ravel_index(
    const unsigned int& i,
    const unsigned int& j,
    const unsigned int& h,
    const unsigned int& w
) {
    // check out of bounds
    if ((i < 0) || (i >= h) || (j < 0) || (j >= w))
        throw std::out_of_range("Index is out of range!");
    // compute raveled index
    return (i * w + j) * 3;
}


FrameBuffer::FrameBuffer(
    const size_t& width, 
    const size_t& height
) : 
    _width(width), 
    _height(height),
    data(new unsigned char[width * height * 3])
{}


FrameBuffer::~FrameBuffer(void)
{
    // free allocated data
    delete[] data;
}

void FrameBuffer::set_pixel(
    const size_t& i,
    const size_t& j,
    const unsigned char& r,
    const unsigned char& g,
    const unsigned char& b
) {
    // get raveled index
    const unsigned int idx = ravel_index(i, j, _height, _width);
    // update values in data array
    data[idx + 0] = r;
    data[idx + 1] = g;
    data[idx + 2] = b;
}


int FrameBuffer::save_to_bmp(
    const char* fname
) const {
    // open file
    std::ofstream f;
    f.open(fname);   
    // total number of bytes 
    uint32_t w = _width;
    uint32_t h = _height;
    uint32_t t = 3 * w * h;
    // use negative height to make sure the 
    // anchor point is the top-left corner
    uint32_t head[13] = {54 + t, 0, 54, 40, w, -h, (24<<16)|1};
    // write header
    f.write("BM", 2);
    f.write((char*)head, 52);
    // write image data and close
    f.write((char*)data, t);
    f.close();
    return 0;
}
