# Lemma 3.2 — Penumbra condition ⇒ bounded shift distortion

Status: v0.1, 2026-07-22. Statements and proof sketches; items marked TODO
need the full ε-δ treatment for the paper. This document supersedes proposal
§3.2 where they disagree (notably: the O(1)-bin claim is FALSE under ×4
angular branching — see Prop. 3 — and the E2/E3 empirics are now theory).

## 0. Setup and notation

2D flatland. Scene: finite union of C¹ shapes with at most K silhouette
points visible from any point (K bounded), emission bounded by L_max,
emitters' radiance view-independent (Lambertian-emissive).

Cascade discretization, all lengths in pixels:

- probe spacing   s_n = s₀·2ⁿ, probes on the grid (i+½)s_n
- angular bins    B_n = B₀·4ⁿ, bin width β_n = 2π/B_n ∝ 4⁻ⁿ
- radial interval I_n = [t_n, t_{n+1}), t_n = t₀(4ⁿ−1)/3, |I_n| = t₀4ⁿ

Key ratio (the "penumbra scale" of the discretization):

    ε_n := √2·s_{n+1} / t_{n+1}  ≤  (3√2·s₀/t₀)·2⁻ⁿ⁻¹ =: ε₀·2⁻ⁿ

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

    J = 1 + O(ε_n),  explicitly  |J − 1| ≤ ε_n·(1 + (1+tanθ_max)) + O(ε_n²).

*Proof sketch.* |r_q/r_p − 1| ≤ d/r_p ≤ ε_n/(1−ε_n). The angle at y between
directions to p and q is φ ≤ arcsin(d/r_q) ≤ ε_n(1+O(ε_n)); then
|cosθ_p/cosθ_q − 1| ≤ φ·tanθ_max + O(φ²) away from grazing. ∎

Remarks. (i) The implementation uses J ≈ r_q/r_p (cos ratio dropped): same
O(ε_n) bound, and for view-independent emitters the dropped factor only
perturbs the *proposal* weights, not sample values — efficiency, not bias.
(ii) The grazing set {θ_q > θ_max} has direction-measure O(K·(π/2−θ_max)⁻¹…):
TODO formalize via the silhouette-count assumption; in GRIS terms these
samples get bounded-but-large J and are handled by the confidence weights.

## 2. Lemma D (direction shift, two unit systems)

    |ω_p − ω_q| ≤ arcsin(d/r_q) ≤ ε_n · (1 + O(ε_n))      [radians]

so the radian shift decays 2⁻ⁿ. In *bin units* at the parent level:

    δ_n := |ω_p − ω_q| / β_{n+1} ≤ ε₀·2⁻ⁿ · B₀4ⁿ⁺¹/(2π) = (2B₀ε₀/π)·2ⁿ

**δ_n grows 2ⁿ.** Proposal §3.2's "bounded within O(1) angular bins" holds
only for B_n ∝ 2ⁿ branching; under the standard ×4 branching the shift is
unbounded in bin units. (Empirically ~4–11 bins at n=3, experiments E3.)

## 3. Proposition R (reprojection residual)

Select the parent bin by the direction from q to the alignment point
z = p + t_c·ω, t_c = √(t_{n+1}t_{n+2}) (geometric mean, g := t_c/t_{n+1} = 2).
For content at true depth r ∈ [t_{n+1}, ∞) along ω, the residual bin
misalignment is

    δ'_n(r) ≤ (B_{n+1}/2π) · d · |r − t_c| / (r·t_c),

which vanishes at r = t_c and is worst at the interval start:
δ'_n(t_{n+1}) = δ_n·(1 − 1/g) = δ_n/2.

Consequences: (i) reprojection cancels parallax exactly at one depth and
helps most where content clusters near t_c; worst case improves only by the
constant (1−1/g); (ii) asymptotically δ'_n still grows 2ⁿ under ×4 branching
— reprojection buys practical cascade depths (N ≤ 6–7 with our constants
keeps δ' ≲ 1–2 bins), not asymptotics. The asymptotic fixes are ×2 branching
(alignment-safe, resolution-poor) or windowed multi-bin lookup with MIS
(Prop. 4 remedy). This is a genuine design axis of the discretization.

## 4. Proposition C (bin misalignment is coverage BIAS, not variance)

The RIS merge consults exactly one parent bin per parent. Reconnection makes
every *consulted* sample a valid sample of p's field (value re-anchored, J
accounted), so misalignment never corrupts values. What it corrupts is
*coverage*: content lying, from the parent's vantage, in a bin that is never
consulted contributes zero proposal mass, and no MIS term compensates —
a systematic energy deficit (measured: 2× column striping, E3/E4).

Formally: let A ⊂ Ω_b be the direction subset of the child bin whose
generating content falls outside the consulted parent bins. Then the merge
estimator targets ∫_{Ω_b \ A} instead of ∫_{Ω_b}, i.e. bias
−∫_A L dω, with |A| controlled by δ'_n. Remedy hierarchy: reprojection
(shrinks |A|), windowed lookup with per-bin MIS (Prop W below), boundary
jitter (converts the *radial* analogue of this gap into variance — E9).

### Proposition W (windowed lookup restores coverage — the LEGAL renormalization)

Consult, per parent q, the bins {bp_q−w, …, bp_q+w}. Say parent q *covers*
direction y iff b*_q(y) := binOf(n+1, dir(q→y)) lies in q's window — a
purely geometric, deterministic predicate, computable at merge time from the
candidate's own y. Use MIS weights

    m_q(y) = β_q·1[covered_q(y)] / Σ_r β_r·1[covered_r(y)].

(i) These are a **legal GRIS Eq 17/20 partition of unity**: the covering set
is support-determined and per-y — exactly the "sum over realizing
techniques" restriction — unlike the validation-survivor renormalization
(Prop V), which is visibility-determined and therefore NOT certifiable this
way. The contrast is the sharpest way to state the difference: *coverage*
renormalization is legal because non-covering ⇒ non-realizing; *visibility*
renormalization is not, because occlusion ≠ non-realization.
(ii) **Coverage restored ⟺ w ≥ δ'_n^max** (Prop R residual): then
covered_q ≡ 1 for all q, m_q = β_q, and supp Y = supp p̂ — the Eq 15
violation of Prop C closes and the −∫_A L deficit vanishes.
(iii) Cost: (2w+1) reservoir *reads* per parent, no extra rays. With our
defaults, Prop R gives δ' ≲ 1–2 bins through N ≤ 7, so w = 1–2 suffices at
3–5× read cost. Partial windows (w < δ'^max) remain unbiased on the covered
set with the residual deficit shrunk to the still-uncovered sliver.

## 5. Proposition V (renormalized validation — bias analysis)

Full derivation cross-checked by independent re-derivation + adversarial audit;
the arguments below **replace** the earlier sketch, two steps of which were
wrong (flagged **[was wrong]**). GRIS eq numbers per
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
- *Kill-after-select* (reverted): m_q = β_q over ALL four, select, then zero an
  occluded selection. Then (†) = Σ_{q∈S} β_q J_q W_q c_q = **β_S · (renorm
  estimate) = v_p · (renorm estimate)**, where v_p = β_S is the *visible-β
  fraction*. So kill-after bias = −(1−v_p)·L_tail, an **O(1) multiplicative
  shrink**. (Measured: S1 lit −16% ⇒ v_p≈0.84; S2 room −33% ⇒ v_p≈0.67.)

Toy model (2 parents, β=½ each, W=J=1, common p̂ ⇒ c₁=c₂, parent 1 visible from
p, parent 2 occluded, target = c₁): renorm-before ⇒ S={1}, m₁=1 ⇒ E[tail]=c₁
(exact); kill-after ⇒ ½·c₁ + ½·0 = ½c₁ (−50% = ×v_p). Confirmed by independent
recomputation.

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
  (Def 5.1 / Eq 27, C_f ≈ (L_max+λ)/λ), the radiance-weighted probability of
  storing a sample in the O(ε_n) strip is itself O(ε_n), so S is unanimous
  w.p. 1−O(ε_n) and the residual collapses to O(L_max·ε_n) = O(ε_n).

**Conclusion.** The renorm-before mean bias is O(ε_n) — *contingent on bounded
radiance* (this hypothesis is load-bearing for the **mean**, not only the
variance; S2's near-point source is the stress case). Measured −2.2% (S1 lit,
ρ=1) is consistent as an order/sign check.

**ρ-mixture.** doValidate is a single Bernoulli(ρ) per merge, so the estimator
is a literal convex combination and

    bias(ρ) = (1−ρ)·bias_novalid + ρ·bias_renorm,   **exactly linear in ρ per merge**,

with bias_novalid = leak = +O(ε_n) (energy deposited where p is shadowed, from
reused c) and bias_renorm = −O(ε_n). Both O(ε_n) ⇒ bias(ρ) = O(ε_n) for all ρ.
**Numeric honesty (audit):**
- The shadow-leak sequence is 27 / 19.3 / 10.7 / ~0 % at ρ = 0 / .25 / .5 / 1.
  Pure single-merge linearity predicts 27 / 20.25 / 13.5 / 0 — endpoints exact,
  interior *over*-predicted. The measured leak is **convex / faster-than-linear**;
  27·(1−ρ)^1.33 fits the interior better.
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
(weights sum to 1 ⇒ O(ε_n) under bounded radiance). This is why the reverted
kill-after variant lost 16–33% and the shipped renorm-before loses ~2%.

## 6. Theorem (per-level reuse soundness — assembled)

Assumptions: (A1) bounded scene, ≤K silhouettes; (A2) reconnections
non-grazing outside a direction set of measure O(ε_n); (A3) β_q bilinear,
∑β = 1; (A4) targets p̂ = lum + λ, λ > 0.

Then for every level n, the neighbor-probe reconnection shift satisfies
J = 1 + O(ε_n), radian shift O(ε_n), and the shifted-integrand mismatch
(Lemma 3.2.I, [integrand-mismatch.md](integrand-mismatch.md))

    ∫ |f_p∘T·J − f_q| dω ≤ L_max·(C₂·ε_n^γ·|Ω| + 3C₁·(K+K_t)·ε_n),
    γ = 1 (polygonal) or ½ (smooth obstacles, grazing corridors).

Consequently (anchoring to GRIS, Lin et al. 2022 — full mapping in
[gris-anchoring.md](gris-anchoring.md)) the per-level merge is a GRIS instance
whose finite-variance guarantee (Theorem 1) holds via the λ-defensive
reasonable-distribution bound (Def 5.1), and whose bias has **three separable
O(ε_n) sources**: (i) the coverage deficit (Prop C = violation of GRIS's
coverage condition Eq 15, sign always negative); (ii) the shift distortion
(Lemma 3.2.I); (iii) the value-passing / visibility gap attacked by ρ-validation
(Prop V), whose renorm-before variant is O(ε_n) *contingent on bounded
radiance*. Since ε_n = ε₀2⁻ⁿ, the total over N levels is bounded by the
geometric series ∑ε_n < 2ε₀ **independent of N and scene scale** — the rigorous
form of "reuse radius ∝ 2ⁿ is a corollary, not a hyperparameter".

**[CORRECTION]** the earlier draft anchored via "∑m=1" (a legal MIS partition).
That is wrong: β-renormalization is *not* a GRIS Eq 17/20 partition (it
renormalizes over the visibility-survivor set, not the per-y realizing set) but
an approximate visibility re-evaluation — see gris-anchoring.md §2 and Prop V.

Status: Lemmas J/D, Props R/C, Prop V, Lemma 3.2.I and the GRIS anchoring are
complete (with the bounded-radiance hypothesis made explicit); geometric
constants (|A|/|Ω_b|, the 27%≈ε₁ leak calibration) remain order-only.

## 7. Open / TODO for the paper

1. ~~Integrand mismatch~~ → DONE, see [integrand-mismatch.md](integrand-mismatch.md)
   (Lemma 3.2.I). Two upgrades over the sketch here: reconnection needs NO
   Lipschitz term (same hit point ⇒ same value; only the Jacobian factor
   survives off-penumbra), and there is a polygonal/smooth dichotomy —
   O(K·ε_n) for polygonal scenes vs O(K·√ε_n) for smooth convex obstacles
   (grazing corridors). Residual gaps listed in its §9.
2. ~~Prop V: conditional-expectation decomposition~~ → DONE (§5 above,
   verified). Key results: identity (†); renorm-before = O(ε_n) *contingent on
   bounded radiance* via off-penumbra unanimity (the "convex ⇒ unbiased" and
   "measure ⇒ bias" shortcuts are both false); kill-after = −(1−v_p)L_tail
   O(1) shrink; ρ-mixture exactly linear per merge but measured convex.
3. ~~GRIS anchoring~~ → DONE, see [gris-anchoring.md](gris-anchoring.md).
   Full correspondence to Def 4.1/4.2/Eq 15–22/Def 5.1/Thm 1/A.4; Prop C = Eq 15
   coverage violation; the value-passing gap is GRIS's own Appendix B "On
   Visibility" (unoccluded target p̂⁻ⱽ, non-canonical reuse) — for which we
   supply the O(ε_n) bound GRIS left as "often imperceptible bias".
3b. Grazing-set measure bound (A2) from K and curvature bounds.
4. Temporal penumbra condition (proposal §3.7): characteristic angular
   velocity v/r ⇒ M_n schedule; formalize block-jitter unbiasedness (E9)
   as piecewise-stationary MIS.
5. ~~Windowed-lookup unbiasedness~~ → DONE (Prop W, §4 above): coverage
   restored ⟺ window ≥ δ'_n; the coverage renormalization is the LEGAL
   Eq 17/20 partition (support-determined), in pointed contrast to Prop V's
   visibility renormalization. Implementation is an M2+/3D-version item.
5b. Chain variance → DONE, see [variance.md](variance.md): selection noise
   = λ·μ per level (zero at λ=0, Lemma S); transfer factor bracketed
   [Σβ², 1]·(1+Cε_n)² under arbitrary parent correlation (Lemma T — risk #1
   can slow, never diverge); total ≤ e^{4Cε₀}·Σ injections, depth-uniform
   (Theorem V); temporal reservoir = AR(1) with coefficient cap/(1+cap),
   predicting E8's autocorr (.667 vs .651 measured at cap 2) and the
   flicker exponent (0.5 vs 0.38±0.05 measured).
6. Explicit constants for our defaults (s₀=1, t₀=4, B₀=4): ε₀ = 3√2/8 ≈ 0.53,
   per-level radian shift ≤ 0.53·2⁻ⁿ; bin misalignment table vs. n.
