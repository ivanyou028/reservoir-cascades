# Lemma 3.2.I — Integrand mismatch under reconnection: full proof

Status: v0.1, 2026-07-23. Discharges TODO item 1 of [lemma-3_2.md](lemma-3_2.md).
Proofs are complete for polygonal scenes; the smooth-obstacle case carries a
√ε term from grazing corridors (Lemma G) that may be improvable.

## 1. Definitions

Fix level n; write t := t_{n+1}, d := |p−q| ≤ √2·s_{n+1}, so d/t ≤ ε_n.

**Far field.** For observation point x and direction ω, let h(x,ω) be the
first scene intersection of the ray x+τω with τ ≥ t (undefined on escape),
and F_x(ω) := E(h(x,ω)), the emission at that point (emitters
view-independent; escape ⇒ F = 0, sky constant handled trivially).

**Shift.** T = T_{q→p} maps ω to dir(p → h(q,ω)) where h exists; identity
on escape directions. J(ω) is the reconnection Jacobian (Lemma J).

**Quantity to bound.** The GRIS-facing mismatch on q's bin domain Ω:

    D := ∫_Ω | F_p(T(ω))·J(ω) − F_q(ω) | dω.

**Sliver.** For a hit direction ω with y = h(q,ω), r = |y−q|, define
Σ(ω) := conv hull region bounded by the segments R_q = [q+tω, y],
R_p = [p + t·T(ω), y], and the endpoints arc. Σ has apex y, length ≤ r, and
width ≤ 2d everywhere (endpoint separation ≤ d + t·|T(ω)−ω| ≤ 2d by Lemma D).

**Penumbra set.** P := { ω ∈ Ω hit : h(p, T(ω)) ≠ h(q, ω) } — directions
where p and q disagree on the far hit (includes "y inside p's near disc").

## 2. Lemma A (disagreement ⇒ sliver or annulus event)

If ω ∈ P, at least one holds:
(a) some obstacle boundary intersects Σ(ω) \ {y};
(b) some obstacle intersects the annulus A_t := { z : t−2d ≤ dist to the
    segment pq ≤ t+2d } along the ray family (interval-start ordering flip).

*Proof.* Suppose neither. R_q is hit-free on (t, r) by definition of h.
If h(p,T(ω)) ≠ y, the segment R_p meets an obstacle at some z ≠ y, or y
precedes t from p. The latter is case (b): |y−p| ≥ |y−q| − d = r − d and
r ≥ t, so |y−p| < t forces r ∈ [t, t+d), i.e. y ∈ A_t — an obstacle point in
the annulus, contradiction. For the former: z ∈ R_p ⊂ ∂Σ, z ≠ y. The
obstacle containing z is disjoint from R_q (hit-free) — but ¬(a) says its
boundary avoids Σ\{y}; a connected obstacle touching ∂Σ at z with boundary
avoiding Σ's interior and the opposite wall R_q must still cross ∂Σ near z
into Σ (obstacles have nonempty interior on the z side), contradiction. ∎
(TODO: the last step deserves a pedantic epsilon-neighborhood argument;
content is elementary planar topology.)

## 3. Lemma B (corridor measure)

Let O be a convex obstacle (or convex boundary arc) entirely at distance
≥ t/2 from q. Then

    |{ ω : O ∩ Σ(ω) ≠ ∅  and  R_q(ω) misses O }| ≤ 2 · (2d) / (t/2) = 8d/t.

*Proof.* Parameterize directions from q by impact parameter b(ω) w.r.t. O
(signed distance from the ray to the nearest silhouette tangent line of O).
Rays missing O but with Σ(ω) ∩ O ≠ ∅ satisfy 0 < b(ω) ≤ width(Σ) ≤ 2d
(Σ lies within 2d of R_q). For a convex O seen from q, b is monotone in ω
near each of its two silhouettes with |db/dω| ≥ dist(q,O) ≥ t/2. Hence each
silhouette contributes an ω-band of measure ≤ 2d/(t/2); two silhouettes. ∎

Summing over the K silhouette arcs visible beyond t:
|{ω : case (a)}| ≤ 8K·d/t ≤ 8K·ε_n. (Obstacles nearer than t/2 to q cannot
generate case (a) beyond t; their silhouettes are counted at the level that
owns them — this is where the cascade decomposition enters the proof.)

## 4. Lemma C (annulus band)

Obstacle boundary arcs crossing the circle of radius t around q contribute
ordering-flip directions of measure ≤ 2·(2d)/t per crossing; with K_t
crossings, |{ω : case (b)}| ≤ 4K_t·ε_n.

*Proof sketch.* A flip requires an obstacle point within the annulus of
radial thickness 4d at radius ≥ t−2d; each transversal boundary crossing of
the t-circle occupies an ω-band of width ≤ (thickness)/(radius)·csc(angle);
transversality (assumption A5 below) bounds csc by a constant. ∎

Remark. This is precisely the band that the interval-boundary jitter (E9)
randomizes: under block jitter the deterministic band becomes a
zero-mean perturbation across blocks. Formalizing that statement is TODO
item 4 (piecewise-stationary MIS).

## 5. Lemma G (grazing control; polygonal vs smooth dichotomy)

The cos-ratio factor of J requires cos θ_q bounded away from 0.

(i) **Polygonal obstacles.** The set of hit directions with cos θ_q < c is
contained in bands around the ≤K vertex/edge-tangent directions of total
measure → 0 as c → 0; taking θ_max fixed adds measure 0 in the limit and
J = 1 + O(ε_n) holds off P. (Edges are flat: a ray hitting an edge interior
has cos θ bounded below by the edge's orientation gap to the ray, and the
directions hitting a given edge at angle < θ_c form a band of measure
≤ (edge length/t)·sin θ_c — absorbable.)

(ii) **Smooth strictly-convex obstacles** (curvature ∈ [κ₋, κ₊], e.g.
circles of radius R = 1/κ). A hit with cos θ_q ≤ c has impact parameter
within R·c²/2 of the silhouette, an ω-band of measure ≤ R c²/(2t) per
silhouette. On the complement, |J−1| ≤ ε_n·(1 + tan θ_max) ≤ ε_n·(2/c).
Balancing c := ε_n^{1/2}:

    grazing-band measure ≤ K·R·ε_n/(2t),   |J−1| ≤ 2·ε_n^{1/2} off it.

So smooth obstacles cost a **√ε_n Jacobian bound** (or an ε_n-measure
exclusion — either way the aggregate mismatch picks up O(K·√ε_n)).
Whether √ε is tight for smooth scenes is open; we conjecture it is, by the
standard soft-shadow-boundary scaling. Our test scenes: boxes → case (i),
the S2/S3 circles → case (ii).

## 6. Theorem I (integrand mismatch)

Assumptions: (A1) finite scene, ≤K silhouette arcs, ≤K_t transversal
t-circle crossings, emission ≤ L_max; (A5) no boundary arc tangent to the
t-circle (transversality; violated on a measure-zero set of t, and the
boundary jitter makes t random anyway).

Then with P' := P ∪ (grazing bands),

    |P'| ≤ C₁·(K + K_t)·ε_n            (polygonal; +K·R·ε_n/t smooth), and
    for ω ∉ P':  F_p(T(ω))·J(ω) = F_q(ω)·(1 + η(ω)),
                 |η| ≤ C₂·ε_n          (polygonal)   or  C₂·√ε_n (smooth);
    D ≤ L_max·( C₂·ε_n^{γ}·|Ω| + 3·|P'| ),   γ = 1 or ½.

*Proof.* Off P': same hit point ⇒ F_p(T(ω)) = E(y) = F_q(ω) exactly — the
entire off-penumbra error is the Jacobian factor (note: NO Lipschitz term;
reconnection re-anchors to the identical emitter point, so emitter texture
never enters). Lemma J with Lemma G controls η. On P': both terms bounded
by L_max·max(J,1) ≤ 3L_max/... and Lemmas A–C bound the measure. ∎

## 7. GRIS-facing corollary and the role of λ

For the RIS merge, what must be bounded is the contribution weight
w = m·p̂_p(T x)·J·W. Theorem I gives p̂_p(Tx)·J ≤ (L_max + λ)(1 + C₂ε^γ)
everywhere, and off P' the ratio p̂_p(Tx)J/p̂_q(x) ∈ [1−Cε^γ, 1+Cε^γ].
On P' the ratio is arbitrary — but the defensive term λ > 0 in p̂ bounds
W ≤ (Σw)/λ, so P' contributes bounded-variance mass proportional to its
measure O(K·ε_n). This is the precise sense of the proposal's "the penumbra
region is backstopped by MIS": λ-defensiveness converts the unbounded penumbra ratio
into O(K·ε_n) extra variance. Per-level, summable: Σ_n ε_n < 2ε₀.

## 8. Consistency check against measurements

Defaults s₀=1, t₀=4, B₀=4 ⇒ ε₀ ≈ 0.53. S1's leak zone sits at chain levels
1–2 (ε ≈ 0.27–0.13); silhouettes: bar 4 + light 4 ⇒ K ≈ 8. Predicted
penumbra fraction C₁Kε/(2π) ~ 0.1–0.3 → the un-validated (ρ=0) leak should
be a noticeable-but-fractional share of vanilla's, and validation-corrected
runs should sit within a few percent of reference — consistent with
measured 27%-of-vanilla (ρ=0) and −2.2% (ρ=1, Prop V bias). Order-of-
magnitude only; the constants are loose by design.

## 9. Remaining gaps

- Lemma A's final topological step: write the ε-neighborhood argument.
- Lemma C constant: make the transversality quantitative (csc bound).
- Smooth-case √ε: prove tightness or improve.
- Aggregate-vs-per-bin: Theorem I is a full-circle statement; per-bin the
  worst bin can be entirely penumbral. The estimator-level consequence
  (bounded W via λ) is per-bin safe; state this explicitly in the paper.
