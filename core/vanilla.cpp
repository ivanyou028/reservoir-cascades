#include "vanilla.h"

namespace rc {

// Canonical merge order (bit-exactness contract with the degenerate mode):
//   for q in parents 0..3: childSum = c[4b]+c[4b+1]+c[4b+2]+c[4b+3];
//                          acc += beta_q * (childSum * 0.25)
//   L = c_near + (escaped ? acc : 0)
Image renderVanillaRC(const Scene& scene, const CascadeCfg& cfg) {
    int N = cfg.levels;
    std::vector<std::vector<Vec3>> L(N);
    std::vector<std::vector<unsigned char>> esc(N);

    // Phase 1: trace deterministic bin-center rays per (probe, bin, level).
    for (int n = 0; n < N; n++) {
        int G = cfg.gridN(n), B = cfg.bins(n);
        L[n].assign(cfg.entryCount(n), Vec3());
        esc[n].assign(cfg.entryCount(n), 0);
        double t0 = cfg.intervalStart(n), t1 = cfg.intervalEnd(n);
        for (int j = 0; j < G; j++)
            for (int i = 0; i < G; i++) {
                Vec2 p = cfg.probePos(n, i, j);
                for (int b = 0; b < B; b++) {
                    double th = cfg.binAngle(n, b); // bin center
                    Vec2 d{std::cos(th), std::sin(th)};
                    Hit h;
                    size_t k = cfg.index(n, i, j, b);
                    if (scene.intersect(p, d, t0, t1, h)) {
                        L[n][k] = scene.shapes[h.shape].emission;
                        esc[n][k] = 0;
                    } else {
                        esc[n][k] = 1; // interval fully transparent
                    }
                }
            }
    }

    // Phase 2: top-down merge N-1..0. Parent values are already final.
    for (int n = N - 2; n >= 0; n--) {
        int G = cfg.gridN(n), B = cfg.bins(n);
        int Bp = cfg.bins(n + 1);
        (void)Bp;
        for (int j = 0; j < G; j++)
            for (int i = 0; i < G; i++) {
                Vec2 p = cfg.probePos(n, i, j);
                CascadeCfg::Parents par = cfg.parentsOf(n, p);
                for (int b = 0; b < B; b++) {
                    size_t k = cfg.index(n, i, j, b);
                    if (!esc[n][k]) continue;
                    Vec3 acc;
                    for (int q = 0; q < 4; q++) {
                        Vec3 childSum;
                        for (int cb = 0; cb < 4; cb++) {
                            size_t kp = cfg.index(n + 1, par.idx[q][0],
                                                  par.idx[q][1], 4 * b + cb);
                            childSum += L[n + 1][kp];
                        }
                        acc += childSum * 0.25 * par.beta[q];
                    }
                    L[n][k] += acc;
                }
            }
    }

    // Phase 3: gather — pixel = mean over level-0 bins; probes inside shapes
    // show their own emission (identical rule in every renderer).
    Image img(cfg.P0, cfg.P0);
    int B0 = cfg.bins(0);
    for (int y = 0; y < cfg.P0; y++)
        for (int x = 0; x < cfg.P0; x++) {
            Vec2 p = cfg.probePos(0, x, y);
            int which;
            if (scene.sdf(p, &which) < 0) {
                img.at(x, y) = scene.shapes[which].emission;
                continue;
            }
            Vec3 acc;
            for (int b = 0; b < B0; b++)
                acc += L[0][cfg.index(0, x, y, b)];
            img.at(x, y) = acc * (1.0 / B0);
        }
    return img;
}

} // namespace rc
