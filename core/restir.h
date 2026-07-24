// Unified Reservoir Cascades framework (§3–§4).
//
// Modes:
//   Degenerate — bin-center rays, value-passing β-weighted merge, no reuse.
//                Must reproduce vanilla.cpp bit-exactly (§3.6 regression).
//   Full       — jittered candidates, merge-as-RIS with β in the *proposal*
//                (§3.3), reconnection shift with 2D Jacobian r_q/r_p (§3.4),
//                per-level W collapse (§3.5), optional temporal reuse.
#pragma once
#include "cascade.h"
#include "image.h"
#include "scene.h"
#include <functional>

namespace rc {

// StochasticRC: jittered candidates + value-passing merge, no reservoirs, no
// reuse — the equal-ray fair baseline ("jittered vanilla"). Its remaining
// error is the structural interpolation/factorization bias; comparing against
// Full at equal frames isolates the merge-as-RIS gain (GO-4 attribution).
// VanillaFix: vanilla RC + the community "bilinear fix" — deterministic
// value-passing merge, but each parent's consulted bin is chosen by
// reprojecting each sub-direction through the characteristic far point
// t_rep = sqrt(t_{n+1}·t_{n+2}) (the same reprojection Full uses for its
// RIS lookup). The community's standard mitigation, as a baseline.
enum class RCMode { Degenerate, Full, StochasticRC, VanillaFix };

struct RParams {
    int frames = 64;        // Full: frames rendered (temporal chain or i.i.d.)
    uint64_t seed = 1;
    bool temporal = true;   // temporal reuse with confidence cap M_n
    double lambda = 0.05;   // defensive mixing: p̂ = lum(c) + λ (§4 dark-region starvation)
    int mcap0 = 8;          // M_n = min(mcap0·2^n, mcapMax)
    int mcapMax = 64;
    double rho = 0.0;       // reconnection validation probability (§3.4); M1: 0
    // Validate only at level 0 (final consumer). Rationale: each level's tail
    // already embeds the ancestor chain's visibility (blocked chains resolve
    // to c=0), so per-level validation MULTIPLIES visibility factors —
    // penumbra scales like (1-f)^depth. One full-path check at the receiver
    // adds exactly one factor. (Empirical finding, 2026-07-22; see
    // docs/experiments.md — candidate theory note for the paper.)
    bool rhoLevel0Only = false;
    // Per-frame log-uniform scaling of t0 by 4^±0.5 (boundaries sweep the
    // full inter-level gap). Converts the seed-stable interval-boundary bias
    // (experiments E4: ±30% bands at level handoffs) into frame-averaged
    // variance. Applies to Full and StochasticRC; never to Degenerate.
    bool boundaryJitter = true;
    // Frames per split block. Within a block the split is fixed (the cascade
    // needs one consistent decomposition for tails to tile the ray); at block
    // switches temporal history is dropped — stored radiance embeds the old
    // split, and mixing splits double-counts interval overlaps (measured 3×
    // leak inflation with per-frame jitter + temporal). Frame-indexed ⇒
    // data-independent ⇒ unbiased. Bounds effective temporal reuse to the
    // block length.
    int bjitterBlock = 8;
    // Windowed parent-bin lookup (Prop W', conditional-cell form): the tail
    // stays LOCAL to the candidate direction ω — a stored sample is accepted
    // only if its reconnected direction lies in ω's level-(n+1) cell
    // (reconnection is parallax-free; parallax lives in the parent-side bin
    // index) — and the window {bp−w..bp+w} per parent exists solely to
    // recover content whose parent-side index is shifted by depth parallax.
    // Per-sample coverage-balance MIS:
    //   m = β_q / Σ_r β_r·1[sample's back-projected bin ∈ r's window].
    //   window < 0 : legacy single-bin reprojected lookup (default; the
    //                cheap approximation, coverage residual measured E3/E4);
    //   window ≥ 0 : fixed radius w at every level (calibration mode,
    //                UNCERTIFIED);
    //   windowAuto : per-level CERTIFIED width from Lemma M
    //                (CascadeCfg::coverageWindow — closed-form sound bound
    //                2 + D1 + max(D2,D3), full-ring escalation at paraxial
    //                breakdown t₁ ≤ d), computed from the jittered geometry;
    //                overrides `window`. Regression: `rc coverage` oracle.
    // Validation (ρ>0) applies at the per-parent winner as before — the
    // narrowed theorem covers windowed + single-frame + ρ=0; ρ>0 windowed is
    // the same empirical extension as single-bin.
    int window = -1;
    bool windowAuto = false;
    // Image = average of gathers over frames [burnIn, frames);
    // burnIn = frames/2 when temporal, 0 otherwise.
};

// Records lum(c)·W per reservoir entry per frame for the first maxEntries
// entries of each level — feeds temporal autocorrelation (risk #1 metric).
struct TemporalProbe {
    int maxEntries = 512;
    std::vector<std::vector<std::vector<double>>> series; // [level][entry][frame]
};

// Ray-cost accounting: candidate rays are one per reservoir per frame in
// every mode (vanilla, stochastic, full); validation shadow rays are the
// only extra cost of ρ > 0. Feeds the paper's equal-cost statement.
//
// candLen/validLen accumulate the *traced distance* (pixels) of each ray —
// the span actually marched: distance-to-hit for a hit, the searched extent
// for a miss. Equal ray *count* (cand/valid) is not equal *work*: cascade
// candidate rays are interval-bounded ([t_n, t_{n+1})), so their traced
// length shrinks with level, while the flat control's are full-length
// ([0, 4·size)). These counters turn "equal count ≠ equal work" into numbers.
struct RayCounts {
    unsigned long long cand = 0;   // Phase-1 interval traces
    unsigned long long valid = 0;  // ρ-validation shadow tests
    double candLen = 0;            // Σ traced distance of candidate rays (px)
    double validLen = 0;           // Σ traced distance of validation rays (px)
    // Parent-reservoir consults per merge: 4 for the single-bin lookup,
    // 4·(2w+1) windowed — the read-amplification cost of exact coverage.
    unsigned long long reads = 0;
};

struct RenderHooks {
    // Dynamic scenes (S3): scene for frame f. Unset ⇒ static.
    std::function<Scene(int)> sceneAt;
    // Called with the gathered image of every frame (pre-averaging).
    std::function<void(int, const Image&)> onFrame;
    // Returns true when the scene changed at frame f (renderer-supplied dirty
    // flag). Temporal confidence is clamped that frame — data-INdependent,
    // so MIS stays unbiased, unlike candidate-value-triggered clamps.
    std::function<bool(int)> sceneChanged;
    TemporalProbe* probe = nullptr;
    RayCounts* rays = nullptr;
};

Image renderReservoirRC(const Scene& scene, const CascadeCfg& cfg,
                        const RParams& prm, RCMode mode,
                        const RenderHooks* hooks = nullptr);

} // namespace rc
