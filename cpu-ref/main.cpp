// CPU reference driver.
//   rc selftest --scene S [--size N]           bit-exact degen-vs-vanilla (§3.6)
//   rc eval --scene S [--size N] [--frames F] [--seed X] [--no-temporal]
//           [--ref-rays K] [--outdir D]        renders ref/vanilla/full + metrics
#include "../core/cascade.h"
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
    bool stoch = flag(argc, argv, "--stoch");
    Image full = renderReservoirRC(sc, cfg, prm,
                                   stoch ? RCMode::StochasticRC : RCMode::Full);
    std::snprintf(buf, sizeof buf, "%s/%s_%d_full.ppm", outdir.c_str(),
                  sc.name.c_str(), size);
    full.writePPM(buf);
    std::snprintf(buf, sizeof buf, "%s/%s_%d_full.pfm", outdir.c_str(),
                  sc.name.c_str(), size);
    full.writePFM(buf);

    report("ref", ref, ref, sc);
    report("vanilla", van, ref, sc);
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

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: rc selftest|eval [options]\n");
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
    std::fprintf(stderr, "unknown command: %s\n", argv[1]);
    return 2;
}
