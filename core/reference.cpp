#include "reference.h"

namespace rc {

Image renderReference(const Scene& scene, int size, int raysPerPixel,
                      uint64_t seed) {
    Image img(size, size);
    double tmax = 4.0 * size;
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            Vec2 p{x + 0.5, y + 0.5};
            int which;
            if (scene.sdf(p, &which) < 0) {
                img.at(x, y) = scene.shapes[which].emission;
                continue;
            }
            PCG32 rng(hashCombine(seed, (uint64_t)y * size + x), 3);
            Vec3 acc;
            for (int k = 0; k < raysPerPixel; k++) {
                double th = TWO_PI * (k + rng.uniform()) / raysPerPixel;
                Vec2 d{std::cos(th), std::sin(th)};
                Hit h;
                if (scene.intersect(p, d, 0.0, tmax, h))
                    acc += scene.shapes[h.shape].emission;
            }
            img.at(x, y) = acc * (1.0 / raysPerPixel);
        }
    }
    return img;
}

} // namespace rc
