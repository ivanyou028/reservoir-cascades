#include "image.h"
#include <cstdio>
#include <cstring>

namespace rc {

void Image::writePPM(const std::string& path, double exposure) const {
    FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) return;
    std::fprintf(f, "P6\n%d %d\n255\n", w, h);
    std::vector<unsigned char> row((size_t)w * 3);
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            Vec3 c = at(x, y) * exposure;
            double v[3] = {c.x, c.y, c.z};
            for (int k = 0; k < 3; k++) {
                double t = v[k] / (1.0 + v[k]);          // Reinhard
                t = std::pow(t < 0 ? 0 : t, 1.0 / 2.2);  // gamma
                int b = (int)(t * 255.0 + 0.5);
                row[(size_t)x * 3 + k] = (unsigned char)(b < 0 ? 0 : b > 255 ? 255 : b);
            }
        }
        std::fwrite(row.data(), 1, row.size(), f);
    }
    std::fclose(f);
}

void Image::writePFM(const std::string& path) const {
    FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) return;
    std::fprintf(f, "PF\n%d %d\n-1.0\n", w, h); // negative scale = little endian
    for (int y = h - 1; y >= 0; y--) {          // PFM rows are bottom-up
        for (int x = 0; x < w; x++) {
            float v[3] = {(float)at(x, y).x, (float)at(x, y).y, (float)at(x, y).z};
            std::fwrite(v, sizeof(float), 3, f);
        }
    }
    std::fclose(f);
}

bool Image::readPFM(const std::string& path, Image& out) {
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) return false;
    char hdr[3] = {};
    int w = 0, h = 0;
    double scale = 0;
    if (std::fscanf(f, "%2s %d %d %lf", hdr, &w, &h, &scale) != 4 ||
        std::strcmp(hdr, "PF") != 0 || scale >= 0) {
        std::fclose(f);
        return false;
    }
    std::fgetc(f); // single whitespace after header
    out = Image(w, h);
    for (int y = h - 1; y >= 0; y--) {
        for (int x = 0; x < w; x++) {
            float v[3];
            if (std::fread(v, sizeof(float), 3, f) != 3) { std::fclose(f); return false; }
            out.at(x, y) = {v[0], v[1], v[2]};
        }
    }
    std::fclose(f);
    return true;
}

} // namespace rc
