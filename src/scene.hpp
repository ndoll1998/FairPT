#ifndef H_SCENE
#define H_SCENE

// forward declarations
class BVH;


class Scene {
private:
    const BVH& _bvh;
public:
    Scene(const BVH& bvh);
    // getter
    const BVH& bvh(void) const;
};

#endif // H_SCENE
