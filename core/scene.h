// Scene: 2D shapes with analytic ray intersection.
// The CPU oracle uses closed-form intersections (exact, fast) rather than SDF
// sphere tracing; the shapes also expose an SDF for inside-tests and for
// future parity with the WebGPU sphere-tracing path.
#pragma once
#include "vec.h"
#include <string>
#include <vector>

namespace rc {

enum class ShapeType { Circle, Box };

struct Shape {
    ShapeType type;
    Vec2 center;
    double radius = 0;   // Circle
    Vec2 half;           // Box half-extents
    double angle = 0;    // Box rotation (radians)
    Vec3 emission;       // radiance emitted at the surface (and interior)
};

struct MaskRect {
    std::string name;
    Vec2 min, max; // pixel coords after load
    bool contains(double x, double y) const {
        return x >= min.x && x < max.x && y >= min.y && y < max.y;
    }
};

struct Hit {
    double t = 0;
    int shape = -1;
    Vec2 pos;
};

struct Scene {
    std::string name;
    std::vector<Shape> shapes;
    std::vector<MaskRect> masks;
    double size = 0; // pixels per world unit; shapes stored in pixel units

    // Nearest intersection with t in [tmin, tmax). Returns false if none.
    bool intersect(Vec2 o, Vec2 d, double tmin, double tmax, Hit& out) const;
    // Signed distance (pixel units); used for inside-shape tests.
    double sdf(Vec2 p, int* which = nullptr) const;

    // Load from minimal-JSON file; world coords in [0,1] are scaled by `size`.
    static Scene load(const std::string& path, double size);
};

} // namespace rc
