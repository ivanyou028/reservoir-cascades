// Float image buffer + PPM (tonemapped preview) / PFM (lossless) IO.
#pragma once
#include "vec.h"
#include <string>
#include <vector>

namespace rc {

struct Image {
    int w = 0, h = 0;
    std::vector<Vec3> px;
    Image() = default;
    Image(int w_, int h_) : w(w_), h(h_), px((size_t)w_ * h_) {}
    Vec3& at(int x, int y) { return px[(size_t)y * w + x]; }
    const Vec3& at(int x, int y) const { return px[(size_t)y * w + x]; }

    void writePPM(const std::string& path, double exposure = 1.0) const;
    void writePFM(const std::string& path) const;
    static bool readPFM(const std::string& path, Image& out);
};

} // namespace rc
