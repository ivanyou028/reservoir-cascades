// Flat world-space reservoir reuse — the non-hierarchical control (§6 "does
// the cascade earn its structure?"). Boissé-2021-style: one probe per pixel,
// B angular bins, full-length candidate rays (no interval decomposition),
// temporal reuse + disk-kernel spatial reuse with reconnection shifts.
//
// Fairness contract with the cascade runs:
//   - identical total candidate budget: B defaults to 4·levels rays/px/frame,
//     the cascade's per-pixel count (16 at 128², 20 at 256²);
//   - it receives our temporal machinery verbatim (immutable cRef
//     re-evaluation, revival, confidence cap) — not a crippled strawman;
//   - spatial reuse follows standard ReSTIR practice: k neighbors uniform in
//     a disk of radius r, confidence-weighted proposal MIS, reconnection
//     Jacobian r_q/r_p, renormalize-before-select ρ-validation (same
//     discipline as our merge; full-length shadow segments — flat has no
//     interval structure to bound them).
// What it cannot have, by construction, is a per-scale reuse radius: r and
// the confidence cap are single global dials, which is exactly the point.
#pragma once
#include "restir.h" // RayCounts
#include "scene.h"
#include "image.h"
#include <functional>

namespace rc {

struct FlatParams {
    int frames = 128;
    uint64_t seed = 1;
    int bins = 0;          // 0 ⇒ 4·levels(size): equal candidate budget
    double radius = 8;     // spatial kernel radius, pixels; 0 ⇒ temporal-only
    int k = 4;             // spatial neighbors per entry per frame
    double rho = 0;        // validation probability per considered neighbor
    double lambda = 0.05;  // same defensive target floor as the cascade
    int mcap = 8;          // temporal confidence cap (single, global — no 2^n)
    bool temporal = true;
};

Image renderFlatReuse(const Scene& scene, int size, const FlatParams& prm,
                      RayCounts* rays = nullptr,
                      std::function<void(int, const Image&)> onFrame = {});

} // namespace rc
