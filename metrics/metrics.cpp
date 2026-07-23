#include "metrics.h"

namespace rc {

double mape(const Image& est, const Image& ref, const MaskRect* mask, double eps) {
    double sum = 0;
    long count = 0;
    for (int y = 0; y < ref.h; y++)
        for (int x = 0; x < ref.w; x++) {
            if (mask && !mask->contains(x + 0.5, y + 0.5)) continue;
            double e = lum(est.at(x, y)), r = lum(ref.at(x, y));
            sum += std::fabs(e - r) / (r + eps);
            count++;
        }
    return count ? sum / count : 0;
}

double maskEnergy(const Image& img, const MaskRect* mask) {
    double sum = 0;
    long count = 0;
    for (int y = 0; y < img.h; y++)
        for (int x = 0; x < img.w; x++) {
            if (mask && !mask->contains(x + 0.5, y + 0.5)) continue;
            sum += lum(img.at(x, y));
            count++;
        }
    return count ? sum / count : 0;
}

const MaskRect* findMask(const Scene& sc, const std::string& name) {
    for (auto& m : sc.masks)
        if (m.name == name) return &m;
    return nullptr;
}

} // namespace rc
