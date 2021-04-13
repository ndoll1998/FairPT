#ifndef H_RNG
#define H_RNG

namespace rng {

    class rng {
    private:
        int _seed;
    public:
        rng(int seed) : _seed(seed) {}
        float randf(void) {
            union { 
                float fres; 
                unsigned int ires; 
            };
            _seed *= 16807;
            ires = ((((unsigned int)_seed)>>9 ) | 0x3f800000);
            return fres - 1.0f;
        }
    };

    namespace { 
        rng _rng = rng(42); 
        float randf(void) { return _rng.randf(); }
    }

};

#endif // H_RNG
