#include "restir.h"

namespace rc {

// A reservoir sample: one concrete ray within the bin, tail already resolved.
struct Sample {
    double omega = 0;  // direction angle (within the bin at its level)
    Vec2 y;            // reconnection point: origin of the radiance in c
    bool hasY = false; // false ⇒ escaped everywhere (sky); shift = translation
    Vec3 c;            // live radiance (cRef rescaled to current emission)
    // Temporal re-evaluation: c is recomputed each frame as
    //   c = cRef · (lum(emission at y now) / emitLum0),
    // with cRef/emitLum0 immutable from resolve time. Without this,
    // lum-targeted temporal RIS prefers stale bright samples forever
    // (measured: zero response to a light turning off). Keeping cRef
    // immutable (rather than overwriting) lets a geometrically-valid sample
    // REVIVE when its emitter comes back — otherwise the dead sample's
    // accumulated M poisons the merge and turn-on never converges.
    Vec3 cRef;
    double emitLum0 = 0;
};

// W is the *bin-mean* UCW: densities are expressed relative to uniform-on-bin,
// so a fresh uniform candidate has W = 1 and the bin-mean estimator is c·W.
// This normalization makes the cross-level collapse (§3.5) a plain product
// c_sel·W_sel with no |Ω| bookkeeping (bin sizes are equal within a level and
// tails are resolved *before* entering the child's own domain).
struct Reservoir {
    Sample s;
    double W = 0;
    double M = 0;
};

struct Cand {
    double omega = 0;
    bool hit = false;
    Vec2 y;      // hit point if hit
    Vec3 cNear;  // emission at near-interval hit
};

static double phat(const Vec3& c, double lambda) { return lum(c) + lambda; }

// ---------------------------------------------------------------------------

Image renderReservoirRC(const Scene& scene, const CascadeCfg& cfg,
                        const RParams& prm, RCMode mode,
                        const RenderHooks* hooks) {
    const int N = cfg.levels;
    const bool degen = (mode == RCMode::Degenerate);
    const bool useTemporal = prm.temporal && mode == RCMode::Full;
    const int frames = degen ? 1 : prm.frames;
    const int burnIn = useTemporal ? frames / 2 : 0;

    if (hooks && hooks->probe) {
        auto& s = hooks->probe->series;
        s.assign(N, {});
        for (int n = 0; n < N; n++)
            s[n].resize(std::min((size_t)hooks->probe->maxEntries,
                                 cfg.entryCount(n)));
    }

    std::vector<std::vector<Cand>> cand(N);
    std::vector<std::vector<Reservoir>> R(N), Rprev(N);
    for (int n = 0; n < N; n++) {
        cand[n].resize(cfg.entryCount(n));
        R[n].resize(cfg.entryCount(n));
        if (useTemporal) Rprev[n].resize(cfg.entryCount(n));
    }

    Image accum(cfg.P0, cfg.P0);
    int accumCount = 0;

    Scene frameScene;
    for (int f = 0; f < frames; f++) {
        const Scene& sc = (hooks && hooks->sceneAt)
                              ? (frameScene = hooks->sceneAt(f))
                              : scene;
        // Per-frame interval-boundary jitter (see RParams::boundaryJitter).
        // Only t0 is scaled: probe grids, bins, and indexing are untouched.
        CascadeCfg cfgF = cfg;
        bool splitSwitched = false;
        if (!degen && prm.boundaryJitter) {
            int block = useTemporal ? std::max(1, prm.bjitterBlock) : 1;
            PCG32 rj(hashCombine(prm.seed, (uint64_t)(f / block) + 0xB007), 17);
            cfgF.t0 = cfg.t0 * std::pow(4.0, rj.uniform() - 0.5);
            splitSwitched = useTemporal && f > 0 && f % block == 0;
        }
        const bool frameChanged = hooks && hooks->sceneChanged &&
                                  hooks->sceneChanged(f);
        // ---- Phase 1: candidate generation (per level, independent) ----
        for (int n = 0; n < N; n++) {
            int G = cfg.gridN(n), B = cfg.bins(n);
            double t0 = cfgF.intervalStart(n), t1 = cfgF.intervalEnd(n);
            for (int j = 0; j < G; j++)
                for (int i = 0; i < G; i++) {
                    Vec2 p = cfg.probePos(n, i, j);
                    for (int b = 0; b < B; b++) {
                        size_t k = cfg.index(n, i, j, b);
                        double xi = 0.5; // degenerate: bin center
                        if (!degen) {
                            PCG32 rng(hashCombine(prm.seed,
                                  hashCombine((uint64_t)f + 1,
                                  hashCombine((uint64_t)n + 1, k))), 7);
                            xi = rng.uniform();
                        }
                        Cand& c = cand[n][k];
                        c.omega = cfg.binAngle(n, b, xi);
                        Vec2 d{std::cos(c.omega), std::sin(c.omega)};
                        Hit h;
                        if (sc.intersect(p, d, t0, t1, h)) {
                            c.hit = true;
                            c.y = h.pos;
                            c.cNear = sc.shapes[h.shape].emission;
                        } else {
                            c.hit = false;
                            c.cNear = Vec3();
                        }
                    }
                }
        }

        // ---- Phase 2: top-down merge N-1..0 (level N-1 handled as top) ----
        for (int n = N - 1; n >= 0; n--) {
            int G = cfg.gridN(n), B = cfg.bins(n);
            for (int j = 0; j < G; j++)
                for (int i = 0; i < G; i++) {
                    Vec2 p = cfg.probePos(n, i, j);
                    CascadeCfg::Parents par{};
                    if (n < N - 1) par = cfg.parentsOf(n, p);
                    for (int b = 0; b < B; b++) {
                        size_t k = cfg.index(n, i, j, b);
                        const Cand& cd = cand[n][k];
                        Sample s;
                        s.omega = cd.omega;

                        if (cd.hit) {
                            s.c = cd.cNear;
                            s.cRef = cd.cNear;
                            s.y = cd.y;
                            s.hasY = true;
                            s.emitLum0 = lum(cd.cNear);
                        } else if (n == N - 1) {
                            s.c = Vec3(); // sky = 0
                            s.hasY = false;
                        } else if (degen) {
                            // §3.6 canonical value-passing merge — must match
                            // vanilla.cpp op-for-op:
                            //   acc += (childSum * 0.25) * beta_q
                            Vec3 acc;
                            for (int q = 0; q < 4; q++) {
                                Vec3 childSum;
                                for (int cb = 0; cb < 4; cb++) {
                                    size_t kp = cfg.index(n + 1, par.idx[q][0],
                                                          par.idx[q][1], 4 * b + cb);
                                    childSum += R[n + 1][kp].s.c;
                                }
                                acc += childSum * 0.25 * par.beta[q];
                            }
                            s.c = cd.cNear + acc;
                            s.hasY = false;
                        } else if (mode == RCMode::StochasticRC) {
                            // Jittered vanilla: value-passing merge over the
                            // parents' bin-mean estimates (c·W). Keeps RC's
                            // interpolation/factorization structure; only the
                            // rays are stochastic. Equal-ray control for
                            // isolating the merge-as-RIS gain.
                            Vec3 acc;
                            for (int q = 0; q < 4; q++) {
                                Vec3 childSum;
                                for (int cb = 0; cb < 4; cb++) {
                                    size_t kp = cfg.index(n + 1, par.idx[q][0],
                                                          par.idx[q][1], 4 * b + cb);
                                    const Reservoir& rq = R[n + 1][kp];
                                    childSum += rq.s.c * rq.W;
                                }
                                acc += childSum * 0.25 * par.beta[q];
                            }
                            s.c = cd.cNear + acc;
                            s.hasY = false;
                        } else {
                            // ---- merge-as-RIS over the 4 parents (§3.3) ----
                            // Target is p's own field: each parent sample is
                            // re-anchored to p via reconnection, so all four
                            // are estimates of the SAME quantity; β_q is only
                            // the MIS/selection prior (Σβ=1 by construction).
                            // Per-parent reprojected bin selection. Pulling a
                            // single bin b'=binOf(ω) for all parents fails at
                            // deep levels: parallax shift in bin units grows
                            // ~2^n under (s×2, B×4, t×4) scaling — ~4 bins at
                            // level 3 — so misaligned parents return darkness
                            // (measured: 2× column striping in S2). Reproject
                            // through the child ray's characteristic far
                            // point instead (RC "bilinear fix", RIS-adapted).
                            double tRep = std::sqrt(cfgF.intervalStart(n + 1) *
                                                    cfgF.intervalEnd(n + 1));
                            Vec2 zRep = p + Vec2{std::cos(cd.omega),
                                                 std::sin(cd.omega)} * tRep;
                            PCG32 rng(hashCombine(prm.seed,
                                  hashCombine((uint64_t)f + 1,
                                  hashCombine((uint64_t)n + 0x51, k))), 11);
                            // ρ-validation (§3.4), "account for it in MIS" reading: a
                            // blocked candidate leaves the valid proposal set
                            // and β renormalizes over survivors. Kill-after-
                            // select instead DOUBLE-counts visibility — the
                            // chain already bakes its own visibility into
                            // sample existence (blocked chains resolve c=0),
                            // so naive killing scales penumbra like
                            // v_chain·v_p (measured: −16% lit S1 / −33% room
                            // S2). All-blocked ⇒ 0 (true umbra, leak dies).
                            bool doValidate = prm.rho > 0 &&
                                              (!prm.rhoLevel0Only || n == 0) &&
                                              rng.uniform() < prm.rho;
                            double J[4], betaValidSum = 0;
                            bool valid[4];
                            const Reservoir* pr[4];
                            for (int q = 0; q < 4; q++) {
                                Vec2 qpos0 = cfg.probePos(n + 1, par.idx[q][0],
                                                          par.idx[q][1]);
                                Vec2 toZ = zRep - qpos0;
                                int bp = cfg.binOf(n + 1,
                                                   std::atan2(toZ.y, toZ.x));
                                size_t kp = cfg.index(n + 1, par.idx[q][0],
                                                      par.idx[q][1], bp);
                                pr[q] = &R[n + 1][kp];
                                const Reservoir& rq = *pr[q];
                                J[q] = 1.0;
                                valid[q] = true;
                                if (rq.s.hasY) {
                                    Vec2 qpos = cfg.probePos(n + 1, par.idx[q][0],
                                                             par.idx[q][1]);
                                    double rQ = (rq.s.y - qpos).len();
                                    double rP = (rq.s.y - p).len();
                                    // 2D reconnection Jacobian (§3.4): angle
                                    // measure ⇒ first power. Never hardcode 1.
                                    J[q] = (rP > 1e-9) ? rQ / rP : 0.0;
                                    if (doValidate) {
                                        // Validate only the segment the tail
                                        // claims beyond the candidate's own
                                        // near interval.
                                        Vec2 dir = rq.s.y - p;
                                        double dist = dir.len();
                                        double tEnd = dist - std::fmax(1e-3, 1e-3 * dist);
                                        double tBeg = cfgF.intervalEnd(n) * 0.999;
                                        Hit h;
                                        if (dist > 1e-9 && tEnd > tBeg &&
                                            sc.intersect(p, dir * (1.0 / dist),
                                                            tBeg, tEnd, h))
                                            valid[q] = false;
                                    }
                                }
                                if (valid[q]) betaValidSum += par.beta[q];
                            }
                            // Known approximation (risk register): the
                            // reconnected direction may exit bin bp; the
                            // penumbra condition bounds this to O(1) bins and
                            // it is accepted un-MIS'd in M1.
                            double w[4], wsum = 0;
                            for (int q = 0; q < 4; q++) {
                                const Reservoir& rq = *pr[q];
                                double m = (valid[q] && betaValidSum > 0)
                                               ? par.beta[q] / betaValidSum
                                               : 0.0;
                                w[q] = m * phat(rq.s.c, prm.lambda) * J[q] * rq.W;
                                if (!(w[q] > 0)) w[q] = 0;
                                wsum += w[q];
                            }
                            Vec3 tail;
                            Vec2 tailY;
                            bool tailHasY = false;
                            if (wsum > 0) {
                                double u = rng.uniform() * wsum;
                                int sel = 3;
                                double run = 0;
                                for (int q = 0; q < 4; q++) {
                                    run += w[q];
                                    if (u < run) { sel = q; break; }
                                }
                                const Reservoir& rs = *pr[sel];
                                // W collapse (§3.5): resolve the tail into a
                                // value NOW; the child's W stays its own.
                                double Wsel = wsum / phat(rs.s.c, prm.lambda);
                                tail = rs.s.c * Wsel;
                                tailY = rs.s.y;
                                tailHasY = rs.s.hasY;
                                s.emitLum0 = rs.s.emitLum0; // propagate origin
                            }
                            s.c = cd.cNear + tail;
                            s.cRef = s.c;
                            s.y = tailY;
                            s.hasY = tailHasY;
                        }

                        // Fresh single-candidate reservoir: uniform-on-bin ⇒
                        // normalized pdf 1 ⇒ W = 1, M = 1.
                        Reservoir rnew;
                        rnew.s = s;
                        rnew.W = 1.0;
                        rnew.M = 1.0;

                        if (useTemporal) {
                            // Temporal merge, identity shift (world-space
                            // probes, static scene). Confidence-weighted
                            // balance heuristic; Σm = 1 over both candidates.
                            Reservoir prev = Rprev[n][k];
                            // Temporal re-evaluation: rescale stored radiance
                            // by the emitter's current output at y.
                            if (prev.s.hasY && prev.s.emitLum0 > 0) {
                                int which = -1;
                                sc.sdf(prev.s.y, &which);
                                double lumNow = which >= 0
                                    ? lum(sc.shapes[which].emission) : 0.0;
                                prev.s.c = prev.s.cRef *
                                           (lumNow / prev.s.emitLum0);
                            }
                            double cap = std::fmin((double)prm.mcap0 * (1 << n),
                                                   (double)prm.mcapMax);
                            double Mprev = std::fmin(prev.M, cap);
                            // Scene-change invalidation: uniform, frame-wide,
                            // independent of any sample draw => unbiased.
                            if (frameChanged) Mprev = 0;
                            // Split switch: old samples embed the previous
                            // interval decomposition — drop them entirely.
                            if (splitSwitched) Mprev = 0;
                            // NOTE: no candidate-triggered M-clamp. Clamping
                            // Mprev when the new candidate looks "surprising"
                            // makes the MIS weights depend on the sample draw
                            // and is measurably biased (+25% S2 upward bias
                            // when tried). Turn-ON adaptation therefore costs
                            // ~M frames; principled fixes (scene-change flags,
                            // temporal-gradient MIS) are M2+ work.
                            if (Mprev > 0) {
                                double mNew = rnew.M / (rnew.M + Mprev);
                                double mPrev = Mprev / (rnew.M + Mprev);
                                double wNew = mNew * phat(rnew.s.c, prm.lambda) * rnew.W;
                                double wPrev = mPrev * phat(prev.s.c, prm.lambda) * prev.W;
                                double ws = wNew + wPrev;
                                Reservoir merged;
                                merged.M = rnew.M + Mprev;
                                if (ws > 0) {
                                    PCG32 rng(hashCombine(prm.seed,
                                          hashCombine((uint64_t)f + 1,
                                          hashCombine((uint64_t)n + 0xA1, k))), 13);
                                    bool takeNew = rng.uniform() * ws < wNew;
                                    merged.s = takeNew ? rnew.s : prev.s;
                                    merged.W = ws / phat(merged.s.c, prm.lambda);
                                } else {
                                    merged.s = rnew.s;
                                    merged.W = 0;
                                }
                                R[n][k] = merged;
                            } else {
                                R[n][k] = rnew;
                            }
                        } else {
                            R[n][k] = rnew;
                        }
                    }
                }
        }

        if (hooks && hooks->probe)
            for (int n = 0; n < N; n++)
                for (size_t e = 0; e < hooks->probe->series[n].size(); e++) {
                    const Reservoir& r = R[n][e];
                    hooks->probe->series[n][e].push_back(lum(r.s.c) * r.W);
                }

        // ---- Phase 3: gather (level 0 → image) ----
        bool wantFrame = hooks && hooks->onFrame;
        if (f >= burnIn || wantFrame) {
            Image cur(cfg.P0, cfg.P0);
            int B0 = cfg.bins(0);
            for (int y = 0; y < cfg.P0; y++)
                for (int x = 0; x < cfg.P0; x++) {
                    Vec2 p = cfg.probePos(0, x, y);
                    int which;
                    if (sc.sdf(p, &which) < 0) {
                        cur.at(x, y) = sc.shapes[which].emission;
                        continue;
                    }
                    Vec3 acc;
                    for (int b = 0; b < B0; b++) {
                        const Reservoir& r = R[0][cfg.index(0, x, y, b)];
                        // bin-mean estimator: c·W (W normalized to bin-uniform)
                        acc += r.s.c * (degen ? 1.0 : r.W);
                    }
                    cur.at(x, y) = acc * (1.0 / B0);
                }
            if (f >= burnIn) {
                for (size_t i = 0; i < accum.px.size(); i++)
                    accum.px[i] += cur.px[i];
                accumCount++;
            }
            if (wantFrame) hooks->onFrame(f, cur);
        }

        if (useTemporal)
            for (int n = 0; n < N; n++) Rprev[n] = R[n];
    }

    for (auto& v : accum.px) v = v * (1.0 / accumCount);
    return accum;
}

} // namespace rc
