#ifndef H_FRAMEBUFFER
#define H_FRAMEBUFFER

#include <cstddef>

class FrameBuffer {
private:
    // layout and data
    const size_t _width, _height;
    unsigned char* data;
public:
    // constructor and destructor
    FrameBuffer(const size_t& width, const size_t& height); 
    ~FrameBuffer(void);
    // manipulation
    void set_pixel(
        const size_t& i, 
        const size_t& j,
        const unsigned char& r, 
        const unsigned char& g, 
        const unsigned char& b
    );
    // getters
    const size_t& width(void) const { return _width; }
    const size_t& height(void) const { return _height; }
    // save to file
    int save_to_bmp(const char* fname) const;
};

#endif // H_FRAMEBUFFER
