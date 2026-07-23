# Lemma 3.2.I — Integrand mismatch under reconnection: full proof

Status: v0.2, 2026-07-23. Proves the shifted-integrand mismatch bound
consumed by [lemma-3_2.md](lemma-3_2.md) §6, where it is cited as
**Lemma 3.2.I** (see that document's header note for project context,
"the proposal", E-numbers, and the scenes S1–S3).
v0.2 closes the §9 gaps of v0.1: Lemma A's topological step is now a full
proof (clopen argument); Lemma C is quantitative (explicit csc α₀ constant,
plus an *averaged* form under boundary jitter that needs no transversality
at all); the grazing analysis (Lemma G) now includes the measure bound from
scene primitives, and — new — a pointwise/aggregate split: the smooth
√ε_n is **pointwise tight** (Lemma G′ constructs the corridor) yet the
**aggregate** mismatch is O(ε_n) for smooth scenes and O(ε_n log(1/ε_n))
for polygonal ones (Theorem I-b) — the corridor is too thin to hurt the
integral. The v0.1 conjecture that √ε is aggregate-tight is thereby
refuted; tightness survives only pointwise.

## 1. Definitions

Fix level n; write t := t_{n+1}, d := |p−q| ≤ √2·s_{n+1}, so d/t ≤ ε_n.

**Far field.** For observation point x and direction ω, let h(x,ω) be the
first scene intersection of the ray x+τω with τ ≥ t (undefined on escape),
and F_x(ω) := E(h(x,ω)), the emission at that point (emitters
view-independent; escape ⇒ F = 0, sky constant handled trivially).

**Shift.** T = T_{q→p} maps ω to dir(p → h(q,ω)) where h exists; identity
on escape directions. J(ω) is the reconnection Jacobian (Lemma J,
[lemma-3_2.md](lemma-3_2.md) §1).

**Quantity to bound.** The GRIS-facing mismatch on q's bin domain Ω:

    D := ∫_Ω | F_p(T(ω))·J(ω) − F_q(ω) | dω.

**Sliver.** For a hit direction ω with y = h(q,ω), r = |y−q|, define
Σ(ω) := conv hull region bounded by the segments R_q = [q+tω, y],
R_p = [p + t·T(ω), y], and the endpoints arc. Σ has apex y, length ≤ r, and
width ≤ 2d everywhere (endpoint separation ≤ d + t·|T(ω)−ω| ≤ 2d by
Lemma D, [lemma-3_2.md](lemma-3_2.md) §2).

**Penumbra set.** P := { ω ∈ Ω hit : h(p, T(ω)) ≠ h(q, ω) } — directions
where p and q disagree on the far hit (includes "y inside p's near disc").

## 2. Lemma A (disagreement ⇒ sliver or shell event) — full proof

If ω ∈ P, at least one holds:
(a) some obstacle boundary meets Σ(ω) \ {y};
(b) the *shell event*: r = |y−q| ∈ [t, t+d) — the hit sits within d of the
    interval start, so the p/q interval-start ordering can flip.

*Proof.* Suppose ¬(a) and ¬(b). Obstacles are compact, connected, with
nonempty interior; ∂O denotes the boundary curve of obstacle O. By
definition of h(q,ω) = y, the open segment R_q° := (q+tω, y) contains no
scene point, and neither does the near endpoint q+tω (the ray is scene-free
on parameters [t, r), and t < r).

Step 1 (the p-ray reaches y). ¬(b) gives r ≥ t+d, so
|y−p| ≥ r − d ≥ t: the ray p + τ·T(ω), which passes through y at parameter
|y−p| by construction of T, has a first scene intersection z = h(p,T(ω))
with parameter in [t, |y−p|]. Disagreement (ω ∈ P) means z ≠ y, i.e.
z ∈ [p+tT(ω), y) =: R_p \ {y}.

Step 2 (the sliver is a Jordan region). Γ := R_q ∪ [q+tω, p+tT(ω)] ∪ R_p
is a closed curve: two segments sharing the single endpoint y plus the near
chord. Distinct segments sharing an endpoint meet only there unless
colinear; the chord meets each segment only at its shared endpoint likewise.
In the degenerate colinear-overlap cases, R_p ⊂ R_q ∪ chord, so
z ∈ R_p ∩ scene lies either on R_q (contradicting scene-freeness) or within
distance d of the interval-start circle (a shell event, excluded by ¬(b));
so assume Γ simple. Σ := Γ ∪ int Γ is then a closed Jordan region, and
Σ \ {y} is path-connected (a Jordan region minus one boundary point).

Step 3 (clopen argument). Let O be the obstacle with z ∈ O, and set
U := O ∩ (Σ \ {y}). U ∋ z, so U ≠ ∅. U is closed in Σ \ {y} since O is
closed. U is also open in Σ \ {y}: for u ∈ U, either u ∈ ∂O — impossible,
since ¬(a) says ∂O misses Σ \ {y} — or u ∈ int O, in which case a planar
neighborhood of u lies in O and its trace on Σ \ {y} lies in U. (The case
z ∈ int O occurs only when the segment start p+tT(ω) itself sits inside O;
a first hit strictly after a free start would cross ∂O first and land in the
previous case.) A nonempty clopen subset of the connected set Σ \ {y} is
the whole set: U = Σ \ {y} ⊇ R_q \ {y} — contradicting the scene-freeness
of R_q° ∪ {q+tω}. ∎

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

## 4. Lemma C (shell band — quantitative, two forms)

The shell event of Lemma A(b) is {ω : r(ω) ∈ [t, t+d)} — first hits in the
radial shell of thickness d at radius t around q.

**C-fixed (deterministic t, transversality).** Assume (A5) every
obstacle-boundary arc crosses the circle of radius t about q at angle
≥ α₀ from the circle's tangent, with curvature ≤ κ₊ and
d ≤ sin²α₀/(4κ₊). Then, with K_t crossings,

    |{ω : shell event}| ≤ 2·csc α₀ · K_t · ε_n.

*Proof.* First hits are boundary points, and the ω-measure of directions
whose first hit traverses a boundary sub-arc of length dℓ at radius
ρ ≥ t is dω = cos θ_inc·dℓ/ρ ≤ dℓ/t. Near an α₀-transversal crossing the
radial coordinate moves along the arc at speed |dρ/ds| = sin(angle to
tangent) ≥ sin α₀ − κ₊·s; the curvature premise keeps this ≥ ½ sin α₀
while the arc stays inside the shell, so the arc-length inside is
≤ 2d/sin α₀ per crossing. Multiply and sum over K_t crossings. ∎

**C-avg (jittered t — no transversality needed).** Under the boundary
jitter (E9), t is drawn with density ρ(t′) ≤ ρ_max on its sweep range
(log-uniform over [t/2, 2t]: ρ_max = 2/(t ln 4)). Then, unconditionally,

    E_t′ |{ω : shell event at t′}| = ∫ dω · Pr{ t′ ≤ r(ω) < t′+d }
                                   ≤ ρ_max · d · |Ω|  =  (2|Ω|/ln 4) · ε_n.

*Proof.* Fubini in (ω, t′); for each hit direction the shell condition
confines t′ to an interval of length d. ∎

C-avg is the formal content of "boundary jitter converts the interval-
boundary band into frame-averaged variance": the deterministic band —
whose constant blows up as csc α₀ for unluckily-tangent geometry — becomes
an *averaged* O(ε_n) with a universal constant, at the price of the band
reappearing per-frame as variance. The estimator-level statement is
Prop J in [lemma-3_2.md](lemma-3_2.md) §5b.

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
silhouette. On the complement, |J−1| ≤ ε_n·(2 + tan θ_max) (Lemma J's
constant) ≤ 3·ε_n/c. Balancing c := ε_n^{1/2}:

    grazing-band measure ≤ K·R·ε_n/(2t),   |J−1| ≤ 3·ε_n^{1/2} off it.

So smooth obstacles cost a **√ε_n pointwise Jacobian bound**. Our test
scenes: boxes → case (i), the S2/S3 circles → case (ii).

(iii) **Grazing profile from scene primitives.** Define
M_q(ψ_c) := |{ω : first hit beyond t has incidence angle ≤ ψ_c from the
surface tangent}|. For a flat edge e, the incidence angle is the
*direction-space offset* ψ(ω) = |ω − dir_e| (mod π), so grazing directions
form fixed bands around ±dir_e intersected with e's subtended arc:

    M(ψ_c) ≤ Σ_e min( 4ψ_c , (ℓ_e/t)·sin ψ_c )  ≤  4K·ψ_c      (flat),
    M(ψ_c) ≤ K · ψ_c² / (2κ₋ t)                                 (smooth),

the smooth case via impact-parameter depth b ≤ ψ_c²/(2κ₋) per silhouette
and dω = db/dist ≤ db/t. This makes the non-grazing assumption *derived*,
with an explicit measure at every threshold: the exclusion is
ψ_c-small × scene-subtended for flat edges, but ψ_c²-small for smooth
arcs — curvature *spreads* grazing directions. That reversal is why the
pointwise and aggregate dichotomies below point in opposite directions.

### Lemma G′ (the smooth √ε_n is pointwise TIGHT — construction)

Scene: one emissive circle of radius R centered at distance D = 2t from q;
p = q + d·u with u ⊥ (center − q), d = ε·t. For hit directions with
cos θ_q ∈ [c, 2c] (impact depth b ∈ [Rc²/2, 2Rc²], a direction band of
measure 3Rc²/(2D) > 0), the reconnection at the shared hit point y
subtends Δθ = ∠(y→p, y→q) ≥ d/(2D) = ε/4, while tan θ_q ≥ 1/(2c). Hence

    |η| = |cos θ_p/cos θ_q − 1| ≥ Δθ·tan θ_q·(1 − o(1)) ≥ ε/(16c).

At c = √ε: |η| ≥ √ε/16 on a set of measure ≈ (3R/2D)·ε > 0. The balanced
√ε_n of Lemma G(ii) is therefore achieved, not pessimistic: **pointwise,
γ = ½ is tight for smooth scenes.** (v0.1 conjectured this; the corridor
construction settles it.)

## 6. Theorem I (integrand mismatch — pointwise and aggregate)

(This is the result cited as **Lemma 3.2.I** from the other notes.)

Assumptions: (A1) finite scene, ≤K silhouette arcs, ≤K_t shell crossings,
emission ≤ L_max; (A2)–(A4) as in [lemma-3_2.md](lemma-3_2.md) §6;
(A5) α₀-transversality for the fixed-t shell bound (Lemma C-fixed), or
boundary jitter for the averaged form (C-avg — no transversality). Write
P′ := P ∪ shell (Lemmas A–C: |P′| ≤ C₁(K+K_t)ε_n with C₁ = C₁(α₀), or in
t-expectation with a universal constant), and G(ψ₀) for the grazing
exclusion of Lemma G(iii).

**I-a (pointwise).** Off P′ ∪ G(ψ₀), reconnection re-anchors to the
identical emitter point, so

    F_p(T(ω))·J(ω) = F_q(ω)·(1 + η(ω)),   |η| ≤ ε_n·(2 + cot ψ₀)

— no Lipschitz term; emitter texture never enters. Balanced instances:

  - smooth (curvature ≥ κ₋): ψ₀ = √ε_n gives |G| ≤ K·ε_n/(2κ₋t) = O(ε_n)
    and |η| ≤ 3√ε_n — and no better exponent is possible (Lemma G′):
    **γ = ½, tight, with an O(ε_n)-measure exclusion.**
  - flat: any fixed ψ₀ gives |η| ≤ C(ψ₀)·ε_n — **γ = 1** — but the
    exclusion G(ψ₀) is a fixed sin ψ₀-fraction of each edge's subtended
    arc (≤ 4Kψ₀), *not* O(ε_n); shrinking it to O(ε_n^{1−a}) costs
    |η| = O(ε_n^{a}). Pointwise, flat scenes trade exclusion measure
    against exponent along this family.

**I-b (aggregate — the √ε disappears).** With no exclusion at all,
saturating |η|·F ≤ 3L_max on P′ ∪ {grazing} and integrating the pointwise
bound against the grazing profile dM(ψ) of Lemma G(iii):

    D ≤ L_max·[ 3|P′| + 2ε_n|Ω| + ∫₀^{π/2} min(3, ε_n cot ψ) dM(ψ) ]

    ∫ dM-part ≤ 4K·ε_n·(1 + log(3/ε_n))           (flat: dM ≤ 4K dψ)
    ∫ dM-part ≤ (πK/2κ₋t)·ε_n + O(ε_n²)           (smooth: dM ≤ Kψ/(κ₋t)dψ)

so D = O(ε_n log(1/ε_n)) for polygonal scenes and **O(ε_n) — no √ε, no
log — for smooth scenes**. The grazing corridor that forces γ = ½
pointwise is too thin (measure ∝ ψ²) to affect the integral: curvature
spreads grazing directions exactly fast enough to cancel the cot ψ
blow-up. The two dichotomies point in opposite directions — smooth is
*worse pointwise, better in aggregate* — and the v0.1 conjecture that √ε
is aggregate-tight is refuted.

*Proof of both.* Off P′, F_p(T(ω)) = E(y) = F_q(ω) exactly (same hit
point, Lemma A); the entire off-penumbra error is the Jacobian factor,
|J−1| ≤ ε_n(2 + cot ψ(ω)) by Lemma J with tan θ = cot ψ. On P′:
|F_p(Tω)J| + |F_q| ≤ L_max(J+1) ≤ 3L_max off the ψ ≤ ε_n/3 saturation
core, which is absorbed at rate 3·M(ε_n/3) ≤ min(4Kε_n, Kε_n²/(18κ₋t)).
For I-b split the integral at the saturation threshold ψ* = ε_n/3 and
integrate dM by parts on each side; the stated bounds follow from the two
profiles of Lemma G(iii). ∎

*Where each form is used:* the **bias ledger** (the paper's mismatch lemma
→ soundness theorem) consumes I-b — per-level aggregate bias
O(ε_n log(1/ε_n)) worst case, O(ε_n) for smooth scenes, summable over
levels either way. The **weight-ratio/variance analysis** (§7,
[variance.md](variance.md)) consumes I-a plus the λ-backstop on the
excluded set. The paper's γ-dichotomy statement is the pointwise I-a; the
aggregate statement is I-b.

## 7. GRIS-facing corollary and the role of λ

For the RIS merge, what must be bounded is the contribution weight
w = m·p̂_p(T x)·J·W. Theorem I-a gives, off P′ ∪ G(ψ₀), the ratio
p̂_p(Tx)J/p̂_q(x) ∈ [1−Cε^γ, 1+Cε^γ] with the (ψ₀, γ) tradeoff of I-a.
On the excluded set the ratio is arbitrary — but the defensive term λ > 0
in p̂ bounds W ≤ (Σw)/λ, so the exclusion contributes bounded-variance
mass proportional to its measure. This is the precise sense in which the
design-stage claim "the penumbra region is backstopped by MIS" (internal
proposal — see the [lemma-3_2.md](lemma-3_2.md) header note) holds:
λ-defensiveness converts the unbounded penumbra ratio into extra variance
of order the excluded measure. The *bias* side needs no exclusion at all:
I-b bounds the aggregate mismatch directly. Per-level, summable:
Σ_n ε_n < 3ε₀.

## 8. Consistency check against measurements

(Scene shorthand: S1 = thin-bar occluder + box light, polygonal; S2/S3 use
circular emitters — see the lab log. "Vanilla" = deterministic RC
baseline; measurements from E5.) Defaults s₀=1, t₀=4, B₀=4 ⇒ ε₀ ≈ 0.53. S1's leak zone sits at chain levels
1–2 (ε ≈ 0.27–0.13); silhouettes: bar 4 + light 4 ⇒ K ≈ 8. Predicted
penumbra fraction C₁Kε/(2π) ~ 0.1–0.3 → the un-validated (ρ=0) leak should
be a noticeable-but-fractional share of vanilla's, and validation-corrected
runs should sit within a few percent of reference — consistent with
measured 27%-of-vanilla leak (ρ=0; E5) and −2.2% lit deficit (ρ=1; E2/E5 —
the Prop V bias, [lemma-3_2.md](lemma-3_2.md) §5). Order-of-
magnitude only; the constants are loose by design.

## 9. Remaining gaps (post-v0.2)

- ~~Lemma A topological step~~ → done (clopen argument, §2).
- ~~Lemma C constant~~ → done (C-fixed: explicit csc α₀; C-avg: universal
  constant under jitter, no transversality).
- ~~Smooth-case √ε tightness~~ → settled both ways: pointwise tight
  (Lemma G′ construction), aggregate refuted (Theorem I-b: smooth
  aggregate is O(ε_n)).
- Aggregate-vs-per-bin: Theorem I-b is a full-circle statement; per-bin
  the worst bin can be entirely penumbral, so per-bin claims must go
  through (i) the λ-bounded weights (bin-local, unconditional) for
  variance and (ii) the full-circle I-b for summed bias. Stated in §6
  ("where each form is used").
- Whether the flat-case aggregate log is removable (edge-orientation
  averaging over the K edges should kill it generically) — open, harmless:
  the log enters no headline claim.
- Lemma B for non-convex C¹ boundaries: reduce to ≤K convex silhouette
  arcs (A1 grants the decomposition); constants absorb.
