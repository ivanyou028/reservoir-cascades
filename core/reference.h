// Ground-truth renderer: per-pixel stratified integration over the full circle
// of directions with analytic full-length rays (direct emission estimand, §5.3).
#pragma once
#include "image.h"
#include "scene.h"

namespace rc {

Image renderReference(const Scene& scene, int size, int raysPerPixel,
                      uint64_t seed = 0x5EEDF00DULL);

} // namespace rc
