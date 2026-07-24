// CPU reference driver.
//   rc selftest --scene S [--size N]           bit-exact degen-vs-vanilla (§3.6)
//   rc eval --scene S [--size N] [--frames F] [--seed X] [--no-temporal]
//           [--ref-rays K] [--outdir D]        renders ref/vanilla/full + metrics
#include "../core/cascade.h"
#include "../core/flat.h"
#include "../core/reference.h"
#include "../core/restir.h"
#include "../core/vanilla.h"
#include "../metrics/metrics.h"
#include <cstdio>
#include <cstring>
#include <string>
#include <sys/stat.h>

using namespace rc;

static std::string arg(int argc, char** argv, const char* name, const char* dflt) {
    for (int i = 1; i < argc - 1; i++)
        if (!std::strcmp(argv[i], name)) return argv[i + 1];
    return dflt;
}
static bool flag(int argc, char** argv, const char* name) {
    for (int i = 1; i < argc; i++)
        if (!std::strcmp(argv[i], name)) return true;
    return false;
}

static int selftest(const std::string& scenePath, int size) {
    Scene sc = Scene::load(scenePath, size);
    CascadeCfg cfg = CascadeCfg::make(size);
    Image a = renderVanillaRC(sc, cfg);
    RParams prm;
    Image b = renderReservoirRC(sc, cfg, prm, RCMode::Degenerate);
    long diff = 0;
    for (size_t i = 0; i < a.px.size(); i++)
        if (!(a.px[i] == b.px[i])) diff++;
    std::printf("[selftest] %s size=%d levels=%d: %s (%ld/%zu pixels differ)\n",
                sc.name.c_str(), size, cfg.levels,
                diff == 0 ? "PASS (bit-exact)" : "FAIL", diff, a.px.size());
    return diff == 0 ? 0 : 1;
}

static void report(const char* label, const Image& est, const Image& ref,
                   const Scene& sc) {
    std::printf("  %-8s MAPE(all)=%.4f", label, mape(est, ref, nullptr));
    for (auto& m : sc.masks) {
        std::printf("  MAPE(%s)=%.4f E(%s)=%.5f", m.name.c_str(),
                    mape(est, ref, &m), m.name.c_str(), maskEnergy(est, &m));
    }
    std::printf("\n");
}

static int eval(int argc, char** argv) {
    std::string scenePath = arg(argc, argv, "--scene", "");
    if (scenePath.empty()) { std::fprintf(stderr, "--scene required\n"); return 2; }
    int size = std::atoi(arg(argc, argv, "--size", "128").c_str());
    int frames = std::atoi(arg(argc, argv, "--frames", "64").c_str());
    int refRays = std::atoi(arg(argc, argv, "--ref-rays", "4096").c_str());
    uint64_t seed = std::strtoull(arg(argc, argv, "--seed", "1").c_str(), nullptr, 10);
    std::string outdir = arg(argc, argv, "--outdir", "out");
    bool noTemporal = flag(argc, argv, "--no-temporal");

    ::mkdir(outdir.c_str(), 0755);
    Scene sc = Scene::load(scenePath, size);
    CascadeCfg cfg = CascadeCfg::make(size);
    std::printf("[eval] %s size=%d levels=%d frames=%d seed=%llu temporal=%d\n",
                sc.name.c_str(), size, cfg.levels, frames,
                (unsigned long long)seed, !noTemporal);

    char buf[512];

    // reference (cached by scene+size+rays)
    std::snprintf(buf, sizeof buf, "%s/%s_%d_ref%d.pfm", outdir.c_str(),
                  sc.name.c_str(), size, refRays);
    Image ref;
    if (!Image::readPFM(buf, ref)) {
        std::printf("  rendering reference (%d rays/px)...\n", refRays);
        ref = renderReference(sc, size, refRays);
        ref.writePFM(buf);
    }
    std::snprintf(buf, sizeof buf, "%s/%s_%d_ref.ppm", outdir.c_str(),
                  sc.name.c_str(), size);
    ref.writePPM(buf);

    Image van = renderVanillaRC(sc, cfg);
    std::snprintf(buf, sizeof buf, "%s/%s_%d_vanilla.ppm", outdir.c_str(),
                  sc.name.c_str(), size);
    van.writePPM(buf);
    std::snprintf(buf, sizeof buf, "%s/%s_%d_vanilla.pfm", outdir.c_str(),
                  sc.name.c_str(), size);
    van.writePFM(buf);

    RParams prm;
    prm.frames = frames;
    prm.seed = seed;
    prm.temporal = !noTemporal;
    prm.rho = std::atof(arg(argc, argv, "--rho", "0").c_str());
    prm.lambda = std::atof(arg(argc, argv, "--lambda", "0.05").c_str());
    prm.rhoLevel0Only = flag(argc, argv, "--rho-l0");
    prm.mcap0 = std::atoi(arg(argc, argv, "--mcap0", "8").c_str());
    prm.boundaryJitter = !flag(argc, argv, "--no-bjitter");
    prm.bjitterBlock = std::atoi(arg(argc, argv, "--bjitter-block", "8").c_str());
    prm.window = std::atoi(arg(argc, argv, "--window", "-1").c_str());
    prm.windowAuto = flag(argc, argv, "--window-auto");
    bool stoch = flag(argc, argv, "--stoch");
    RayCounts rays;
    RenderHooks rayHooks;
    rayHooks.rays = &rays;
    Image full = renderReservoirRC(sc, cfg, prm,
                                   stoch ? RCMode::StochasticRC : RCMode::Full,
                                   &rayHooks);
    std::printf("  rays/frame: candidate=%.0f validation=%.0f (+%.1f%%)\n",
                (double)rays.cand / prm.frames,
                (double)rays.valid / prm.frames,
                rays.cand ? 100.0 * rays.valid / rays.cand : 0.0);
    std::printf("  parent reads/frame: %.0f (window=%s)\n",
                (double)rays.reads / prm.frames,
                prm.windowAuto ? "auto" :
                (prm.window >= 0 ? arg(argc, argv, "--window", "-1").c_str()
                                 : "off"));
    std::printf("  traced px/frame: candidate=%.0f validation=%.0f "
                "(mean len: cand=%.2f valid=%.2f px/ray)\n",
                rays.candLen / prm.frames, rays.validLen / prm.frames,
                rays.cand ? rays.candLen / rays.cand : 0.0,
                rays.valid ? rays.validLen / rays.valid : 0.0);
    std::snprintf(buf, sizeof buf, "%s/%s_%d_full.ppm", outdir.c_str(),
                  sc.name.c_str(), size);
    full.writePPM(buf);
    std::snprintf(buf, sizeof buf, "%s/%s_%d_full.pfm", outdir.c_str(),
                  sc.name.c_str(), size);
    full.writePFM(buf);

    // vanilla RC + community "bilinear fix" (deterministic, 1 frame)
    Image vfix = renderReservoirRC(sc, cfg, prm, RCMode::VanillaFix);

    report("ref", ref, ref, sc);
    report("vanilla", van, ref, sc);
    report("vanfix", vfix, ref, sc);
    report(stoch ? "stoch" : "full", full, ref, sc);
    return 0;
}

// S3 (§5.4): dynamic light. Blink phase measures step-response convergence;
// static phase measures flicker and per-level lag-1 temporal autocorrelation
// (risk #1 red line: structural oscillation must decay with growing M).
static int s3(int argc, char** argv) {
    std::string scenePath = arg(argc, argv, "--scene", "scenes/s3.json");
    int size = std::atoi(arg(argc, argv, "--size", "128").c_str());
    int frames = std::atoi(arg(argc, argv, "--frames", "192").c_str());
    int blinkP = std::atoi(arg(argc, argv, "--blink", "32").c_str());
    uint64_t seed = std::strtoull(arg(argc, argv, "--seed", "1").c_str(), nullptr, 10);

    Scene sc = Scene::load(scenePath, size);
    CascadeCfg cfg = CascadeCfg::make(size);
    const MaskRect* mask = findMask(sc, "floor");
    if (!mask) { std::fprintf(stderr, "scene needs a 'floor' mask\n"); return 2; }

    ::mkdir("out", 0755);
    char buf[512];
    std::snprintf(buf, sizeof buf, "out/%s_%d_ref4096.pfm", sc.name.c_str(), size);
    Image refOn;
    if (!Image::readPFM(buf, refOn)) {
        refOn = renderReference(sc, size, 4096);
        refOn.writePFM(buf);
    }
    double Eon = maskEnergy(refOn, mask);

    RParams prm;
    prm.frames = frames;
    prm.seed = seed;
    prm.temporal = true;
    prm.mcap0 = std::atoi(arg(argc, argv, "--mcap0", "8").c_str());
    prm.rho = std::atof(arg(argc, argv, "--rho", "0.25").c_str());
    prm.boundaryJitter = !flag(argc, argv, "--no-bjitter");
    prm.bjitterBlock = std::atoi(arg(argc, argv, "--bjitter-block", "8").c_str());

    std::printf("[s3] %s size=%d frames=%d blinkP=%d mcap0=%d rho=%.2f Eon(ref)=%.5f\n",
                sc.name.c_str(), size, frames, blinkP, prm.mcap0, prm.rho, Eon);

    // ---- blink run: square-wave emission on shape 0 ----
    std::vector<double> series;
    RenderHooks hooks;
    hooks.sceneAt = [&](int f) {
        Scene s2 = sc;
        if ((f / blinkP) % 2 == 1) s2.shapes[0].emission = Vec3();
        return s2;
    };
    hooks.onFrame = [&](int, const Image& img) {
        series.push_back(maskEnergy(img, mask));
    };
    // Renderer-supplied dirty flag at blink toggles (data-independent).
    // --no-dirty disables it (measures the ~cap-frames chain-limited turn-on).
    if (!flag(argc, argv, "--no-dirty"))
        hooks.sceneChanged = [&](int f) { return f > 0 && f % blinkP == 0; };
    renderReservoirRC(sc, cfg, prm, RCMode::Full, &hooks);

    FILE* csv = std::fopen("out/s3_series.csv", "w");
    if (csv) {
        std::fprintf(csv, "frame,E,lightOn\n");
        for (size_t f = 0; f < series.size(); f++)
            std::fprintf(csv, "%zu,%.6f,%d\n", f, series[f],
                         ((f / blinkP) % 2 == 1) ? 0 : 1);
        std::fclose(csv);
    }

    // step-response on a 4-frame trailing mean: frames to enter ±10%·Eon band
    std::vector<double> smooth(series.size());
    for (size_t f2 = 0; f2 < series.size(); f2++) {
        int a = (int)f2 - 3 < 0 ? 0 : (int)f2 - 3;
        double s2 = 0;
        for (int g = a; g <= (int)f2; g++) s2 += series[g];
        smooth[f2] = s2 / (f2 - a + 1);
    }
    std::printf("  step response, smoothed (band ±10%% Eon):");
    for (int b = blinkP; b + 4 < frames; b += blinkP) {
        bool on = ((b / blinkP) % 2 == 0);
        double target = on ? Eon : 0.0;
        int conv = -1;
        for (int f2 = b; f2 < std::min(frames, b + blinkP); f2++) {
            if (std::fabs(smooth[f2] - target) <= 0.1 * Eon) { conv = f2 - b; break; }
        }
        std::printf(" %s:%d", on ? "ON" : "OFF", conv);
    }
    std::printf("  (frames; -1 = not converged within half-period)\n");

    // ---- static run: flicker + per-level lag-1 autocorrelation ----
    TemporalProbe probe;
    RenderHooks hooks2;
    hooks2.probe = &probe;
    int warm = frames / 2;
    std::vector<double> m1((size_t)size * size), m2((size_t)size * size);
    int cnt = 0;
    hooks2.onFrame = [&](int f, const Image& img) {
        if (f < warm) return;
        for (size_t i = 0; i < img.px.size(); i++) {
            double l = lum(img.px[i]);
            m1[i] += l; m2[i] += l * l;
        }
        cnt++;
    };
    renderReservoirRC(sc, cfg, prm, RCMode::Full, &hooks2);

    double fsum = 0; long fcnt = 0;
    for (int y = 0; y < size; y++)
        for (int x = 0; x < size; x++) {
            if (!mask->contains(x + 0.5, y + 0.5)) continue;
            size_t i = (size_t)y * size + x;
            double mean = m1[i] / cnt;
            double var = std::fmax(0.0, m2[i] / cnt - mean * mean);
            if (mean > 1e-5) { fsum += std::sqrt(var) / mean; fcnt++; }
        }
    std::printf("  flicker (temporal CV, static, mask): %.4f\n",
                fcnt ? fsum / fcnt : 0.0);

    std::printf("  lag-1 autocorr per level (static window):");
    for (int n = 0; n < cfg.levels; n++) {
        double rsum = 0; long rcnt = 0;
        for (auto& sr : probe.series[n]) {
            if ((int)sr.size() <= warm + 2) continue;
            double mm = 0; int len = (int)sr.size() - warm;
            for (int f2 = warm; f2 < (int)sr.size(); f2++) mm += sr[f2];
            mm /= len;
            double v = 0, c = 0;
            for (int f2 = warm; f2 < (int)sr.size(); f2++) {
                double d = sr[f2] - mm;
                v += d * d;
                if (f2 + 1 < (int)sr.size()) c += d * (sr[f2 + 1] - mm);
            }
            if (v < 1e-12) continue;
            rsum += c / v; rcnt++;
        }
        std::printf(" L%d:%.3f(n=%ld)", n, rcnt ? rsum / rcnt : 0.0, rcnt);
    }
    std::printf("\n");
    return 0;
}

// compare (§6): flat world-space reuse vs. cascade at identical candidate
// budget, through one metric pipeline. Emits a CSV row per run:
//   scene,mode,radius,k,rho,mcap,seed,leakPct,recPct,mape,frameMape,cand,valid,
//   candLen,validLen
// candLen/validLen: total traced distance (px) — equal ray count is not equal
// work (cascade interval-bounded vs. flat full-length; see RayCounts).
// leakPct: shadow-mask excess energy as % of vanilla RC's excess (S1);
// recPct: room-mask energy as % of reference (S2); frameMape: mean per-frame
// (pre-averaging) MAPE over the post-burn-in window — the single-frame
// quality the accumulated MAPE hides.
static int compare(int argc, char** argv) {
    std::string scenePath = arg(argc, argv, "--scene", "");
    if (scenePath.empty()) { std::fprintf(stderr, "--scene required\n"); return 2; }
    std::string mode = arg(argc, argv, "--mode", "flat"); // flat|cascade
    int size = std::atoi(arg(argc, argv, "--size", "128").c_str());
    int frames = std::atoi(arg(argc, argv, "--frames", "128").c_str());
    int refRays = std::atoi(arg(argc, argv, "--ref-rays", "4096").c_str());
    uint64_t seed = std::strtoull(arg(argc, argv, "--seed", "1").c_str(), nullptr, 10);
    std::string outdir = arg(argc, argv, "--outdir", "out");
    std::string csvPath = arg(argc, argv, "--csv", "");
    ::mkdir(outdir.c_str(), 0755);

    Scene sc = Scene::load(scenePath, size);
    char buf[512];
    std::snprintf(buf, sizeof buf, "%s/%s_%d_ref%d.pfm", outdir.c_str(),
                  sc.name.c_str(), size, refRays);
    Image ref;
    if (!Image::readPFM(buf, ref)) {
        std::printf("  rendering reference (%d rays/px)...\n", refRays);
        ref = renderReference(sc, size, refRays);
        ref.writePFM(buf);
    }
    CascadeCfg cfg = CascadeCfg::make(size);
    Image van = renderVanillaRC(sc, cfg);

    const MaskRect* mShadow = findMask(sc, "shadow");
    const MaskRect* mRoom = findMask(sc, "room");

    RayCounts rays;
    int burnIn = frames / 2;
    double frameMapeSum = 0;
    int frameMapeCnt = 0;
    auto onFrame = [&](int f, const Image& img) {
        if (f < burnIn) return;
        frameMapeSum += mape(img, ref, nullptr);
        frameMapeCnt++;
    };

    FlatParams fp;
    fp.frames = frames;
    fp.seed = seed;
    fp.radius = std::atof(arg(argc, argv, "--radius", "8").c_str());
    fp.k = std::atoi(arg(argc, argv, "--k", "4").c_str());
    fp.rho = std::atof(arg(argc, argv, "--rho", "0").c_str());
    fp.mcap = std::atoi(arg(argc, argv, "--mcap", "8").c_str());
    fp.bins = std::atoi(arg(argc, argv, "--bins", "0").c_str());
    fp.temporal = !flag(argc, argv, "--no-temporal");

    Image est;
    if (mode == "flat") {
        est = renderFlatReuse(sc, size, fp, &rays, onFrame);
    } else if (mode == "cascade") {
        RParams prm;
        prm.frames = frames;
        prm.seed = seed;
        prm.temporal = fp.temporal;
        prm.rho = fp.rho;
        prm.window = std::atoi(arg(argc, argv, "--window", "-1").c_str());
        prm.windowAuto = flag(argc, argv, "--window-auto");
        RenderHooks hooks;
        hooks.rays = &rays;
        hooks.onFrame = onFrame;
        est = renderReservoirRC(sc, cfg, prm, RCMode::Full, &hooks);
    } else {
        std::fprintf(stderr, "unknown --mode %s\n", mode.c_str());
        return 2;
    }

    double leakPct = -1, recPct = -1;
    if (mShadow) {
        double er = maskEnergy(ref, mShadow), ev = maskEnergy(van, mShadow),
               ee = maskEnergy(est, mShadow);
        if (ev - er > 1e-12) leakPct = 100.0 * (ee - er) / (ev - er);
    }
    if (mRoom) {
        double er = maskEnergy(ref, mRoom), ee = maskEnergy(est, mRoom);
        if (er > 1e-12) recPct = 100.0 * ee / er;
    }
    double mapeAll = mape(est, ref, nullptr);
    double fMape = frameMapeCnt ? frameMapeSum / frameMapeCnt : -1;

    std::printf("[compare] %s mode=%s r=%.0f k=%d rho=%.3f mcap=%d seed=%llu "
                "bins=%d: leak%%=%.2f rec%%=%.2f mape=%.3f fmape=%.3f "
                "rays: cand=%.0f/f valid=%.0f/f (+%.1f%%) "
                "traced px/f: cand=%.0f valid=%.0f (mean cand=%.2f px/ray)\n",
                sc.name.c_str(), mode.c_str(), fp.radius, fp.k, fp.rho,
                fp.mcap, (unsigned long long)seed, fp.bins,
                leakPct, recPct, mapeAll, fMape,
                (double)rays.cand / frames, (double)rays.valid / frames,
                rays.cand ? 100.0 * rays.valid / rays.cand : 0.0,
                rays.candLen / frames, rays.validLen / frames,
                rays.cand ? rays.candLen / rays.cand : 0.0);

    if (!csvPath.empty()) {
        FILE* csv = std::fopen(csvPath.c_str(), "a");
        if (csv) {
            std::fprintf(csv, "%s,%s,%.0f,%d,%.3f,%d,%llu,%.4f,%.4f,%.4f,"
                         "%.4f,%llu,%llu,%.1f,%.1f\n",
                         sc.name.c_str(), mode.c_str(), fp.radius, fp.k,
                         fp.rho, fp.mcap, (unsigned long long)seed, leakPct,
                         recPct, mapeAll, fMape,
                         (unsigned long long)rays.cand,
                         (unsigned long long)rays.valid,
                         rays.candLen, rays.validLen);
            std::fclose(csv);
        }
    }
    return 0;
}

// coverage: direct support-completeness oracle for the windowed lookup.
// Pure geometry, no rendering: for every level, probe positions spanning a
// parent cell (worst displacement at corners), anchor directions ω and
// content directions φ at cell edges/center (worst intra-cell separation),
// depths from the interval start (worst reprojection residual) to far field,
// and boundary-jitter extremes, check that the content's parent-side bin
//   b*_q(y) = binOf(n+1, dir(q→y))
// lies within the consulted window [bp_q − w, bp_q + w] anchored at the
// candidate's reprojection. Reports violations for the auto width w_n and
// for w_n − 1 (negative control: violations MUST appear there, proving the
// test has teeth). MAPE improvements cannot certify support completeness;
// this enumeration can.
static int coverage(int argc, char** argv) {
    int size = std::atoi(arg(argc, argv, "--size", "128").c_str());
    int wOverride = std::atoi(arg(argc, argv, "--window", "-999").c_str());
    CascadeCfg cfg = CascadeCfg::make(size);
    double jf[3] = {std::pow(4.0, -0.5), 1.0, std::pow(4.0, 0.5)};
    long long totalViol = 0, totalChecks = 0;
    std::printf("[coverage] size=%d levels=%d (w from --window-auto formula"
                "%s)\n", size, cfg.levels,
                wOverride != -999 ? " OVERRIDDEN" : "");
    for (int n = 0; n + 1 < cfg.levels; n++) {
        long long viol[2] = {0, 0}, checks = 0;
        int wAutoMax = 0;
        for (int ji = 0; ji < 3; ji++) {
            CascadeCfg cfgF = cfg;
            cfgF.t0 = cfg.t0 * jf[ji];
            const int Bp = cfg.bins(n + 1);
            double t1 = cfgF.intervalStart(n + 1);
            double t2 = cfgF.intervalEnd(n + 1);
            double g = std::sqrt(t2 / t1);
            int w = cfgF.coverageWindow(n);
            if (wOverride != -999) w = wOverride;
            // w ≥ Bp/2 ⇒ full ring: circular distance can never exceed it.
            if (w > wAutoMax) wAutoMax = w;
            double tRep = std::sqrt(t1 * t2);
            // Probe positions: 5×5 sub-grid across one parent cell in the
            // grid interior, plus one near the border (parent clamping folds
            // weights there — worst displacement geometry differs).
            int G = cfg.gridN(n);
            int pis[2] = {G / 2, 1};
            for (int pi = 0; pi < 2; pi++) {
                int i0 = pis[pi], j0 = pis[pi];
                for (int su = 0; su < 5; su++)
                    for (int sv = 0; sv < 5; sv++) {
                        Vec2 p = cfg.probePos(n, i0, j0);
                        p.x += (su / 4.0 - 0.5) * cfg.spacing(n) * 0.999;
                        p.y += (sv / 4.0 - 0.5) * cfg.spacing(n) * 0.999;
                        CascadeCfg::Parents par = cfg.parentsOf(n, p);
                        // Anchor/content directions: cell edges + center of
                        // every level-(n+1) cell.
                        double us[3] = {1e-6, 0.5, 1.0 - 1e-6};
                        for (int cb = 0; cb < Bp; cb += 1) {
                            for (int ua = 0; ua < 3; ua++)
                                for (int uc = 0; uc < 3; uc++) {
                                    double om = TWO_PI * (cb + us[ua]) / Bp;
                                    double ph = TWO_PI * (cb + us[uc]) / Bp;
                                    Vec2 zRep = p + Vec2{std::cos(om),
                                                         std::sin(om)} * tRep;
                                    double rs[7] = {1.0, 1.02, 1.2, g,
                                                    2 * g, 16.0, 1e5};
                                    for (int ri = 0; ri < 7; ri++) {
                                        double r = t1 * rs[ri];
                                        Vec2 y = p + Vec2{std::cos(ph),
                                                          std::sin(ph)} * r;
                                        for (int q = 0; q < 4; q++) {
                                            Vec2 qp = cfg.probePos(n + 1,
                                                par.idx[q][0], par.idx[q][1]);
                                            Vec2 tz = zRep - qp;
                                            Vec2 ty = y - qp;
                                            int bp = cfg.binOf(n + 1,
                                                std::atan2(tz.y, tz.x));
                                            int bs = cfg.binOf(n + 1,
                                                std::atan2(ty.y, ty.x));
                                            int d = bs - bp;
                                            d %= Bp; if (d < 0) d += Bp;
                                            int cd = d <= Bp - d ? d : Bp - d;
                                            checks++;
                                            if (cd > w) viol[0]++;
                                            if (cd > w - 1) viol[1]++;
                                        }
                                    }
                                }
                        }
                    }
            }
        }
        totalViol += viol[0];
        totalChecks += checks;
        std::printf("  L%d: w=%d checks=%lld violations=%lld (w-1: %lld)"
                    " %s\n", n, wAutoMax, checks, viol[0], viol[1],
                    viol[0] == 0 ? "OK" : "FAIL");
    }
    std::printf("[coverage] total: %lld/%lld violations %s\n", totalViol,
                totalChecks, totalViol == 0 ? "— support complete"
                                            : "— WIDTH INSUFFICIENT");
    return totalViol == 0 ? 0 : 1;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: rc selftest|eval|s3|compare|coverage "
                             "[options]\n");
        return 2;
    }
    if (!std::strcmp(argv[1], "selftest")) {
        std::string scenePath = arg(argc, argv, "--scene", "");
        int size = std::atoi(arg(argc, argv, "--size", "128").c_str());
        if (scenePath.empty()) { std::fprintf(stderr, "--scene required\n"); return 2; }
        return selftest(scenePath, size);
    }
    if (!std::strcmp(argv[1], "eval")) return eval(argc, argv);
    if (!std::strcmp(argv[1], "s3")) return s3(argc, argv);
    if (!std::strcmp(argv[1], "compare")) return compare(argc, argv);
    if (!std::strcmp(argv[1], "coverage")) return coverage(argc, argv);
    std::fprintf(stderr, "unknown command: %s\n", argv[1]);
    return 2;
}
