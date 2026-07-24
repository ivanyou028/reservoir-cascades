# Lab notebook — experiments

Format per entry: config / seed / metrics / one-line conclusion. The four
Go/No-Go criteria have tracking items at the end. (Translated from the
project's working log; section references "proposal §x" point to the
project's internal proposal document — the paper mirrors their content.)

---

## E1 — Degenerate-mode regression (proposal §3.6 / §12.2)

- Config: size=128, B0=4, t0=4px, levels=4; degenerate mode vs an
  independently hand-written vanilla RC.
- Verdict: S1 and S2 both **bit-exact** (0/16384 pixels differ).
  `make test` is the standing regression.
- Conclusion: the unified framework strictly contains vanilla RC as its
  degenerate case ✓.

## E2 — The "double counting" bias of validation rays (theory finding #1)

- Symptom: kill-after-select ρ-validation (validate the selected sample,
  zero it on failure) systematically loses energy in penumbra regions:
  S1 lit −16%, S2 room −33% (at ρ=1).
- Level-0-only validation ≈ all-level validation (same deficit) → rules out
  the "per-level compounding" hypothesis.
- Mechanism: the cascade chain already bakes visibility into sample
  *existence* (blocked chains resolve to c=0, folded into the W statistics);
  a naive kill multiplies in p's visibility a second time ⇒ penumbra scales
  like v_chain·v_p instead of v_p.
- Fix: the correct reading of the proposal's "account for it in MIS" —
  **validate before selecting + renormalize β over the surviving proposal
  set**. All four parents blocked ⇒ 0 (true umbra; hard leaks die); some
  blocked ⇒ survivors inherit the full β mass (no double discount).
- After the fix: S1 ρ=1 lit deficit 16% → 2.2%, leak stays 0.
- Paper material: validation's unbiasedness semantics in a cascade are not
  "multiply by visibility" but a domain restriction of the proposal set.

## E3 — Parallax bin misalignment (theory finding #2; overturns an assumption of proposal §3.2)

- Symptom: column striping inside the S2 fan (0.4–1.3×), even at ρ=0 with
  no temporal reuse.
- Mechanism: the proposal's `b' = childBin(s.ω)` uses one bin for all four
  parents. Under the (s×2, B×4, t×4) scaling, the parent-parallax direction
  shift grows ~2^n **in bin units** (≈4 bins at level 3) — the claim
  "direction shift bounded within O(1) angular bins" is **false** for this
  discretization (true in radians, false in bins: bin width shrinks 4^n,
  faster than parallax shrinks 2^n). Vanilla RC hides this behind its
  4-child-bin average + bilinear blur; single-bin RIS lookup pulls from a
  misaligned, dark bin.
- Fix: **per-parent reprojected bin selection** (the RIS version of the RC
  community's "bilinear fix"): z = p + ω·t_rep with t_rep = geometric mean
  of the parent interval √(start·end); bp_q = binOf(n+1, dir(q→z)).
- After the fix: S1 ρ=1 lit at 97.8%; S2 stripes reduced but not gone (E4).
- Paper material: the rigorous form of Lemma 3.2 must state the shift bound
  in bin units; the discretization scaling (angular ×2 vs ×4) plus
  reprojection is a design axis.

## E4 — Residual interval-boundary bias (open at M2)

- Config: S2, ρ=0, no-temporal, frames=256, seeds {1,7,42}.
- Seed-stable structural residual: ~1.3× overshoot just behind the wall,
  ~0.7× under-collection near the level-2→3 boundary (dist≈84px); total fan
  energy 91–95%. Remaining column-to-column fluctuation is spatially
  correlated noise (unstable across seeds).
- Hypothesized mechanism: when content straddles an interval boundary, the
  hand-off between near-segment hit statistics and far-segment tail does
  not close under inter-probe parallax (vanilla RC shows ringing in the
  same region).
- Candidate M2 fixes: stochastic boundary jitter (bias→noise) / overlapped
  intervals + MIS.

## E5 — M1 acceptance + Go/No-Go preview

Config: size=128, B0=4, t0=4px, levels=4, temporal (M_n=8·2^n, cap 64),
frames=64, seed=1, λ=0.05. Reference: 4096 rays/px stratified, analytic
intersections.

### S1 (thin occluder, tilt 0.05 rad, thickness 1.0px < s_1=2px)

| method | MAPE(all) | MAPE(shadow) | E(shadow) leak | E(lit) |
|---|---|---|---|---|
| reference | 0 | 0 | 0.00000 | 0.14776 |
| vanilla RC | 1.4427 | 11.85 | 0.01185 | 0.15087 |
| full ρ=0 | 0.5835 | 3.20 | 0.00320 (27%) | 0.14730 |
| full ρ=0.25 | 0.4176 | 2.29 | **0.00229 (19.3%)** | 0.14645 |
| full ρ=1 | 0.1208 | 0.00 | **0.00000 (0%)** | 0.14448 |

### S2 (pinhole + small source, radius 0.64px, sub-bin)

| method | MAPE(all) | MAPE(room) | E(room) | recovery |
|---|---|---|---|---|
| reference | 0 | 0 | 0.02839 | 100% |
| vanilla RC | 6.4634 | 9.87 | 0.03271 | 115% (structure destroyed: ringing snowflake) |
| full ρ=0 | 1.8253 | 3.13 | 0.02487 | 87.6% |
| full ρ=0.25 | 1.1071 | 1.92 | 0.02335 | **82.2%** |
| full ρ=1 | 0.1147 | 0.06 | 0.02064 | 72.7% |

### Conclusions

- **M1 acceptance: PASS.** Degenerate mode bit-exact ✓; S1 leak far below
  RC (19–100% reduction) with MAPE better across the board (2.5–12×) ✓.
- **A single config ρ=0.25 clears both GO-1 (leak <20%) and GO-2
  (recovery >80%).** ρ is a genuine bias/cost dial: S1 wants ρ high, S2
  wants ρ low; 0.25 clears both.
- GO-3 (S3 temporal) and GO-4 (ablation attribution) deferred to M2.

## Deviations from the proposal (registered)

1. CPU oracle uses analytic intersections (closed-form circle/box), not SDF
   sphere tracing — a correctness baseline wants exact hits; the WebGPU
   version (M2) does the SDF path for shadertoy-ecosystem parity.
2. Language: C++17 (no Rust toolchain on the machine); zero dependencies,
   PCG32 with fixed seeds for reproducibility.
3. Single-bin `childBin(s.ω)` lookup replaced by per-parent reprojection (E3).
4. ρ-validation semantics: validate-before-select + β renormalization, not
   kill-after-select (E2).
5. Intervals use geometric-series starts start_n = t0·(4^n−1)/3 (contiguous
   coverage from 0), not the literal t_n = t0·4^n (which leaves [0,t0)
   uncovered).

## E6 — Multi-seed error bars (GO-1/GO-2 margins, 2026-07-22)

Config: ρ=0.25, temporal, frames=64, seeds 1–8.
- S1 leak (% of vanilla): **18.0% ± 1.9%**, range [15.4, 21.0]
  (1/8 seeds marginally over the bar)
- S2 recovery: **86.2% ± 4.6%**, range [82.2, 94.9] (all seeds clear)
- Conclusion: GO-1 passes on the mean with the worst seed marginal; GO-2
  stable. ρ can be nudged (~0.3) to buy margin.

## E7 — Equal-ray ablation chain (GO-4 attribution, 2026-07-22)

seed=1, frames=64, three configurations at identical ray budget:

| config | S1 E(shadow) | S1 MAPE(all) | S2 E(room) | S2 MAPE(room) |
|---|---|---|---|---|
| StochasticRC (jitter + value passing, no RIS) | 0.01151 (=97% vanilla) | 2.39 | 0.03020 (overshoot) | 10.35 |
| full, no temporal (merge-as-RIS only) | 0.00228 | 0.40 | 0.02399 | 1.95 |
| full + temporal | 0.00229 | 0.42 | 0.02335 | 1.92 |

- **The leak is structural in the merge, not ray starvation**: the
  equal-ray jittered variant leaks ≈ vanilla.
- **Essentially the entire gain comes from merge-as-RIS**; temporal adds
  little on frame-averaged statics (frame averaging already accumulates).
  GO-4 holds on statics; temporal's value on dynamics is E8.

## E8 — S3 dynamic light (risk-1 red-line test + two temporal findings, 2026-07-22)

Scene: s3.json (circular light r=0.03 blinking square-wave P=32, thin bar
occluder, floor mask), frames=192, ρ=0.25.

**Red-line verdict: NOT triggered.** Static-segment flicker (temporal CV)
decays monotonically with M: mcap0=8: 0.89 → 32: 0.54 → 64: 0.40; no
structural oscillation that fails to decay with M. Lag-1 autocorrelation
rises with M (L0 0.85→0.91; increasing with level L0<L1<L2) = longer
memory, slower mixing — the proposal's predicted benign form ("slower
convergence, not variance explosion").

**Temporal finding #1 (fatal, fixed): naive luminance-target temporal RIS
cannot track dimming at all.** 48 frames after light-off the output was
still at 70% of the ON level (the target permanently prefers stale bright
samples). Fix = temporal re-evaluation: samples store immutable
(cRef, emitLum0); each frame c = cRef·(lum_now(y)/emitLum0). Light-off
response 2–3 frames; static scenes bit-identical; and samples can
**revive** (when the light returns, geometrically-still-valid old samples
recover instantly; ON jumps to ~0.3–0.5 immediately).

**Temporal finding #2 (open): brightening adaptation is M-limited.** After
ON the curve jumps (revival) then climbs slowly, not entering the ±10% band
within 32 frames (and decaying per cycle: dark samples progressively evict
revivable ones during OFF and accumulate "darkness confidence"). Tried a
candidate-triggered M-clamp (new 8× brighter than prev ⇒ M≤4): S3 ON
converges in 6–10 frames **but statics bias up +25% on S2** (recovery
82%→103%) — MIS weights depending on the sampled values ⇒ biased; reverted.
Principled options (M2+): scene-change-flag invalidation, temporal-gradient
MIS, dual-reservoir history. Honest current state: ON adaptation ~M frames,
same order as ReSTIR-family estimators without invalidation; OFF adaptation
2–3 frames (re-evaluation is exact).

## E9 — Interval-boundary jitter + scene-change invalidation (fixes for E4 and E8#2, 2026-07-22)

**Boundary jitter**: scale t0 log-uniformly by 4^±0.5 per frame (or per
block); boundaries sweep the whole inter-level gap; probe grids/bins/
indexing untouched. Three lessons:
1. **Per-frame jitter is incompatible with temporal reuse**: stored
   radiance bakes in the old split; mixing splits double-counts interval
   overlaps (measured 3× S1 leak inflation). Fix: splits fixed per block
   (default 8 frames), hard M reset at block switches (frame-indexed,
   data-independent ⇒ unbiased).
2. **Consistency check without temporal**: no-temporal + per-frame jitter
   gives S2 total fan energy 1.003 (was 0.91–0.95); E4's structural low
   band disappears.
3. Block length = the "boundary decorrelation rate vs temporal depth" dial:
   frames=64/block=16 gives only 2 split samples → ±24% swing across seeds;
   frames=128/block=8 (16 splits) → ±6%.

**Scene-change invalidation**: the renderer raises a dirty flag
(frame-indexed); that frame hard-resets M (=0). Unlike E8's reverted
candidate-triggered clamp this is data-independent ⇒ unbiased; S3 blink
converges in 3 frames both directions. Clamping to 4 is insufficient
(ON still fails under the revived/dark mixture); the hard reset is clean —
the same move production renderers make on disocclusion.

### Summary (frames=128, block=8, 6 seeds)

| config | S1 leak %vanilla | S2 recovery |
|---|---|---|
| ρ=0.25 | 21.4% ± 4.4 | **99.4% ± 6.0** (was 86.2 with a systematic deficit) |
| **ρ=0.5 (new recommended)** | **10.7% ± 2.4** ✓ | **93.5% ± 6.2** ✓ |
| ρ=1 | ~0% (≤0.2%) | — (~90%, not fully swept) |

### Two recommended configs

- **Offline/accumulating**: bjitter block=8, ρ=0.5 → all four GO bars clear
  with real margin, boundary-unbiased.
- **Real-time/streaming**: bjitter block=∞ (no switches in static segments,
  flicker 0.56) + scene-change hard reset → OFF:3 / ON:3 frames; boundary
  bias stays at single-split level. Future: per-level staggered splits may
  give both.

## E10 — WebGPU port validation (2026-07-22)

/webgpu: WGSL compute, fused kernel (trace + merge-as-RIS + temporal in one
dispatch per level, top-down), ping-pong reservoir buffers, SDF sphere
tracing (shadertoy parity — not the CPU's analytic intersections), scene
JSONs shared with the CPU side. 256², f32.
Run locally: `python3 -m http.server 8123` at repo root → /webgpu/index.html.

- Measured 120 fps (browser vsync-capped) @ 256²·4 bins·5 levels, Apple
  silicon.
- Metrics in the CPU oracle's regime (live mask energies, ρ=0.5,
  single-frame, no accumulation):
  S1 E(shadow)=0.00015 (leak≈0), E(lit)=0.143;
  S2 E(room)=0.02864 vs ref 0.02839 (**100.9% recovery**);
  S3 blink E(floor)≈0.0369 vs Eon 0.0377.
- Vanilla mode reproduces every expected failure shape on GPU (S2 ringing
  snowflake + blobbed room fan), one click away from full mode — the
  demo's core image.
- WebGPU pitfalls hit during the port (recorded for the next person):
  binding the same buffer read_write + read in the top-level merge silently
  invalidates the whole command buffer (all-black); layout:'auto' exposes
  only the bindings each entry point actually uses.

## E11 — The "leak ring": spatially localizing interval-boundary leak (2026-07-23, found by the author in the demo)

Symptom: WebGPU demo, S2, ρ=0 + single split (block=9999): a faint arc of
ghost light (periodic blobs) appears in the shadow region.

Diagnosis (CPU oracle, 256², ρ=0, no jitter, seed 3): for every leaking
pixel compute t_w, the distance along its sightline-to-source at which it
crosses the wall; the histogram spikes **6–8× at t_w ∈ [80,88) (604 px vs
~70–90 in neighboring bands)** — exactly t₃=84, the level-2→3 interval
boundary; a secondary bump sits at t₂=20. Mechanism: for pixels on the arc,
level-3 parents sit closer than 84px to the wall, so the parents' own
interval [84,340) **starts beyond the wall** — their rays hit the source
untested and store bright samples; the child chain reconnects that energy
in, and at ρ=0 nothing ever checks p→y. Blob periodicity = parent-grid
beat. This is precisely the annulus band of
[theory/integrand-mismatch.md](theory/integrand-mismatch.md) Lemma C plus
GRIS's unoccluded-target gap, made visible.

Suppression check (leak pixels inside the t₃ band): ρ=0 no-jitter 604;
ρ=0.5 no-jitter 298 (−51%); ρ=0 block-8 jitter 306 (−49%; more pixels but
dimmer — a literal bias→variance demonstration); at ρ=1 total leak ~0.1%.

Disposition: not a bug but an exhibit — the demo's explainer gained a
"Try this: drag ρ to 0" paragraph presenting the arc as a live
demonstration of the paper's central failure mode.

## E12 — Figure sweeps: ρ- and cap-dependence (2026-07-23)

The data behind the paper's measurement figures; every number regenerable
via `build/rc` (commands in the repo history).

**ρ sweep** (128², frames=128, block-8 jitter, temporal, seeds 1–6,
mean±sd; S1 leak as % of vanilla's, S2 recovery as % of reference):

| ρ | 0 | 0.125 | 0.25 | 0.375 | 0.5 | 0.75 | 1 |
|---|---|---|---|---|---|---|---|
| S1 leak % | 35.3±8.2 | 27.8±5.9 | 21.4±4.5 | 15.8±3.4 | 10.7±2.4 | 3.7±0.9 | 0.1±0.1 |
| S2 recovery % | 107.2±5.9 | 103.0±5.8 | 99.4±6.1 | 96.0±6.0 | 93.5±6.2 | 89.2±6.5 | 86.4±6.9 |

Note: this jittered config's ρ=0 leak (35.3%) exceeds E5's no-jitter 27% —
the split ensemble includes leakier splits; both are ≈ ε₁ in order. (The
E9 summary's ρ=0.5 recovery is corrected here to 93.5±6.2; an earlier
aggregation reported 94.0.)

**Cap sweep** (S3 static segment, block=∞, frames=192, ρ=0.25):

| cap | 2 | 4 | 8 | 16 | 32 | 64 |
|---|---|---|---|---|---|---|
| flicker CV | 1.330 | 1.096 | 0.853 | 0.724 | 0.558 | 0.395 |
| L0 lag-1 autocorr | 0.645 | 0.769 | 0.848 | 0.887 | 0.905 | 0.918 |

Log-log fit of flicker vs (1+cap): exponent ≈ 0.38±0.05 (the AR(1) model
of [theory/variance.md](theory/variance.md) §4 predicts ½ and the direction
of every deviation).

## E13 — ρ-dial S2 deficit decomposition (2026-07-23, paper revision)

- Config: S2 256², frames=128, seed 3; pairs (ρ=0, ρ=1) under frozen split
  and block-8 jitter. Metric: where does the energy that validation removes
  live? (Differenced renders, fan mask, distance-to-fan-boundary bands.)
- Validation-removed energy = 5.8% of ref (frozen) / 6.8% (jitter);
  **66.6% / 63.7% of it within 2 px of the fan's penumbra boundary,
  85.2% / 83.2% within 4 px** (band areas 18.8% / 36.7%); fan core is
  validation-neutral (**+0.04%** frozen / +1.07% jitter).
- Separate ρ-INdependent core deficit −4.4% (frozen split only) = the
  interval-boundary bias (E4's residual; jitter's job per E9).
- Conclusion: the ρ-dial's S2 recovery cost is Prop-validation's O(ε)
  penumbra-set residual, concentrated where S2 stores an O(1) energy
  fraction (the Assumption-1 stress test, by design) — not an O(1)
  estimator defect. → paper §5 Prop 3 cross-ref + §6 main results.

## E14 — ray-cost accounting (2026-07-23)

- RayCounts hooks in core/restir.{h,cpp}; eval prints rays/frame.
  Candidates: 262144/frame at 128² in EVERY mode (= vanilla's budget).
- Validation shadow rays (the only extra cost of ρ>0; segment-bounded,
  start t_{n+1}): ρ=0.25 +8.4% (S1) / +13.2% (S2); ρ=0.375 +12.6% / +19.8%;
  ρ=0.5 +16.7% / +26.4%; ρ=1 +33.4% / +52.8%.
- ρ=0.375 is the cheapest setting clearing both GO bars (E12: S1 15.8±3.4,
  S2 96.0±6.0) → "equal-cost" anchor for the paper: both bars cleared at
  ≤20% extra rays. → §6 cost statement, discussion cost axis.

## E15 — vanilla RC + community "bilinear fix" baseline (2026-07-23)

- RCMode::VanillaFix: deterministic value-passing merge, per-parent
  per-sub-bin reprojection through t_rep = √(t_{n+1}t_{n+2}) (the same
  reprojection Full uses for its RIS lookup). Selftest untouched (bit-exact
  degen ✓). eval now reports a `vanfix` row.
- S1: leak E(shadow) = 0.01124 = **94.9% of vanilla's** (0.01185);
  MAPE(all) 1.26 vs 1.44; MAPE(lit) halves (1.14 → 0.64).
- S2: E(room) = 0.03239 (114% of ref, structure still wrong);
  MAPE(room) 8.82 vs vanilla 9.87 (ours: 1.11 at ρ=0.5 seed 1).
- Conclusion: the community fix removes only the *misalignment* component
  (Lemma-shift's subject — hence lit-region MAPE halves); the leak and the
  angular-resolution ceiling are *merge-semantics* bias, untouched by
  realignment and removed by merge-as-RIS. → Table 1 row + related work.

## E16 — Flat world-space reuse control (equal budget, 2026-07-23, paper revision)

- Motivation: the sharpest reviewer question a hierarchical estimator
  faces — "does the cascade structure itself earn its overhead, or would
  any world-space reservoir reuse do?" Previously there was no real
  competing baseline.
- Implementation: `core/flat.{h,cpp}` + the `rc compare` subcommand +
  `scripts/flat_sweep.sh` (420 runs: 2 scenes × {cascade ρ∈{0,.25,.5}} ∪
  {flat r∈{0,2,4,8,16,32} × ρ∈{0,.5,1} × cap∈{8,32}} × 6 seeds ×
  128 frames). Fairness contract: (a) equal candidate budget
  (B_flat = 4·levels = 16 at 128² → 262,144 rays/frame, ray-for-ray equal
  to the cascade); (b) flat inherits our temporal machinery verbatim
  (immutable-cRef re-evaluation / revival / confidence cap); (c) spatial
  reuse is standard ReSTIR (disk kernel k=4, reconnection + Jacobian,
  renormalize-before-select validation) with **per-candidate generalized
  balance MIS** — the first pass used naive M-weighting, whose darkening
  bias *masked* the leak (S2 r4 ρ0: 64.7% "recovery" naive → 132.9%
  over-count under fair MIS); the naive variant was discarded to avoid a
  strawman baseline.
- Results (cap 8, 6 seeds; full table in `out/flat_sweep_agg.json` after
  running the sweep):
  - r=0 (temporal-only, zero sharing): S1 leak 0 / MAPE 0.061; S2 rec
    100.4%. At equal ray *count*, per-pixel brute force wins static
    frame-averaged quality — but every ray is full-length (~24× nominal
    path budget; free under analytic intersections, not under DDA/BVH),
    nothing is amortized, and per-frame ≈ accumulated quality (no
    per-frame win).
  - Sharing without validation (ρ=0): catastrophic leak — r2 175%,
    r4 793%, r8 2819%, r16 4305% of vanilla's leaked energy (1.7–43×),
    worsening monotonically with r and cap.
  - Sharing + full validation (ρ=1): leak dies, but S2 recovery collapses
    for r≥4 (r4 59%, r8 27%, r16 16%) — the survivor set splits on O(1)
    of the domain when the kernel spans a visibility boundary, an O(1)
    ratio-estimator bias = **measured confirmation of Prop V's unanimity
    mechanism** (the cascade's ∝2ⁿ radii are exactly the scale at which
    unanimity holds).
  - The only flat configs holding both scenes confine sharing to r=2
    (barely sharing): ρ=1 leak 0 / rec 92.5%, at +64–99% full-length
    shadow rays; vs cascade ρ=0.5: leak 10.7 / rec 93.5, at +18–28%
    interval-bounded rays with reuse radii growing to ~23 px at the
    deepest level.
  - cap=32 + sharing: **diverges** (S1 r8 leak ~4669× vanilla; MAPE grows
    exponentially in frame count: 42→145→757→2646 at 16/32/64/128
    frames). Mechanism: stored contribution weights multiply across
    frames and random-walk — precisely the pathology the per-level W
    collapse (§3.5) eliminates, now confirmed by the control.
- Conclusion: three failure axes (leak / validation collapse / W-chain
  divergence); flat hits every one, the cascade closes each structurally.
  Paper: new Table tab:flat + the "Does the cascade earn its structure?"
  paragraph; related-work world-space paragraph gains the control
  sentence.
- Honesty note: equal ray count ≠ equal work (flat rays are full-length);
  2D analytic scenes hide that gap — stated explicitly in the paper.

## E17 — Windowed lookup (Prop W′) implemented + certified (2026-07-23, erratum response)

- Motivation: after the coverage-fraction erratum the soundness theorem is
  stated for the windowed configuration — which had never been implemented.
  "The theorem validates a method that was never run" was the largest
  remaining objection (external review concurred on priority).
- Implementation: new branch in restir.cpp Full mode; `--window W` fixed
  half-width / `--window-auto` per-level certified width from
  `CascadeCfg::coverageWindow` (shared by code and oracle — no drift);
  `RayCounts.reads` counts amplification. Off by default; bit-exact
  regression green throughout.
- Three design iterations, each falsified by the oracle within minutes:
  v1 integrated the whole child bin without the bin-width ratio (26× energy
  explosion); v2 added the 0.25 factor but deposited bin-mean tails,
  destroying tail angular resolution (4.6× S1 leak at ρ=0); v3 FINAL —
  the window is a SUPPORT-COMPLETION device, not an integration device:
  the tail stays local to the candidate's parent-width cell (reconnected
  directions are parallax-free; parallax lives only in the parent-side bin
  index), the window only recovers depth-shifted content.
- **Coverage oracle** (`rc coverage`, pure geometric enumeration: probe
  positions across a parent cell, anchor/content directions at cell edges,
  depths from the interval start to 1e5×, boundary-jitter extremes):
  the first run BIT — w = ceil(δ') leaves violations (intra-cell offset +
  discrete rounding); margin +2 still failed level 0 → root cause: at
  extreme jitter ε₀ = 1.41 > 1, the geometry is non-paraxial and NO finite
  window suffices → escalate to full-ring consult (dedup). Final:
  **zero violations over 12.7M (128²) / 51.4M (256²) checks; the negative
  control (w−1) violates — the test has teeth.** Certified widths
  w = {16 (full ring), 5, 8, 13}, reads/parent {16, 11, 17, 27} (~3–27×):
  steeper than pre-oracle estimates, reported as-is.
- **ρ-separation test** (128², 64 independent frames averaged,
  no temporal, seed 1; single-bin → windowed-auto):
  S1 leak% 31.4→37.3 (ρ=0) / 10.5→12.0 (ρ=.5) / **0.01→0.15 (ρ=1: the
  windowed increment is eliminated by validation)**; S2 rec%
  102.9→98.9 (closer to 100) with MAPE better at every ρ (1.86 vs 2.12
  at ρ=0; banding config 1.53 vs 2.05, −25%). The two error sources
  (coverage vs visibility) separate cleanly, as the two-source theorem
  predicts. Widening beyond the certified width leaves results
  bit-identical (filtered slots contribute nothing) — direct evidence of
  the support-completion semantics.
- Naming: all E17 metrics are averages over 64 INDEPENDENT frames
  (no temporal reuse) — not "single-frame quality".
- Remaining: multi-seed × ρ × temporal × worst-case-geometry sweep;
  paper table with a proved-vs-measured split.

## Go/No-Go tracking (proposal §9; decided at end of M2)

- [x] GO-1 S1 leak < 20% of vanilla: multi-seed **18.0%±1.9%** at ρ=0.25
      (1/8 seeds at 21.0%, marginal; ρ buys margin — 10.7%±2.4 at ρ=0.5)
- [x] GO-2 S2 recovery > 80%: multi-seed **86.2%±4.6%** (99.4/94.0 with
      boundary jitter), all seeds clear
- [x] GO-3 S3: flicker decays monotonically with M (0.89→0.40), **no
      structural oscillation — red line not triggered**; note: ON
      adaptation ~M frames (E8 finding #2, not a red-line item)
- [x] GO-4 merge-as-RIS is the main gain: equal-ray ablation (E7) —
      StochasticRC ≈ vanilla; RIS alone contributes ~all improvement
