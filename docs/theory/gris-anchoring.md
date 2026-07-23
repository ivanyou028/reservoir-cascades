# GRIS anchoring — the merge as a Generalized RIS instance

Status: v0.1, 2026-07-23. Companion to [lemma-3_2.md](lemma-3_2.md) (see
its header note for project context): this note supplies the GRIS
correspondence used by its §6 — anchoring the cascade merge to the exact
conditions of the GRIS theorems rather than a paraphrase. All equation
numbers below are Lin, Kettunen, Bitterli, Pantaleoni, Yuksel & Wyman 2022
(SIGGRAPH; "GRIS"). Statements that reverse an earlier internal draft are
flagged **[CORRECTION]**.

## 1. The correspondence (per probe p, child bin b, level n)

| GRIS object | Our merge |
|---|---|
| primary domain Ω | child bin Ω_b (directions ω_p at p, tail beyond t_{n+1}) |
| integrand f(ω) | L_{≥n+1}(p,ω), p's OWN tail field **with p's own visibility** |
| estimand | L̄_n(p,b) = (1/\|Ω_b\|)∫_{Ω_b} f, the 1/\|Ω_b\| an outer constant |
| target p̂(Y) | lum(c)+λ, λ>0, evaluated on the shifted sample's stored radiance |
| techniques i | parents q ∈ {0,1,2,3} (bilinear neighbours, level n+1) |
| source domains Ω_i | parent bins Ω_q (correlated inputs across children — GRIS allows) |
| input X_i | parent reservoir sample (ω_q, y_q, c_q, W_q); invariant coord = vertex y_q |
| shift T_i | reconnection T_{q→p}: keep y_q, set ω_p = dir(p→y_q) |
| Jacobian \|∂T_i/∂X_i\| | J_q = r_q/r_p = \|y_q−q\|/\|y_q−p\| (2D, first power); =1 if no y |
| UCW W_i | W_q, parent stored UCW (layered — W already collapsed at parent) |
| resampling MIS m_i | m_q = (valid_q ∧ β_S>0) ? β_q/β_S : 0, β_S := Σ_{q∈S}β_q |

Then w_q = m_q·p̂(c_q)·J_q·W_q is **Eq (19) term for term**; selection s ∝ w_q
with W_sel = (Σ_q w_q)/p̂(c_sel) is **Eq (22)** (the ideal m=c form); the tail
is c_sel·W_sel and L̂_bin = c_near + tail. The selection identity

    E_sel[ tail | samples ] = Σ_q m_q J_q W_q c_q          (p̂ cancels exactly)

holds (it is the workhorse of Prop V, [lemma-3_2.md](lemma-3_2.md) §5).

**[CORRECTION] two fixes to the naive correspondence:**
1. **D(T_q) is only the bijection domain.** Per Def 4.2, D(T_q) = the subset of
   Ω_q on which reconnection is a well-defined bijection (operationally:
   hasY_q). The support test p̂(Y_q)>0 is a *separate* condition (Eq 13/§4.3):
   w_q>0 iff X_q∈D(T_q) **and** p̂(Y_q)>0. An earlier draft folded p̂>0 into
   D(T_q); that conflates the shift domain with the target support.
2. **p̂(T_q X_q) is realized as p̂(c_q) only off the penumbra set.** The stored
   radiance c_q equals the target-domain radiance at Y_q exactly because
   reconnection re-anchors to the same emitter point (Lemma 3.2.I,
   [integrand-mismatch.md](integrand-mismatch.md)) — exact off
   P', not everywhere.

## 2. Which GRIS conditions hold, which fail

### HOLD

- **Eq (15) first inclusion, supp Y ⊂ supp p̂.** The λ-floor makes p̂ ≥ λ > 0 on
  all of Ω_b; w_q>0 requires p̂(c_q)>0; so no sample is ever produced outside
  supp p̂ = Ω_b.
- **Def 5.1 / Eq (27) reasonable distribution (bounded UCW).** p̂ ≥ λ > 0 gives
  W_sel ≤ (Σw)/λ < ∞, hence f·W ≤ C_f a.s., securing the Theorem 1
  finite-variance bound (Eq 29–30) *independently of every bias question*.
  **Two load-bearing caveats:**
  1. **Bounded radiance.** It is the same C_f that, downstream, converts an
     O(ε_n) *measure* bound (on the coverage sliver A and penumbra set P') into
     an O(ε_n) *bias* bound. For a near-point / unbounded emitter concentrated
     in an O(ε_n) strip the conversion fails and an O(1) over-count is
     constructible (see [lemma-3_2.md](lemma-3_2.md) Prop V, crux). Under the
     theory's own Def 5.1 premise it holds. This is why S2 (a near-point
     source) is the stress test.
  2. **[CORRECTION — found in review, missed by the audits] the a.s. constant
     compounds across the layered chain.** C_λ := (L_max+λ)/λ is only the
     *per-level* factor. Eq (27) demands an almost-sure bound, and stored
     radiances compound: c_child = c_near + c_sel·W_sel, where W_sel = Σw/p̂(c_sel)
     can reach ~C_λ·max_q W_q^{res} when a dark sample (p̂=λ) is selected
     against a bright proposal. Unrolling the layered collapse gives an a.s.
     bound on stored c at level n of order L_max·C_λ^{N−n} — **finite** (so
     Theorem 1 applies with SOME C_f) but **depth-exponential**, i.e.
     practically vacuous at depth. The meaningful controls are (i) the *mean*:
     E[c·W] stays ≤ the bin-mean ≤ L_max by the layered unbiasedness (each
     level's collapse is an expectation-preserving resolve), and (ii) temporal
     M-averaging keeping realized W near 1. The dark-selected-against-bright
     spike (probability ∝ w_dark/Σw, so rare but heavy-tailed) is the likely
     theory-side account of the observed spatially-correlated W-noise (lab
     log E4). The chain-variance recursion replacing the a.s. bound is given
     in [variance.md](variance.md) §3 (Theorem V).
- **Eq (19) weight form and Eq (22) UCW form:** hold term for term.
- **The technique/domain correspondence** (parents = techniques, parent bins =
  Ω_i, reconnection T_{q→p} with Jacobian r_q/r_p): legitimate under Def 4.2.

### FAIL

- **Eq (15) coverage (second inclusion, supp p̂ ⊂ supp Y): FAILS. This is
  Prop C.** The merge consults only the four reprojected parent bins
  bp_q = binOf(n+1, dir(q→z)). Bin parallax (dir(p→y) ≠ dir(q→y) at finite
  distance) leaves a sliver A ⊂ Ω_b whose reconnection vertices back-project
  into an *unconsulted* parent bin; for ω∈A no technique realizes Y, so
  supp Y = Ω_b∖A ⊊ Ω_b = supp p̂. See §3 for the bias.
- **Eq (17)/(20) partition of unity: FAILS as a GRIS certification.**
  **[CORRECTION over an earlier draft]**, which claimed the
  β-renormalization *is* Eq 17/20's "restrict to realizing techniques". It is
  not. Arithmetically Σ_{q∈S} β_q/β_S = 1, but S = {q : valid_q} is the *shadow-
  ray validation-survivor* set (visibility-determined, y-independent), whereas
  GRIS's realizing set is {q : y ∈ T_q(supp X_q)} (support-determined, per-y).
  **Occlusion ≠ non-realization.** Two concrete defects:
  - Eq (20)'s side condition "m_i(y) > 0 whenever c_i(y) ≠ 0" is *violated*: a
    parent that geometrically realizes y (shift valid, p̂>0, c_q≠0) but whose
    shadow ray p→y is blocked gets m_q=0. The p→y visibility lives in f
    (Eq 13), **not** in c_q and **not** in p̂ = lum(c)+λ — so c_q(y) ≠ 0 while
    m_q = 0.
  - A constant m_q cannot be a *per-y* partition where coverage is non-uniform,
    putting "Eq 17/20 holds" in direct tension with "Eq 15 coverage fails".
  Correct reading: β-renormalize-over-survivors is an **approximate, stochastic
  re-evaluation of the p→y visibility factor of f(Y)** — part of the structural
  gap (§4), to be *bounded* (Prop V), not *certified* by Eq 17/20. It coincides
  with the legal Eq 20 restriction only in the degenerate ρ=0 / uniform-
  coverage case.
- **Estimand identity f(Y) = stored c_s: FAILS off the emitter-anchored
  regime.** The tail carries the *parent's* visibility V(q,y) via c_sel, so the
  merge evaluates f_parent(Y), not f_p(Y) = V(p,y)·radiance. They differ exactly
  on P' (|P'| ≤ C₁(K+K_t)ε_n) and where a p→y blocker was invisible to q. This
  is the shadow leak — the structural gap (§4).

## 3. Coverage bias = Eq (15) violation (proof)

Eq (15): supp Y = supp p̂ ∩ ∪_q T_q(supp X_q). Since
∪_q T_{q→p}(consulted bin bp_q) omits the parallax sliver A,

    supp Y = Ω_b ∖ A ⊊ Ω_b = supp p̂.

GRIS (Eq 16) then makes the estimator unbiased for ∫_{supp Y} f, hence **biased
for the target ∫_{supp p̂} f by exactly**

    bias = ∫_{supp Y} f − ∫_{supp p̂} f = − ∫_A L_{≥n+1}(p,ω) dω,   A = supp p̂ ∖ supp Y.

The sign is **always negative** (missing support only removes energy);
|A|/|Ω_b| = O(ε_n), ε_n ≤ ε₀·2⁻ⁿ. Only the order and sign are pinned; the
parallax-to-bin-width constant is not derived. Consistent with the measured
lit-region deficit −2.2% at ρ=1 (lab log E2/E5, S1) as an order-of-magnitude
check (**not** a
derived value, and **not** on the same axis as the shadow-leak sequence — see
Prop V).

## 4. The single structural gap, and the GRIS authors' own account of it

**The gap:** the merge never re-evaluates the shifted integrand f(Y) under
probe p's *own* visibility; it deposits the stored parent radiance c_sel as-is
(value-passing / layered RIS). GRIS Eq (16) demands f(Y)·W_Y at the shifted
sample; ours substitutes c_sel·W_sel. By Lemma 3.2.I, reconnection re-anchors to
the same emitter point, so c_sel = f(Y) *exactly* off the penumbra set P'; the gap
is nonzero only on P' and where a p→y blocker was invisible to q — the shadow
leak. The ρ-Bernoulli shadow ray over [t_{n+1}, |y−p|) is an approximate,
stochastic version of the missing V(p,y) re-evaluation, which is why ρ→1 nearly
closes the leak.

**This is not our analogy — it is GRIS's own Appendix B ("On Visibility").**
GRIS observes that ReSTIR DI uses a target p̂_i *without* the visibility term;
that "neglecting visibility … creates paths with positive p̂_i that never get
sampled as X_i due to occlusion. This implies supp p̂_i ⊄ supp X_i, making X_i
non-canonical. Without extra guarantees, Y … no longer covers supp p̂_i,
breaking the supp p̂ ⊂ supp Y_M assumption … and preventing convergence." Our
stored target p̂ = lum(c)+λ is exactly such an *unoccluded target p̂⁻ⱽ* (c is
the far emitter radiance; p's visibility to y is not baked in). GRIS notes the
leftover is "often imperceptible bias" and gives **no bound**.

**Our contribution is the bound GRIS's Appendix B invites:** in the cascade the
penumbra condition pins this visibility-coverage bias to O(ε_n) = O(ε₀2⁻ⁿ) per
level, summable to < 3ε₀ over the whole cascade independent of depth.

**Canonical samples (GRIS §5.5, Thm A.4).** GRIS Thm A.4 needs |R| ≥ 1 canonical
samples (an importance sampler that directly targets p̂ with identity shift, so
supp p̂ ⊂ supp X). In our merge the child's fresh jittered near-interval
candidate (c_near) is canonical for the *near* interval [t_n, t_{n+1}). But the
*tail* sub-problem — the [t_{n+1},∞) merge over four parents — has **|R| = 0
canonical samples**: all four parents are reused (non-canonical). So Thm A.4's
|R| ≥ 1 is violated for the tail; there is no built-in weight bound and no
automatic coverage. This is precisely why (a) coverage can fail (Prop C) and
(b) the λ-defensive term is needed as the surrogate weight bound (W ≤ Σw/λ). In
one line: **the cascade tail merge trades the canonical sample for the penumbra
bound.**

## 5. Summary — three separable deficiencies

A fully-unbiased GRIS instance of the merge would need *both* of the
following fixed — they are distinct (an earlier draft conflated them):

1. **Coverage (Eq 15, Prop C)** — a *geometrically fixable* O(ε_n) deficit,
   negative sign. Remedy: widen the consulted-bin neighbourhood (windowed
   lookup with per-bin MIS) so ∪_q T_q(supp X_q) covers Ω_b.
2. **Partition-of-unity mislabel** — β-renormalization is not an Eq 17/20 MIS
   partition but an approximate visibility re-evaluation; its bias is bounded in
   Prop V, not certified here.
3. **Value-passing structural gap (§4)** — the merge never re-evaluates f(Y)
   under p's visibility. This is *the* structural gap: it persists even under
   perfect coverage and exact MIS, and is the one the ρ-validation attacks.

Items 1 and 3 are distinct: coverage is about *which directions get any
proposal*; the value-passing gap is about *whether the proposed value carries
p's visibility*. Both are O(ε_n) under bounded radiance and summable across the
cascade.

## 6. Open (ranked by payoff)

(The chain-variance recursion and the windowed-lookup condition, formerly
listed here, are resolved in [variance.md](variance.md) and
[lemma-3_2.md](lemma-3_2.md) Prop W respectively.)

1. The geometric constant of |A|/|Ω_b| for the axis-aligned grid (a
   computable integral over content depth — would upgrade one order-only
   claim to an exact constant); the leak's ~27–35%≈ε₁ calibration (E5 /
   E12 configs) stays calibrated.
2. Unanimity under inter-parent correlation: the union bound over
   per-parent sliver events is correlation-robust (shared grandparents only
   *help* unanimity); state this explicitly in Prop V.
3. Two-component temporal model (switch + AR(1)); inter-level ESS to locate
   a* in Lemma T's bracket (variance.md §5).
