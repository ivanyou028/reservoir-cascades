// Reservoir Cascades — core math: vectors + deterministic RNG.
// All CPU-reference math is double precision; canonical op order is part of
// the bit-exactness contract between vanilla RC and the degenerate mode (§3.6).
#pragma once
#include <cstdint>
#include <cmath>

namespace rc {

struct Vec2 {
    double x = 0, y = 0;
    Vec2() = default;
    Vec2(double x_, double y_) : x(x_), y(y_) {}
    Vec2 operator+(Vec2 o) const { return {x + o.x, y + o.y}; }
    Vec2 operator-(Vec2 o) const { return {x - o.x, y - o.y}; }
    Vec2 operator*(double s) const { return {x * s, y * s}; }
    double dot(Vec2 o) const { return x * o.x + y * o.y; }
    double len() const { return std::sqrt(x * x + y * y); }
    Vec2 normalized() const { double l = len(); return {x / l, y / l}; }
};

struct Vec3 {
    double x = 0, y = 0, z = 0;
    Vec3() = default;
    Vec3(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}
    Vec3 operator+(Vec3 o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3 operator*(double s) const { return {x * s, y * s, z * s}; }
    Vec3& operator+=(Vec3 o) { x += o.x; y += o.y; z += o.z; return *this; }
    bool operator==(Vec3 o) const { return x == o.x && y == o.y && z == o.z; }
};

// Luminance target p̂ (§3.3): plain average keeps the estimator chromaticity-
// ratio-bounded (§3.5); exact weights don't matter, only positivity.
inline double lum(Vec3 c) { return (c.x + c.y + c.z) * (1.0 / 3.0); }

constexpr double TWO_PI = 6.283185307179586476925286766559;

// splitmix64: stateless hash used to derive independent PCG streams per
// (seed, frame, level, probe, bin) so results are reproducible and order-free.
inline uint64_t splitmix64(uint64_t z) {
    z += 0x9e3779b97f4a7c15ULL;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}
inline uint64_t hashCombine(uint64_t a, uint64_t b) {
    return splitmix64(a ^ (b + 0x9e3779b97f4a7c15ULL + (a << 6) + (a >> 2)));
}

struct PCG32 {
    uint64_t state = 0x853c49e6748fea9bULL;
    uint64_t inc = 0xda3e39cb94b95bdbULL;
    explicit PCG32(uint64_t seed, uint64_t seq = 1) {
        state = 0; inc = (seq << 1u) | 1u;
        next(); state += seed; next();
    }
    uint32_t next() {
        uint64_t old = state;
        state = old * 6364136223846793005ULL + inc;
        uint32_t xorshifted = (uint32_t)(((old >> 18u) ^ old) >> 27u);
        uint32_t rot = (uint32_t)(old >> 59u);
        return (xorshifted >> rot) | (xorshifted << ((-rot) & 31u));
    }
    // uniform in [0,1)
    double uniform() { return next() * (1.0 / 4294967296.0); }
};

} // namespace rc
