// Metrics (§6): MAPE (full image / per mask) and mask energy statistics.
#pragma once
#include "../core/image.h"
#include "../core/scene.h"
#include <string>

namespace rc {

// Mean absolute percentage error on luminance; eps floors the denominator.
// mask == nullptr ⇒ whole image.
double mape(const Image& est, const Image& ref, const MaskRect* mask,
            double eps = 1e-3);

// Mean luminance over a mask (or whole image).
double maskEnergy(const Image& img, const MaskRect* mask);

const MaskRect* findMask(const Scene& sc, const std::string& name);

} // namespace rc
