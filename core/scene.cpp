#include "scene.h"
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <memory>
#include <stdexcept>

namespace rc {

// ---------- analytic intersections ----------

static bool rayCircle(Vec2 o, Vec2 d, Vec2 c, double r,
                      double tmin, double tmax, double& tHit) {
    Vec2 oc = o - c;
    double b = oc.dot(d);
    double q = oc.dot(oc) - r * r;
    double disc = b * b - q;
    if (disc < 0) return false;
    double s = std::sqrt(disc);
    double t0 = -b - s, t1 = -b + s;
    if (t0 >= tmin && t0 < tmax) { tHit = t0; return true; }
    if (t1 >= tmin && t1 < tmax) { tHit = t1; return true; }
    return false;
}

static bool rayBox(Vec2 o, Vec2 d, const Shape& s,
                   double tmin, double tmax, double& tHit) {
    // transform ray into box frame
    double ca = std::cos(-s.angle), sa = std::sin(-s.angle);
    Vec2 ro = o - s.center;
    Vec2 lo{ca * ro.x - sa * ro.y, sa * ro.x + ca * ro.y};
    Vec2 ld{ca * d.x - sa * d.y, sa * d.x + ca * d.y};
    double t0 = tmin, t1 = tmax;
    for (int ax = 0; ax < 2; ax++) {
        double loA = ax ? lo.y : lo.x, ldA = ax ? ld.y : ld.x;
        double h = ax ? s.half.y : s.half.x;
        if (std::fabs(ldA) < 1e-12) {
            if (loA < -h || loA > h) return false;
        } else {
            double inv = 1.0 / ldA;
            double ta = (-h - loA) * inv, tb = (h - loA) * inv;
            if (ta > tb) std::swap(ta, tb);
            if (ta > t0) t0 = ta;
            if (tb < t1) t1 = tb;
            if (t0 > t1) return false;
        }
    }
    // t0 is the entry point (or tmin if we start inside)
    if (t0 >= tmin && t0 < tmax) { tHit = t0; return true; }
    return false;
}

bool Scene::intersect(Vec2 o, Vec2 d, double tmin, double tmax, Hit& out) const {
    bool found = false;
    double best = tmax;
    for (size_t i = 0; i < shapes.size(); i++) {
        double t;
        bool h = shapes[i].type == ShapeType::Circle
                     ? rayCircle(o, d, shapes[i].center, shapes[i].radius, tmin, best, t)
                     : rayBox(o, d, shapes[i], tmin, best, t);
        if (h) { found = true; best = t; out.t = t; out.shape = (int)i; }
    }
    if (found) out.pos = o + d * out.t;
    return found;
}

double Scene::sdf(Vec2 p, int* which) const {
    double best = 1e30;
    int bi = -1;
    for (size_t i = 0; i < shapes.size(); i++) {
        const Shape& s = shapes[i];
        double d;
        if (s.type == ShapeType::Circle) {
            d = (p - s.center).len() - s.radius;
        } else {
            double ca = std::cos(-s.angle), sa = std::sin(-s.angle);
            Vec2 rp = p - s.center;
            Vec2 lp{ca * rp.x - sa * rp.y, sa * rp.x + ca * rp.y};
            Vec2 q{std::fabs(lp.x) - s.half.x, std::fabs(lp.y) - s.half.y};
            Vec2 qc{std::fmax(q.x, 0.0), std::fmax(q.y, 0.0)};
            d = qc.len() + std::fmin(std::fmax(q.x, q.y), 0.0);
        }
        if (d < best) { best = d; bi = (int)i; }
    }
    if (which) *which = bi;
    return best;
}

// ---------- minimal JSON ----------
// Supports: objects, arrays, numbers, strings. Enough for scene files.

namespace mj {
struct Val;
using ValPtr = std::shared_ptr<Val>;
struct Val {
    enum { NUM, STR, ARR, OBJ } kind = NUM;
    double num = 0;
    std::string str;
    std::vector<ValPtr> arr;
    std::map<std::string, ValPtr> obj;
};

struct Parser {
    const char* p;
    explicit Parser(const char* s) : p(s) {}
    void ws() { while (*p && std::isspace((unsigned char)*p)) p++; }
    [[noreturn]] void fail(const char* msg) {
        throw std::runtime_error(std::string("JSON parse error: ") + msg);
    }
    ValPtr parse() { ws(); return value(); }
    ValPtr value() {
        ws();
        if (*p == '{') return object();
        if (*p == '[') return array();
        if (*p == '"') return string();
        return number();
    }
    ValPtr object() {
        auto v = std::make_shared<Val>(); v->kind = Val::OBJ;
        p++; ws();
        if (*p == '}') { p++; return v; }
        while (true) {
            ws();
            if (*p != '"') fail("expected key");
            auto k = string();
            ws();
            if (*p != ':') fail("expected ':'");
            p++;
            v->obj[k->str] = value();
            ws();
            if (*p == ',') { p++; continue; }
            if (*p == '}') { p++; return v; }
            fail("expected ',' or '}'");
        }
    }
    ValPtr array() {
        auto v = std::make_shared<Val>(); v->kind = Val::ARR;
        p++; ws();
        if (*p == ']') { p++; return v; }
        while (true) {
            v->arr.push_back(value());
            ws();
            if (*p == ',') { p++; continue; }
            if (*p == ']') { p++; return v; }
            fail("expected ',' or ']'");
        }
    }
    ValPtr string() {
        auto v = std::make_shared<Val>(); v->kind = Val::STR;
        p++;
        while (*p && *p != '"') {
            if (*p == '\\') p++;
            v->str += *p++;
        }
        if (*p != '"') fail("unterminated string");
        p++;
        return v;
    }
    ValPtr number() {
        auto v = std::make_shared<Val>(); v->kind = Val::NUM;
        char* end;
        v->num = std::strtod(p, &end);
        if (end == p) fail("expected number");
        p = end;
        return v;
    }
};
} // namespace mj

static Vec2 asVec2(const mj::ValPtr& v, double scale) {
    return {v->arr[0]->num * scale, v->arr[1]->num * scale};
}
static Vec3 asVec3(const mj::ValPtr& v) {
    return {v->arr[0]->num, v->arr[1]->num, v->arr[2]->num};
}

Scene Scene::load(const std::string& path, double size) {
    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) throw std::runtime_error("cannot open scene: " + path);
    std::fseek(f, 0, SEEK_END);
    long n = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    std::string buf((size_t)n, 0);
    if (std::fread(&buf[0], 1, (size_t)n, f) != (size_t)n) {
        std::fclose(f);
        throw std::runtime_error("cannot read scene: " + path);
    }
    std::fclose(f);

    mj::Parser parser(buf.c_str());
    auto root = parser.parse();

    Scene sc;
    sc.size = size;
    if (root->obj.count("name")) sc.name = root->obj["name"]->str;
    for (auto& sv : root->obj["shapes"]->arr) {
        Shape s;
        auto& o = sv->obj;
        std::string ty = o["type"]->str;
        if (ty == "circle") {
            s.type = ShapeType::Circle;
            s.center = asVec2(o["center"], size);
            s.radius = o["radius"]->num * size;
        } else if (ty == "box") {
            s.type = ShapeType::Box;
            s.center = asVec2(o["center"], size);
            s.half = asVec2(o["half"], size);
            if (o.count("angle")) s.angle = o["angle"]->num;
        } else {
            throw std::runtime_error("unknown shape type: " + ty);
        }
        if (o.count("emission")) s.emission = asVec3(o["emission"]);
        sc.shapes.push_back(s);
    }
    if (root->obj.count("masks")) {
        for (auto& [name, mv] : root->obj["masks"]->obj) {
            MaskRect m;
            m.name = name;
            m.min = asVec2(mv->obj["min"], size);
            m.max = asVec2(mv->obj["max"], size);
            sc.masks.push_back(m);
        }
    }
    return sc;
}

} // namespace rc
