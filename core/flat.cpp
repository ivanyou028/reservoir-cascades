#include "flat.h"
#include "cascade.h"

namespace rc {

// Same sample semantics as the cascade renderer (restir.cpp): live radiance c
// plus immutable as-resolved cRef/emitLum0 for temporal re-evaluation.
struct FSample {
    double omega = 0;
    Vec2 y;
    bool hasY = false;
    Vec3 c;
    Vec3 cRef;
    double emitLum0 = 0;
};

struct FRes {
    FSample s;
    double W = 0;
    double M = 0;
};

static double fphat(const Vec3& c, double lambda) { return lum(c) + lambda; }

Image renderFlatReuse(const Scene& sc, int size, const FlatParams& prm,
                      RayCounts* rays,
                      std::function<void(int, const Image&)> onFrame) {
    const int B = prm.bins > 0 ? prm.bins
                               : 4 * CascadeCfg::make(size).levels;
    const int frames = prm.frames;
    const int burnIn = prm.temporal ? frames / 2 : 0;
    const double tmax = 4.0 * size; // same horizon as renderReference
    const size_t NE = (size_t)size * size * B;
    const bool spatial = prm.radius > 0 && prm.k > 0;

    auto probePos = [](int i, int j) { return Vec2{i + 0.5, j + 0.5}; };
    auto entry = [&](int i, int j, int b) {
        return ((size_t)j * size + i) * B + b;
    };
    auto binOf = [&](double theta) {
        double t = theta / TWO_PI;
        t -= std::floor(t);
        int b = (int)(t * B);
        return b >= B ? B - 1 : b;
    };

    std::vector<FRes> Rprev(NE), Rt(NE), R(NE);

    Image accum(size, size);
    int accumCount = 0;

    for (int f = 0; f < frames; f++) {
        // ---- Phase 1+2: fresh candidate + temporal merge → Rt ----
        for (int j = 0; j < size; j++)
            for (int i = 0; i < size; i++) {
                Vec2 p = probePos(i, j);
                for (int b = 0; b < B; b++) {
                    size_t kk = entry(i, j, b);
                    PCG32 rng(hashCombine(prm.seed,
                          hashCombine((uint64_t)f + 1,
                          hashCombine((uint64_t)0x23, kk))), 23);
                    FSample s;
                    s.omega = TWO_PI * (b + rng.uniform()) / B;
                    Vec2 d{std::cos(s.omega), std::sin(s.omega)};
                    Hit h;
                    if (rays) rays->cand++;
                    if (sc.intersect(p, d, 0.0, tmax, h)) {
                        s.hasY = true;
                        s.y = h.pos;
                        s.c = sc.shapes[h.shape].emission;
                        s.cRef = s.c;
                        s.emitLum0 = lum(s.c);
                    }
                    FRes fresh;
                    fresh.s = s;
                    fresh.W = 1.0;
                    fresh.M = 1.0;

                    if (prm.temporal) {
                        FRes prev = Rprev[kk];
                        // Temporal re-evaluation (identical to the cascade's).
                        if (prev.s.hasY && prev.s.emitLum0 > 0) {
                            int which = -1;
                            sc.sdf(prev.s.y, &which);
                            double lumNow = which >= 0
                                ? lum(sc.shapes[which].emission) : 0.0;
                            prev.s.c = prev.s.cRef *
                                       (lumNow / prev.s.emitLum0);
                        }
                        double Mprev = std::fmin(prev.M, (double)prm.mcap);
                        if (Mprev > 0) {
                            double mNew = fresh.M / (fresh.M + Mprev);
                            double mPrev = Mprev / (fresh.M + Mprev);
                            double wNew =
                                mNew * fphat(fresh.s.c, prm.lambda) * fresh.W;
                            double wPrev =
                                mPrev * fphat(prev.s.c, prm.lambda) * prev.W;
                            double ws = wNew + wPrev;
                            FRes merged;
                            merged.M = fresh.M + Mprev;
                            if (ws > 0) {
                                bool takeNew = rng.uniform() * ws < wNew;
                                merged.s = takeNew ? fresh.s : prev.s;
                                merged.W =
                                    ws / fphat(merged.s.c, prm.lambda);
                            } else {
                                merged.s = fresh.s;
                                merged.W = 0;
                            }
                            Rt[kk] = merged;
                        } else {
                            Rt[kk] = fresh;
                        }
                    } else {
                        Rt[kk] = fresh;
                    }
                }
            }

        // ---- Phase 3: spatial reuse (reads Rt, writes R) ----
        if (spatial) {
            for (int j = 0; j < size; j++)
                for (int i = 0; i < size; i++) {
                    Vec2 p = probePos(i, j);
                    for (int b = 0; b < B; b++) {
                        size_t kk = entry(i, j, b);
                        PCG32 rng(hashCombine(prm.seed,
                              hashCombine((uint64_t)f + 1,
                              hashCombine((uint64_t)0x31, kk))), 31);

                        // Proposal set: self + up to k disk neighbors.
                        // Exclusion (coverage / validation) drops a technique
                        // from the set entirely — renormalize-before-select,
                        // the same discipline as the cascade merge. MIS uses
                        // the proper generalized balance (per-candidate
                        // realization test), NOT naive M-splitting: naive
                        // confidence weights systematically darken wherever
                        // neighbor supports differ (measured −25–40% on S2),
                        // and we refuse to hang the comparison on a known-
                        // biased strawman.
                        const int KMAX = 16;
                        const FRes* cand[KMAX + 1];
                        double J[KMAX + 1], M[KMAX + 1];
                        Vec2 qpos[KMAX + 1];
                        int cnt = 0;
                        cand[cnt] = &Rt[kk];
                        J[cnt] = 1.0;
                        M[cnt] = std::fmax(Rt[kk].M, 1e-9);
                        qpos[cnt] = p;
                        cnt++;
                        int kNb = prm.k > KMAX ? KMAX : prm.k;
                        for (int t = 0; t < kNb; t++) {
                            double ang = TWO_PI * rng.uniform();
                            double rad = prm.radius * std::sqrt(rng.uniform());
                            int qi = (int)std::floor(p.x + rad * std::cos(ang));
                            int qj = (int)std::floor(p.y + rad * std::sin(ang));
                            if (qi < 0 || qj < 0 || qi >= size || qj >= size)
                                continue;
                            if (qi == i && qj == j) continue;
                            const FRes& nb = Rt[entry(qi, qj, b)];
                            Vec2 q = probePos(qi, qj);
                            double Jq = 1.0;
                            if (nb.s.hasY) {
                                // Reconnection shift q→p: keep y, re-aim.
                                Vec2 toY = nb.s.y - p;
                                double rP = toY.len();
                                double rQ = (nb.s.y - q).len();
                                if (rP < 1e-9) continue;
                                // Coverage: the reconnected direction must
                                // still lie in this probe's bin b (the shift
                                // must land in the target domain). Parallax
                                // grows with r/dist — the flat analogue of
                                // the cascade's per-level coverage question.
                                if (binOf(std::atan2(toY.y, toY.x)) != b)
                                    continue;
                                Jq = rQ / rP;
                                // ρ-validation: full-length shadow segment
                                // (no interval structure to bound it).
                                if (prm.rho > 0 &&
                                    rng.uniform() < prm.rho) {
                                    double tEnd = rP -
                                        std::fmax(1e-3, 1e-3 * rP);
                                    Hit h;
                                    if (tEnd > 1e-3) {
                                        if (rays) rays->valid++;
                                        if (sc.intersect(p,
                                                toY * (1.0 / rP),
                                                1e-3, tEnd, h))
                                            continue; // blocked: drop
                                    }
                                }
                            } else {
                                // Sky sample: translation shift, same
                                // direction, trivially in-bin, J = 1.
                                Jq = 1.0;
                            }
                            cand[cnt] = &nb;
                            J[cnt] = Jq;
                            M[cnt] = std::fmax(nb.M, 1e-9);
                            qpos[cnt] = q;
                            cnt++;
                        }

                        // Generalized balance over survivors: technique t
                        // (reuse from probe qpos[t]) realizes point y iff
                        // dir(qpos[t]→y) falls in bin b at that probe; sky
                        // samples are realized by every technique
                        // (translation shift preserves direction).
                        double Msum = 0;
                        for (int t = 0; t < cnt; t++) Msum += M[t];
                        double w[KMAX + 1], wsum = 0;
                        for (int t = 0; t < cnt; t++) {
                            double denom = Msum;
                            if (cand[t]->s.hasY) {
                                denom = 0;
                                for (int u = 0; u < cnt; u++) {
                                    Vec2 toY = cand[t]->s.y - qpos[u];
                                    if (binOf(std::atan2(toY.y, toY.x)) == b)
                                        denom += M[u];
                                }
                            }
                            double m = denom > 0 ? M[t] / denom : 0.0;
                            w[t] = m * fphat(cand[t]->s.c, prm.lambda) *
                                   J[t] * cand[t]->W;
                            if (!(w[t] > 0)) w[t] = 0;
                            wsum += w[t];
                        }
                        FRes outr;
                        outr.M = Msum;
                        if (wsum > 0) {
                            double u = rng.uniform() * wsum;
                            int sel = cnt - 1;
                            double run = 0;
                            for (int t = 0; t < cnt; t++) {
                                run += w[t];
                                if (u < run) { sel = t; break; }
                            }
                            outr.s = cand[sel]->s;
                            if (outr.s.hasY) {
                                // Store the reconnected direction at p.
                                Vec2 toY = outr.s.y - p;
                                outr.s.omega = std::atan2(toY.y, toY.x);
                            }
                            outr.W = wsum / fphat(outr.s.c, prm.lambda);
                        } else {
                            outr.s = Rt[kk].s;
                            outr.W = 0;
                        }
                        R[kk] = outr;
                    }
                }
        } else {
            R = Rt;
        }

        // ---- Gather ----
        bool wantFrame = (bool)onFrame;
        if (f >= burnIn || wantFrame) {
            Image cur(size, size);
            for (int y = 0; y < size; y++)
                for (int x = 0; x < size; x++) {
                    Vec2 p = probePos(x, y);
                    int which;
                    if (sc.sdf(p, &which) < 0) {
                        cur.at(x, y) = sc.shapes[which].emission;
                        continue;
                    }
                    Vec3 acc;
                    for (int b = 0; b < B; b++) {
                        const FRes& r = R[entry(x, y, b)];
                        acc += r.s.c * r.W;
                    }
                    cur.at(x, y) = acc * (1.0 / B);
                }
            if (f >= burnIn) {
                for (size_t i2 = 0; i2 < accum.px.size(); i2++)
                    accum.px[i2] += cur.px[i2];
                accumCount++;
            }
            if (wantFrame) onFrame(f, cur);
        }

        if (prm.temporal) Rprev = R;
    }

    for (auto& v : accum.px) v = v * (1.0 / accumCount);
    return accum;
}

} // namespace rc
