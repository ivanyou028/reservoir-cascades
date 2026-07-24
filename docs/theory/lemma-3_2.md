# Lemma 3.2 — Penumbra condition ⇒ bounded shift distortion

> **Context.** These notes accompany the Reservoir Cascades project (see the
> repo README): a Radiance Cascades merge re-cast as ReSTIR/GRIS-style
> resampling. References of the form "proposal §x" (including this document's
> own number, 3.2) point to the project's internal proposal document, not
> included in this repo. E-numbers cite the [lab log](../experiments.md);
> scenes S1 (thin occluder), S2 (pinhole + sub-bin source), S3 (dynamic
> light) are defined there and in `scenes/`. "Vanilla" = the deterministic
> RC baseline. ρ ∈ [0,1] is the validation probability (defined in §5).

Status: v0.2, 2026-07-23. The ε-δ treatments this document depends on are
complete — here and in [integrand-mismatch.md](integrand-mismatch.md)
(v0.2: full Lemma A topology, quantitative shell constants, grazing
measures from scene primitives, and a pointwise-tight / aggregate-refuted
resolution of the smooth-case √ε question). New in this revision: Prop J
(block-jitter formalization, §5b) and the explicit-constants table (§8).
This document supersedes proposal
§3.2 where they disagree (notably: the O(1)-bin claim is FALSE under ×4
angular branching — see Lemma D, §2 — and the E2/E3 empirics are now theory).

## 0. Setup and notation

2D flatland. Scene: finite union of C¹ shapes with at most K silhouette
points visible from any point (K bounded), emission bounded by L_max,
emitters' radiance view-independent (Lambertian-emissive).

Cascade discretization, all lengths in pixels:

- probe spacing   s_n = s₀·2ⁿ, probes on the grid (i+½)s_n
- angular bins    B_n = B₀·4ⁿ, bin width β_n = 2π/B_n ∝ 4⁻ⁿ
- radial interval I_n = [t_n, t_{n+1}), t_n = t₀(4ⁿ−1)/3, |I_n| = t₀4ⁿ

Key ratio (the "penumbra scale" of the discretization):

    ε_n := √2·s_{n+1}/t_{n+1} = (3√2·s₀/t₀)·2ⁿ⁺¹/(4ⁿ⁺¹−1) ≤ (4/3)·ε₀·2⁻ⁿ,
    ε₀ := 3√2·s₀/(2t₀)

(the 4/3 — attained at n=0 — is absorbed into the O(·)s below; note
Σ_n ε_n < (8/3)ε₀ < 3ε₀, the bound used by the assembled theorem)

√2·s_{n+1} bounds the child-to-parent displacement d = |p−q| (bilinear
support = parent-cell diagonal). ε_n decays geometrically: RADIAN-scale
parallax shrinks with level. This is the engine of every bound below.

Reconnection shift T_{q→p}: parent sample (ω_q, y), r_q = |y−q| ≥ t_{n+1};
keep y, set ω_p = dir(p→y), r_p = |y−p|. Note r_p ≥ r_q − d ≥ t_{n+1}(1−ε_n).

## 1. Lemma J (Jacobian bound)

The 2D angle-measure reconnection Jacobian through a surface point y with
surface angle θ_q, θ_p (between the surface normal at y and the directions
to q, p) is

    |dω_p/dω_q| = (r_q/r_p) · (cos θ_p / cos θ_q).

**Claim.** If θ_q ≤ θ_max (non-grazing), then

    J = 1 + O(ε_n),  explicitly  |J − 1| ≤ ε_n·(2 + tanθ_max) + O(ε_n²).

*Proof sketch.* |r_q/r_p − 1| ≤ d/r_p ≤ ε_n/(1−ε_n). The angle at y between
directions to p and q is φ ≤ arcsin(d/r_q) ≤ ε_n(1+O(ε_n)); then
|cosθ_p/cosθ_q − 1| ≤ φ·tanθ_max + O(φ²) away from grazing. ∎

Remarks. (i) The implementation uses J ≈ r_q/r_p (cos ratio dropped): same
O(ε_n) bound, and for view-independent emitters the dropped factor only
perturbs the *proposal* weights, not sample values — efficiency, not bias.
(ii) The grazing set {θ_q > θ_max} has small direction-measure (quantified
for polygonal vs smooth obstacles in [integrand-mismatch.md](integrand-mismatch.md)
Lemma G); in GRIS terms these samples get bounded-but-large J and are
handled by the confidence weights.

## 2. Lemma D (direction shift, two unit systems)

    |ω_p − ω_q| ≤ arcsin(d/r_q) ≤ ε_n · (1 + O(ε_n))      [radians]

so the radian shift decays 2⁻ⁿ. In *bin units* at the parent level:

    δ_n := |ω_p − ω_q| / β_{n+1} ≤ (4/3)ε₀·2⁻ⁿ · B₀4ⁿ⁺¹/(2π) = (8B₀ε₀/3π)·2ⁿ

**δ_n grows 2ⁿ.** Proposal §3.2's "bounded within O(1) angular bins" holds
only for B_n ∝ 2ⁿ branching; under the standard ×4 branching the shift is
unbounded in bin units. (Empirically ≈4 bins at n=3 — lab log [E3](../experiments.md); the δ_n
bound above allows up to ~11.)

## 3. Proposition R (reprojection residual)

Select the parent bin by the direction from q to the alignment point
z = p + t_c·ω, t_c = √(t_{n+1}t_{n+2}) (geometric mean, g := t_c/t_{n+1} ≈ 2).
For content at true depth r ∈ [t_{n+1}, ∞) along ω, the residual bin
misalignment is

    δ'_n(r) ≤ (B_{n+1}/2π) · d · |r − t_c| / (r·t_c),

which vanishes at r = t_c and is worst at the interval start:
δ'_n(t_{n+1}) = δ_n·(1 − 1/g) ≈ δ_n/2.

Consequences: (i) reprojection cancels parallax exactly at one depth and
helps most where content clusters near t_c; worst case improves only by the
constant (1−1/g); (ii) asymptotically δ'_n still grows 2ⁿ under ×4 branching
— reprojection buys practical cascade depths (with the E3 calibration,
δ' stays ≲2 bins through n ≈ 3–4, covering our prototypes' N = 4–5
levels; the 2ⁿ growth resumes beyond), not asymptotics. The asymptotic fixes are ×2 branching
(alignment-safe, resolution-poor) or windowed multi-bin lookup with MIS
(Prop W remedy, §4). This is a genuine design axis of the discretization.

## 4. Proposition C (bin misalignment is coverage BIAS, not variance)

The RIS merge consults exactly one parent bin per parent. Reconnection makes
every *consulted* sample a valid sample of p's field (value re-anchored, J
accounted), so misalignment never corrupts values. What it corrupts is
*coverage*: content lying, from the parent's vantage, in a bin that is never
consulted contributes zero proposal mass, and no MIS term compensates —
a systematic energy deficit (measured: 0.4–1.3× column striping, E3;
~1.3×/~0.7× residual banding, E4).

Formally: let A ⊂ Ω_b be the direction subset of the child bin whose
generating content falls outside the consulted parent bins. Then the merge
estimator targets ∫_{Ω_b \ A} instead of ∫_{Ω_b}, with |A| controlled by
δ'_n. **[ERRATUM 2026-07-23b, external review]** Controlled by δ′_n means
exactly that: |A| is O(ε_n) in radians, but as a FRACTION of the child bin
|A|/|Ω_b| = O(δ′_n) = Θ(2ⁿ) — saturating at O(1) by n ≈ 2–3 (§8 table:
δ′ = 1.0/1.5/2.8/5.4 at n = 0..3). The earlier reading of this as an
O(ε_n) per-bin bias conflated radians with bin fractions and is WRONG;
misalignment can lose (and misplace — E3's striping is bright AND dark)
an O(1) fraction of a bin at depth. The assembled theorem (§6) is
therefore stated for the windowed lookup, where coverage is exact and this
term is zero; single-bin + reprojection is the implemented approximation,
its residual measured (E3/E4), not certified. Remedy hierarchy:
reprojection (shrinks the residual, exact at one depth), windowed lookup
with per-bin MIS (Prop W below — the certified fix), boundary jitter
(converts the *radial* analogue of this gap into variance — E9); in 3D,
a ×2-per-axis direction schedule makes the shift Θ(1) in bin units and
removes the obstruction structurally — it is specific to one angular
dimension.

### Proposition W′ (windowed lookup, conditional-cell form — the LEGAL renormalization)

**[Restated 2026-07-23d to match the implementation exactly (external-review
gate); the earlier child-bin phrasing is superseded. Implemented in
restir.cpp (`--window`/`--window-auto`); certified by the `rc coverage`
oracle.]**

Structure: the outer candidate draws ω uniform on the child bin Ω_b;
conditional on ω, the windowed merge targets the tail restricted to ω's
**parent-width cell** C(ω) (the level-(n+1) bin containing ω). The child-bin
estimand is recovered by marginalizing over ω — the candidate's own
stratification — so the merge's target domain is C(ω), not Ω_b.

Per parent q, consult the bins {bp_q(ω)−w, …, bp_q(ω)+w} (bp_q = the
reprojected bin of Prop R, anchored at the candidate's ω). Accept a stored
sample y only if its RECONNECTED direction dir(p→y) lies in C(ω) — the
locality filter; reconnected directions are parallax-free (content along ω
reconnects to ω exactly; parallax lives only in the parent-side bin index),
so the filter is exact while the consult set widens. Say parent r *covers*
y iff b*_r(y) := binOf(n+1, dir(r→y)) lies in r's window; use MIS weights

    m_q(y) = β_q·1[covered_q(y)] / Σ_r β_r·1[covered_r(y)].

(i) These are a **legal GRIS Eq 17/20 partition of unity** over the
realizing techniques for C(ω): the covering set is support-determined and
per-y — unlike the validation-survivor renormalization (Prop V), which is
visibility-determined and NOT certifiable this way. *Coverage*
renormalization is legal because non-covering ⇒ non-realizing; *visibility*
renormalization is not, because occlusion ≠ non-realization.
(ii) **Coverage is restored — supp Y ⊇ C(ω)'s content at every depth —
once w ≥ δ'_n + intra-cell margin.** The margin (content φ and anchor ω
share C(ω) but differ by up to one bin, plus discrete boundary rounding)
is +2 at our defaults; in the non-paraxial regime ε_n ≥ 1 (level 0 at
extreme boundary jitter: parent displacement exceeds the interval start,
so dir(q→y) is unconstrained) NO finite window suffices and the width
escalates to the **full ring** (every parent bin once — coverage exact by
construction). Certified widths are geometry-computed
(`CascadeCfg::coverageWindow`) and validated by a direct enumeration
oracle (`rc coverage`): **zero violations over 12.7M/51.4M checks at
128²/256² including jitter extremes, with the negative control (w−1)
violating — the test has teeth.** MAPE improvements cannot certify support
completeness; this enumeration does.
(iii) Cost, honestly split into two modes:
  - **Certified mode** (`--window-auto`, the theorem configuration):
    per-level widths at the 128² defaults w = {16 (full ring), 5, 8, 13}
    → reads/parent {16, 11, 17, 27} ≈ 3–27× the single-bin lookup's; no
    extra rays. This is the price of exact coverage after the oracle's
    verdict — steeper than the pre-oracle estimate.
  - **Calibration mode** (small fixed w = 1–2, 3–5× reads): matches the
    E3-measured typical content shift, UNCERTIFIED — worst-case content
    (bin-boundary + interval-start) escapes it (oracle: violations at
    w−1). Partial windows remain unbiased on the covered set with the
    residual deficit shrunk to the still-uncovered sliver; they belong in
    the measured column, never beside the guarantee.
Empirically, widening beyond the certified width is value-neutral
(filtered slots contribute nothing — support-completion semantics), so
certified-vs-calibration differ in cost and worst-case guarantee, not in
typical-scene output (lab log E17: identical S2 results under both
widths).

## 5. Proposition V (renormalized validation — bias analysis)

Throughout, ρ ∈ [0,1] is the *validation probability*: at each merge, with
probability ρ a shadow ray from the child probe p re-checks visibility of
each parent's stored vertex y_q; valid_q is the outcome and S = {q : valid_q}
the survivor set (ρ=0: never validate; ρ=1: always). The derivation below is
complete at proof-sketch level and was re-derived independently from
scratch; two steps of an earlier sketch were wrong and are retained, flagged
**[was wrong]**, so the reader can see what changed. GRIS eq numbers per
[gris-anchoring.md](gris-anchoring.md).

**Central identity (exact).** Condition on the survivor set S = {q : valid_q}.
With w_q = m_q·p̂(c_q)·J_q·W_q, p_sel(q) = w_q/Σ_S w, W_sel = (Σ_S w)/p̂(c_sel),
the p̂ factor cancels:

    E[ tail | samples, S ] = Σ_{q∈S} (w_q/Σ_S w)·c_q·(Σ_S w/p̂(c_q))
                           = Σ_{q∈S} m_q J_q W_q c_q.                    (†)

(Sum over S is the code's sum over all q, since m_q=0 off S ⇒ w_q=0.)

**Renorm-before vs kill-after.**
- *Renorm-before-select* (ours): m_q = β_q/β_S, β_S := Σ_{q∈S} β_q. Then
  (†) = Σ_{q∈S} (β_q/β_S) J_q W_q c_q — weights summing to 1.
- *Kill-after-select* (an earlier implementation, since removed — lab log
  E2): m_q = β_q over ALL four, select, then zero an
  occluded selection. Then (†) = Σ_{q∈S} β_q J_q W_q c_q = **β_S · (renorm
  estimate) = v_p · (renorm estimate)**, where v_p = β_S is the *visible-β
  fraction*. So kill-after bias = −(1−v_p)·L_tail, an **O(1) multiplicative
  shrink**. (Measured, E2: S1 lit −16% ⇒ v_p≈0.84; S2 room −33% ⇒ v_p≈0.67.)

Toy model (2 parents, β=½ each, W=J=1, common p̂ ⇒ c₁=c₂, parent 1 visible from
p, parent 2 occluded, target = c₁): renorm-before ⇒ S={1}, m₁=1 ⇒ E[tail]=c₁
(exact); kill-after ⇒ ½·c₁ + ½·0 = ½c₁ (−50% = ×v_p).

**Crux: is the renorm-before residual truly O(ε_n)?** Yes — but the two easy
justifications are both **wrong**, and the correct argument needs a hypothesis.

- **[was wrong] "convex combination of ≈-unbiased estimators ⇒ unbiased".**
  The weights β_q/β_S have a *random denominator* (β_S depends on which parents
  survive), so the tail is a **self-normalized / ratio estimator** and
  E[A/B] ≠ E[A]/E[B]. Convexity alone gives nothing.
  *Correct mechanism = off-penumbra unanimity:* each parent's own visibility is
  already baked into its reservoir, so **outside** the p-vs-parent visibility-
  boundary shift (the penumbra symmetric-difference P', width O(ε_n) by
  Lemma D) the survivor set is **unanimous** — S = {all 4} (β_S=1, renorm is a
  no-op) or S = ∅ (merge outputs 0) — a *deterministic* denominator, hence no
  ratio bias. Ratio bias is confined to P'.
- **[was wrong] "measure O(ε_n) ⇒ bias O(ε_n)".** A measure-O(ε_n) bound on P'
  does **not** bound the *radiance-weighted* bias. Counterexample (two emitters,
  an occluder blocking [p,α] but not [p,β], parents storing α or β w.p. ½,
  c·W = 2C): renorm-before gives E[tail] = (15/16)·2C = 1.875C vs true C — a
  **+87% O(1) OVER-count**. This needs an emitter with O(1) sampling mass in an
  O(ε_n) strip, i.e. *unbounded* radiance density.
  *Resolution:* under the **bounded-radiance / reasonable-distribution premise**
  (Def 5.1 / Eq 27; per-level factor C_λ = (L_max+λ)/λ — see
[gris-anchoring.md](gris-anchoring.md) §2 for why the a.s. C_f compounds
with depth), the radiance-weighted probability of
  storing a sample in the O(ε_n) strip is itself O(ε_n), so S is unanimous
  w.p. 1−O(ε_n) and the residual collapses to O(L_max·ε_n) = O(ε_n).
- *Correlation-robustness.* The unanimity step bounds
  P(non-unanimous) ≤ P(∃q : stored y_q ∈ penumbra strip)
  ≤ Σ_q P(y_q ∈ strip) — a union bound, valid under **arbitrary**
  inter-parent correlation; independence is never invoked. Shared-ancestor
  correlation (the four parents descending from common grandparent
  reservoirs) makes stored samples coincide more often, pushing survivor
  verdicts toward agreement — the bound is conservative there, never
  violated. Empirical footnote: the flat-reuse control (lab log
  [E16](../experiments.md)) shows what happens when unanimity is
  *structurally absent* — validated sharing at a scale-mismatched radius
  splits the survivor set on O(1) of the domain and the ratio bias becomes
  O(1); the cascade's ∝2ⁿ radii are exactly the scale at which unanimity
  holds off O(ε_n) sets.

**[Scope note 2026-07-23c, second external review — sample-distribution
gap].** The unanimity step needs P(stored y_q ∈ penumbra strip) = O(ε_n)
in the radiance-weighted sense. That is immediate for FRESH candidates
(one selection round: the p̂-tilt costs one bounded factor C_λ). After
cascaded/temporal reselection the stored-location density can tilt by a
p̂-ratio per round, and bounded TRUE radiance alone does not obviously
control it. Repair path (open): route the argument through set-restricted
contribution mass — E[c·W·1_{y∈S}] tracks the true integral over S by
per-level restricted unbiasedness — rather than through selection
probabilities; until that is written out, the O(ε_n) claim below is proved
for merges whose parent samples are one selection round deep, and stated
as the target for the chained case.

**Conclusion.** The renorm-before mean bias is O(ε_n) — *contingent on bounded
radiance* (this hypothesis is load-bearing for the **mean**, not only the
variance; S2's near-point source is the stress case). Measured −2.2% (S1 lit,
ρ=1) is consistent as an order/sign check.

**ρ-mixture.** doValidate is a single Bernoulli(ρ) per merge, so the estimator
is a literal convex combination and

    bias(ρ) = (1−ρ)·bias_novalid + ρ·bias_renorm,   **exactly linear in ρ per merge**,

with bias_novalid = leak = +O(ε_n) (energy deposited where p is shadowed, from
reused c) and bias_renorm = −O(ε_n). Both O(ε_n) ⇒ bias(ρ) = O(ε_n) for all ρ.
**Numeric consistency:**
- The shadow-leak sequence (S1 E(shadow) as % of vanilla's) is
  27 / 19.3 / ~0 % at ρ = 0/.25/1 (lab log E5: single seed, no boundary
  jitter); the jittered multi-seed config gives 35.3 / 21.4 / 10.7 / 0.1 %
  at ρ = 0/.25/.5/1 (E12). Against either config, single-merge linearity
  over-predicts the interior (e.g. E5: 27 → predicted 20.25 vs measured
  19.3): the measured decay is **convex / faster-than-linear**, and
  27·(1−ρ)^1.33 fits the E5 interior better.
- The bow is a genuine second-order effect (cascade compounding across the ~2
  leak-zone levels → a (1−ρ)² admixture, OR within-merge survivor/c-value
  correlation) but does **not** uniquely fingerprint the mixture — the data does
  not discriminate. **[was wrong]** the earlier "near-linearity is a signature"
  claim is dropped.
- **−2.2% is the LIT-region deficit, a different region/normalization than the
  shadow-leak sequence** — do NOT use it as the ρ=1 leak floor (which is ~0).
- The leak leading constant (27% ≈ ε₁) is *calibrated, not derived*; only the
  (1−ρ) scaling and the O(ε_n) order are first-principles.

Kill-after vs renorm-before, in one line: kill-after normalizes over all four
(weights sum to v_p<1 ⇒ O(1) shrink); renorm-before normalizes over survivors
(weights sum to 1 ⇒ O(ε_n) under bounded radiance). This is why the
kill-after variant (since removed) lost 16–33% and the current
renorm-before implementation loses ~2%.

## 5b. Proposition J (block-consistent interval jitter)

Setting: block index β(f) = ⌊f/K⌋; splits ξ_β i.i.d. log-uniform,
t₀(ξ) = t₀·4^{ξ−1/2}; frame f runs the cascade with split ξ_{β(f)}; at
every block switch the temporal confidence is hard-reset (M := 0, history
dropped). This is the boundary-jitter scheme of lab log
[E9](../experiments.md). Let Ê_f denote frame f's gathered estimator and
b(ξ) the fixed-split systematic error (the interval-boundary bands of E4,
the shell/coverage terms of Lemma C / Prop C at split ξ).

(i) **No selection bias from switching.** The reset and all temporal-MIS
confidence weights are measurable w.r.t. (f, ξ) alone — independent of
every sample draw. Conditioned on ξ, the weights are deterministic
constants, so each frame keeps its fixed-split conditional expectation:

    E[ Ê_f | ξ ] = E_split(ξ_{β(f)})[ Ê ].

This is exactly the property candidate-triggered clamps destroy (their m
depends on the draw; measured +25% bias, E8) and frame-indexed resets
preserve.

(ii) **Mixture identity.** Averaging frames and taking expectation over ξ,

    E[ accumulated ] = E_ξ [ E_split(ξ)[ Ê ] ] = (split-free mean) + E_ξ[ b(ξ) ],

i.e. the deterministic per-split bands enter only through their ξ-average.
By Lemma C-avg ([integrand-mismatch.md](integrand-mismatch.md) §4), the
shell terms of E_ξ[b(ξ)] are O(ε_n) per level with a *universal*
constant — no transversality assumption; the csc α₀ blow-ups of
unluckily-tangent fixed splits are averaged away.

(iii) **What is NOT claimed.** E_ξ[b(ξ)] = 0 is not proved; what is proved
is the O(ε_n) bound on its shell component, and what is measured is the
fan-energy ratio moving 0.91 → 1.003 under jitter (E9). The deterministic
band is converted into per-frame variance whose magnitude frame-averaging
controls — bias→variance made literal, at the price that effective
temporal depth is capped at the block length K (history drops at
switches). K is thereby a genuine design dial (boundary-bias
decorrelation vs. temporal depth), not a nuisance parameter. ∎

## 6. Theorem (per-level reuse soundness — assembled)

Assumptions: (A1) bounded scene, ≤K silhouettes; (A2) reconnections
non-grazing outside a direction set of measure O(ε_n); (A3) β_q bilinear,
∑β = 1; (A4) targets p̂ = lum + λ, λ > 0.

Then for every level n, the neighbor-probe reconnection shift satisfies
J = 1 + O(ε_n), radian shift O(ε_n), and the shifted-integrand mismatch
(Lemma 3.2.I, [integrand-mismatch.md](integrand-mismatch.md))

    ∫ |f_p∘T·J − f_q| dω ≤ L_max·(C₂·ε_n^γ·|Ω| + 3C₁·(K+K_t)·ε_n),
    γ = 1 (polygonal) or ½ (smooth obstacles, grazing corridors)

(K_t = transversal obstacle-boundary crossings of the radius-t_{n+1}
circle; C₁, C₂ absolute constants — all per
[integrand-mismatch.md](integrand-mismatch.md)).

Consequently (anchoring to GRIS, Lin et al. 2022 — full mapping in
[gris-anchoring.md](gris-anchoring.md)), **for the windowed merge**
(Prop W with w ≥ δ′_n^max, coverage restored exactly) the per-level merge
is a GRIS instance whose finite-variance guarantee (Theorem 1) holds via
the λ-defensive reasonable-distribution bound (Def 5.1), and whose bias
has **two separable sources, both read as aggregates over the full
direction circle** (per-bin worst cases can saturate —
integrand-mismatch.md §9): (i) the shift distortion (Lemma 3.2.I /
Theorem I-b): O(ε_n) for smooth scenes, O(ε_n·log(1/ε_n)) for polygonal
ones (the flat-edge grazing log; a fixed non-grazing exclusion removes it
at the price of a scene-explicit excluded band where λ pays variance);
(ii) the value-passing / visibility gap attacked by ρ-validation (Prop V),
O(ε_n) *contingent on bounded radiance*. Coverage contributes **zero** by
construction. Both Σ_n ε_n < 3ε₀ and Σ_n ε_n·log(1/ε_n) < ∞ converge, so
the totals are **independent of N and scene scale**, and the reuse-radius
corollary — radius ∝ 2ⁿ from the penumbra condition — rides on the
reconnection lemmas alone, unaffected by the coverage question.

**[ERRATUM 2026-07-23b — external review]** The previous statement claimed
*three* O(ε_n) sources, including a single-bin coverage deficit of
relative size O(ε_n). That conflated radians with bin fractions: the
single-bin sliver fraction is O(δ′_n) = Θ(2ⁿ), saturating at O(1) by
n ≈ 2–3 (Prop C erratum, §4; gris-anchoring §3 erratum). The single-bin
reprojected merge used in all experiments is therefore an approximation
**outside this theorem's premise**: its coverage residual is the measured
E3/E4 striping/banding — empirical, not certified — and the windowed
configuration is the theorem-conforming one. Scope note on variance:
Theorem V (variance.md) covers the **single-frame** estimator, where
stored W ≡ 1 is structural (the per-level collapse); with temporal reuse
parents carry W ≠ 1 and the combined temporal×hierarchy bound is open
(variance.md §5; the flat control's stored-W divergence, lab log
[E16](../experiments.md), measures what unbounded chaining does).

**[CORRECTION]** the earlier draft anchored via "∑m=1" (a legal MIS partition).
That is wrong: β-renormalization is *not* a GRIS Eq 17/20 partition (it
renormalizes over the visibility-survivor set, not the per-y realizing set) but
an approximate visibility re-evaluation — see gris-anchoring.md §2 and Prop V.

Status: Lemmas J/D, Props R/C, Prop V, Lemma 3.2.I and the GRIS anchoring are
complete (with the bounded-radiance hypothesis made explicit); geometric
constants (|A|/|Ω_b|, the 27%≈ε₁ leak calibration) remain order-only.

## 7. Open problems

Resolved threads live in their own documents: the integrand-mismatch bound
in [integrand-mismatch.md](integrand-mismatch.md) (Lemma 3.2.I; v0.2 adds
the full Lemma A topology, quantitative shell constants, the grazing
measures from scene primitives, and settles the smooth-case √ε_n both
ways — pointwise tight by construction, O(ε_n) in aggregate), the GRIS
correspondence in [gris-anchoring.md](gris-anchoring.md), the variance side
in [variance.md](variance.md), Prop V/W in §4–§5 above, block-jitter
formalization in Prop J (§5b), and the explicit-constants table in §8
below. What remains open:

1. Temporal penumbra condition: characteristic angular velocity v/r ⇒ a
   principled M_n schedule (the *static* block-jitter statement is Prop J;
   the dynamic-scene schedule is a design question, not a correctness gap).
2. The coverage-sliver fraction |A|/|Ω_b| as a computed geometric integral
   over content depth (currently order-only; §8 tabulates its worst-case
   driver δ′_n). The ρ=0 leak calibration (~27% no-jitter E5, ~35%
   jittered-config E12; both ≈ ε₁) stays calibrated, not derived.
3. Windowed-lookup (Prop W) implementation and its measured cost/benefit —
   deferred to the 3D version of the method.
4. Whether the polygonal-case aggregate log factor
   ([integrand-mismatch.md](integrand-mismatch.md) Theorem I-b) is
   removable; further items in its §9 and [variance.md](variance.md) §5.

## 8. Explicit constants at the defaults (s₀=1, t₀=4, B₀=4)

ε₀ = 3√2·s₀/(2t₀) = 3√2/8 ≈ 0.5303. Exact per-level values (not the
(4/3)ε₀2⁻ⁿ bound), with the shift/misalignment chain evaluated:

| n | t_{n+1} | ε_n (exact) | bound (4/3)ε₀2⁻ⁿ | radian shift ≤ | δ_n (bins) | δ′_n = δ_n(1−1/g_n) |
|---|---------|-------------|-------------------|----------------|------------|----------------------|
| 0 | 4       | 0.7071      | 0.7071            | 0.7071         | 1.80       | 1.00                 |
| 1 | 20      | 0.2828      | 0.3536            | 0.2828         | 2.88       | 1.47                 |
| 2 | 84      | 0.1347      | 0.1768            | 0.1347         | 5.49       | 2.76                 |
| 3 | 340     | 0.0665      | 0.0884            | 0.0665         | 10.84      | 5.43                 |
| 4 | 1364    | 0.0332      | 0.0442            | 0.0332         | 21.66      | 10.83                |

δ_n = ε_n·B_{n+1}/(2π); g_n = √(t_{n+2}/t_{n+1}) → 1−1/g_n ≈ 0.55, 0.51,
0.503, 0.5007, 0.5002 (reprojection halves the worst case, exactly as
Prop R states). Σ_{n≥0} ε_n ≈ 1.274 < (8/3)ε₀ ≈ 1.414 < 3ε₀ ≈ 1.591.
Calibration cross-check (lab log [E3](../experiments.md)): measured raw
shift ≈ 4 bins at n=3 vs the 10.84 worst case (content sits nearer t_c
than the interval start); post-reprojection ≈ 2 bins, matching δ′ scaled
by the same ≈0.37 content factor. The worst-case column is what the
theorems consume; the measured column is what the implementation sees.
