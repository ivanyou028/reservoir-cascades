// Cascade discretization (§5.2). Shared plumbing for both the independent
// vanilla RC implementation and the unified reservoir framework — the merge
// *algorithms* stay separate; only geometry/indexing lives here.
//
// Level n (all in pixel units):
//   probe spacing  s_n = s0 · 2^n, probes at (i+0.5)·s_n
//   angular bins   B_n = B0 · 4^n, bin b covers [b, b+1)·2π/B_n
//   interval       [start_n, start_{n+1}) with start_n = t0·(4^n − 1)/3
//                  (geometric series ⇒ contiguous from 0, lengths t0·4^n)
#pragma once
#include "vec.h"
#include <vector>

namespace rc {

struct CascadeCfg {
    int P0 = 128;   // level-0 probes per axis == image size in pixels
    int B0 = 4;     // level-0 angular bins
    double t0 = 4;  // level-0 interval length (pixels)
    int levels = 0; // computed: smallest N+1 s.t. start_{N+1} ≥ diagonal

    static CascadeCfg make(int size, int b0 = 4, double t0_ = 4.0) {
        CascadeCfg c;
        c.P0 = size; c.B0 = b0; c.t0 = t0_;
        double diag = std::sqrt(2.0) * size;
        int n = 0;
        while (c.intervalStart(n + 1) < diag) n++;
        c.levels = n + 1;
        return c;
    }

    int gridN(int n) const { return P0 >> n; }              // probes per axis
    double spacing(int n) const { return (double)(1 << n); }
    int bins(int n) const { return B0 << (2 * n); }         // B0·4^n
    double intervalStart(int n) const {
        return t0 * (std::pow(4.0, n) - 1.0) / 3.0;
    }
    double intervalEnd(int n) const { return intervalStart(n + 1); }

    Vec2 probePos(int n, int i, int j) const {
        double s = spacing(n);
        return {(i + 0.5) * s, (j + 0.5) * s};
    }
    double binAngle(int n, int b, double jitter01 = 0.5) const {
        return TWO_PI * (b + jitter01) / bins(n);
    }
    // Angular bin at level n containing direction theta.
    int binOf(int n, double theta) const {
        double t = theta / TWO_PI;
        t -= std::floor(t);
        int b = (int)(t * bins(n));
        return b >= bins(n) ? bins(n) - 1 : b;
    }

    // Bilinear parents of probe position `pos` in the level-(n+1) grid.
    // Canonical order (bit-exactness contract): (i0,j0), (i1,j0), (i0,j1), (i1,j1).
    struct Parents {
        int idx[4][2];
        double beta[4]; // bilinear weights, sum to 1
    };
    Parents parentsOf(int n, Vec2 pos) const {
        int G = gridN(n + 1);
        double s = spacing(n + 1);
        double u = pos.x / s - 0.5, v = pos.y / s - 0.5;
        int i0 = (int)std::floor(u), j0 = (int)std::floor(v);
        double fu = u - i0, fv = v - j0;
        // clamp cell to grid, folding weights onto the border probes
        if (i0 < 0) { i0 = 0; fu = 0; }
        if (j0 < 0) { j0 = 0; fv = 0; }
        if (i0 > G - 2) { i0 = G - 2; fu = 1; }
        if (j0 > G - 2) { j0 = G - 2; fv = 1; }
        Parents p;
        int i1 = i0 + 1, j1 = j0 + 1;
        p.idx[0][0] = i0; p.idx[0][1] = j0; p.beta[0] = (1 - fu) * (1 - fv);
        p.idx[1][0] = i1; p.idx[1][1] = j0; p.beta[1] = fu * (1 - fv);
        p.idx[2][0] = i0; p.idx[2][1] = j1; p.beta[2] = (1 - fu) * fv;
        p.idx[3][0] = i1; p.idx[3][1] = j1; p.beta[3] = fu * fv;
        return p;
    }

    // Certified coverage window half-width for the level-n windowed merge
    // (Prop W', conditional-cell form) — computes the closed-form SOUND
    // upper bound of the margin lemma (Lemma M, theory notes): no
    // small-angle steps and no tuned margin, but NOT minimal — it uses
    // atan x ≤ x and ρ ≥ τ−d, so certified widths carry slack:
    //   write e := p−q (|e| = d ≤ √2·s_{n+1}), and for a ray direction û,
    //   ψ(τ) = atan2(e⊥, τ+e∥) the exact parent-side deviation. Then
    //   ψ'(τ) = −e⊥/ρ(τ)² is sign-fixed ⇒ depth extremes sit at the
    //   interval start / ∞ (endpoint reduction), and
    //     |ψ_φ(r) − ψ_ω(t_c)| ≤ β·D1 + max(D2, D3)·β   with
    //     D1 = d(t_c+d)/(t_c−d)²          (anchor rotation, per radian)
    //     D2 = d(t_c−t₁)/((t₁−d)(t_c−d))/β  (depth sweep, start side)
    //     D3 = d/(t_c−d)/β                 (depth sweep, far side)
    //   plus 1 bin for the intra-cell anchor-content offset and 1 for
    //   index rounding: w ≥ 2 + D1 + max(D2, D3). Paraxial breakdown
    //   t₁ ≤ d (possible at level 0 under extreme boundary jitter) ⇒ FULL
    //   RING (every parent bin once; complete by construction). The
    //   `rc coverage` oracle is the regression for this lemma, not its
    //   substitute. Call on the JITTERED cfg so the width tracks the
    //   active split.
    int coverageWindow(int n) const {
        double t1 = intervalStart(n + 1), t2 = intervalEnd(n + 1);
        double d = std::sqrt(2.0) * spacing(n + 1);
        if (t1 <= d) return bins(n + 1); // non-paraxial: full ring
        double tc = std::sqrt(t1 * t2);
        double beta = TWO_PI / bins(n + 1);
        double D1 = d * (tc + d) / ((tc - d) * (tc - d));
        double D2 = d * (tc - t1) / ((t1 - d) * (tc - d)) / beta;
        double D3 = d / (tc - d) / beta;
        double need = 2.0 + D1 + std::fmax(D2, D3);
        int w = (int)std::ceil(need - 1e-9);
        return w < 1 ? 1 : w;
    }

    size_t entryCount(int n) const {
        return (size_t)gridN(n) * gridN(n) * bins(n);
    }
    size_t index(int n, int i, int j, int b) const {
        return ((size_t)j * gridN(n) + i) * bins(n) + b;
    }
};

} // namespace rc
