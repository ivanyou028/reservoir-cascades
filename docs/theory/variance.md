# Variance analysis — the missing half of bias→variance

Status: v0.1, 2026-07-23. Companion to [lemma-3_2.md](lemma-3_2.md) (see
its header note for project context, "the proposal", and E-numbers).
Supplies the chain-variance recursion flagged in
[gris-anchoring.md](gris-anchoring.md) §2 (caveat 2) — replacing the
depth-exponential a.s. C_f bound — plus the Markov analysis of the temporal
reservoir. Notation as in [lemma-3_2.md](lemma-3_2.md); scalar
analysis on luminance (RGB picks up a chroma factor κ = maxchannel/lum ≤ 3,
entering squared; noted where it matters).

## 0. Setup

Per (probe p, bin b, level n), single-frame no-temporal first. Escaped-case
tail: X_n = c_s·W_sel, W_sel = (Σ_q w_q)/p̂(c_s), w_q = m_q p̂(c_q) J_q W_q,
p̂(c) = c + λ (luminance), Σm_q = 1, J_q ≤ 1+Cε_n (Lemma J), W_q = 1 fresh.
**Scope [made explicit 2026-07-23b, after external review]:** W_q ≡ 1 is
*structural* for the single-frame estimator, not an assumption — the
per-level collapse stores every fresh reservoir with W = 1 (restir.cpp
phase 2), and the collapse's own selection randomness lives inside the
stored value c_q, which is exactly what Lemmas S/T track. With temporal
reuse ON, stored reservoirs carry W = Σw/p̂ ≠ 1 and the analysis below
does NOT cover the combined temporal×hierarchy chain — see §5.
μ := E_sel[X_n | {c_q}] = Σ_q m_q J_q c_q (identity (†), Prop V).

## 1. Lemma S (selection second moment — λ is the entire selection-noise dial)

    E_sel[ X_n² | {c_q} ] ≤ μ·(μ + λ·J̄),   J̄ := Σ_q m_q J_q ≤ 1+Cε_n,

hence Var_sel[X_n | {c_q}] ≤ λ·(1+Cε_n)·μ.

*Proof.* E[X²|·] = Σ_q (w_q/Σw)·c_q²(Σw/p̂(c_q))² = Σ_q w_q (c_q²/p̂(c_q)²)·Σw.
Substituting w_q = m_q p̂(c_q) J_q: the q-th term is m_q J_q·(c_q²/p̂(c_q)).
Since λ>0 gives c/p̂(c) = c/(c+λ) ≤ 1, c²/p̂ ≤ c, so the first factor sums to
≤ Σ m_q J_q c_q = μ. The second factor Σw = Σ_r m_r J_r (c_r+λ) = μ + λJ̄. ∎
(RGB: multiply by κ² ≤ 9; in practice κ≈1 for near-neutral emitters.)

Interpretation: **with λ=0 the RIS selection is zero-variance given the
parents** — target ∝ value makes c_s·W_sel = Σ m J c deterministically (this
is the proposal §3.5 "chromaticity stabilizer" — the design choice of a
target proportional to the stored value — doing quantitative work). λ injects
selection variance linearly: it is a *dial*, trading dark-region coverage
(Prop V's defensive floor) against λμ per-level noise. This also resolves the
apparent conflict with the a.s. bound C_λ^depth of gris-anchoring §2: the
dark-selected-against-bright spike has magnitude ~(μ/λ)·c but probability
~λ/(μ+λ), so its second-moment contribution stays ≤ μ(μ+λJ̄) — the heavy tail
is integrable, only the sup-bound is depth-exponential.

## 2. Lemma T (level transfer under arbitrary parent correlation)

    Var[ Σ_q m_q J_q c_q ] ≤ (1+Cε_n)² · a* · V_{n+1},
    a* ∈ [Σ_q m_q², 1],   V_{n+1} := max_q Var[c_q].

*Proof.* Cauchy–Schwarz: Var[Σ a_q Z_q] ≤ (Σ a_q σ_q)² ≤ (Σa_q)²·max σ²
with Σ_q m_q J_q ≤ (1+Cε_n) — the a*=1 endpoint (perfectly correlated
parents). Independent parents give a* = Σm_q² ∈ [¼, 1] (≈ 0.25–0.36 for
bilinear β). ∎

**[Scope note 2026-07-23c, second external review]** The Cauchy–Schwarz
step above treats the coefficients m_qJ_q as fixed. In the estimator they
are random (m_q via validation survivors, J_q via the stored hit point)
and correlated with c_q. For ρ=0 the fluctuation is deterministic-bounded
(|J−1| ≤ Cε_n, m_q = β_q fixed) and the repair is a centering computation
adding an O(ε_n²)-factor cross term; for ρ>0 the coefficient variance
couples to survivor non-unanimity and adds an O(ε_n)·μ̄² injection term
(aggregate reading), contingent on the Prop-V sample-distribution bound.
Status update 2026-07-23f: the ρ=0 single-bin case is now CLOSED by
Lemma T′ (§2b below); ρ>0 and the windowed merge's slot-balance
coefficients remain open.

**Inter-level correlation (the proposal's "risk #1"; cf. E8's red-line
test), quantified:** it moves a* within [Σm², 1] —
from 4× variance reduction per merge toward none — but *never past 1*:
correlation slows convergence, it cannot cause divergence. This is the
rigorous form of the proposal's "downgraded from variance explosion
to slower convergence". The empirical
position of a* is measurable via inter-level ESS (future work; the E8
autocorrelation is the temporal analogue).


## 2b. Lemma T′ (random-coefficient repair, single-bin merge, ρ=0)

**[2026-07-23f — closes review-round-2 gap 1 for the ρ=0 single-frame
single-bin case. No independence or conditioning between (J_q, c_q) is
used anywhere; only a deterministic Jacobian bound and the L² triangle
inequality.]**

Setting: single frame, no temporal, ρ = 0, single-bin merge. Then
m_q = β_q (deterministic, Σβ = 1), W_q ≡ 1 (structural), and the
IMPLEMENTED Jacobian J_q = r_q/r_p satisfies the deterministic bound

    J_q ∈ [1/(1+ε_n), 1/(1−ε_n)],  hence  |J_q − 1| ≤ κ_n := ε_n/(1−ε_n)

(r_q ≥ t_{n+1} for every stored parent sample, d ≤ ε_n·t_{n+1}; the
distance-ratio J has NO grazing exception — that caveat belongs to the
cos-ratio variant the implementation drops).
**HYPOTHESIS: ε_n < 1 at every level covered by the claim.** This is a
real exclusion, not a technicality: at ε_n ≥ 1 (attainable at level 0
under extreme boundary jitter) r_p has no positive lower bound and
J = r_q/r_p is unbounded — the full-ring escalation repairs COVERAGE
only and does nothing for this variance bound. Jitter splits with
ε₀ ≥ 1 fall outside Lemma T′ and outside Theorem V as restated below.

**Claim.** With Z_q := c_q, V := max_q Var[Z_q], L̄ := max_q |E Z_q|
(≤ L_max by layered unbiasedness), and arbitrary joint dependence between
the (J_q, Z_q):

    Var[ Σ_q β_q J_q Z_q ] ≤ τ_n·V + κ_n(1+κ_n)·L̄²,
    τ_n := a*(1+κ_n) + κ_n(2+κ_n),   a* ∈ [Σβ², 1] as in Lemma T.

*Proof.* Split Σβ_qJ_qZ_q = S + R with S := Σβ_qZ_q and
R := Σβ_q(J_q−1)Z_q. σ(S+R) ≤ σ(S) + σ(R) (L² triangle inequality —
valid under any correlation). σ(S)² ≤ a*·V is Lemma T with genuinely
FIXED coefficients β_q. For R: |R| ≤ κ_n·Σβ_q|Z_q| almost surely, so by
Jensen on the convex combination E[R²] ≤ κ_n²·Σβ_q E[Z_q²]
≤ κ_n²(V + L̄²). Expanding (σ(S)+σ(R))² and absorbing the cross term via
2√(a*V)·κ√(V+L̄²) ≤ κ(a*V + V + L̄²) and √a* ≤ 1 gives the claim. ∎

**Consequences.** (i) Theorem V's unrolling survives with transfer
factors τ_k in place of a*(1+Cε)²: τ_k ≤ (1+2κ_k)² at the a*=1 endpoint,
Σκ_k < ∞, so the product stays depth- and scene-scale-independent; the
new κ(1+κ)L̄² terms join the per-level injections (each O(ε_n)L_max²,
summable). (ii) Constants, honestly: κ is NOT small at level 0
(κ₀ = ε₀/(1−ε₀) ≈ 2.41 at the unjittered defaults), and τ₀ depends on
the unknown correlation position a*: **worst case (a* = 1, arbitrary
parent correlation) τ₀ = 1 + 3κ₀ + κ₀² ≈ 14.1**; the near-independent
end (a* ≈ Σβ² ≈ 1/4) gives ≈ 11.5. One finite level-0 factor either
way, N-independent, decaying fast with level (κ₁ ≈ 0.39 ⇒ τ₁ ≤ 2.4;
κ₂ ≈ 0.16 ⇒ τ₂ ≤ 1.6). Orders and depth-uniformity are what the theorem
claims; these constants are what they cost.
(iii) Scope, explicitly: this closes the random-coefficient gap for the
SINGLE-BIN ρ=0 single-frame merge. Still open: (a) ρ > 0 — m_q becomes
survivor-dependent and couples to the Prop-V unanimity machinery (round-2
gap 2); (b) the WINDOWED merge's variance bookkeeping — its per-slot
balance weights m = β/D(y) are sample-dependent beyond a small
perturbation of fixed β, so this lemma does not transfer verbatim
(empirically the windowed and single-bin merges are value-identical on
typical scenes at certified widths, E17, but that is a measurement, not
the missing lemma). The certified-coverage configuration (windowed) and
the fully-proved-variance configuration (single-bin) therefore do not yet
coincide; closing (b) unifies them. Baton, verbatim: **Lemma T′ closed
for single-bin, single-frame, ρ=0, ε_n<1; it does not yet close
Theorem V for the certified windowed estimator.**

## 3. Theorem V (depth-uniform variance non-amplification — single-frame)

("Theorem V" = variance; distinct from Prop V, the validation-bias result of
[lemma-3_2.md](lemma-3_2.md) §5.)

**Scope [restated 2026-07-23g]: single-bin merge, single frame, ρ = 0,
ε_k < 1 at every level (Lemma T′'s hypothesis — jitter splits pushing
ε₀ ≥ 1 fall outside). This theorem does NOT yet cover the certified
windowed estimator (Lemma T′ (iii)).**

Let ι′_n := λ(1+κ_n)μ_n + Var_cand,n + κ_n(1+κ_n)·L̄² be the per-level
injection (defensive selection noise + the own-candidate Bernoulli noise,
Var_cand,n ≤ μ_n·L_max crude, + Lemma T′'s coefficient-fluctuation term).
Combining Lemmas S and T′ by the law of total variance and unrolling
n = N−1..0:

    V_0 ≤ Σ_n [ Π_{k<n} τ_k ] · ι′_n,      τ_k = a*_k(1+κ_k) + κ_k(2+κ_k)
        ≤ exp( Σ_k (3κ_k + κ_k²) ) · Σ_n ι′_n     (τ ≤ 1+3κ+κ² at a* = 1),

finite since Σ_k κ_k < ∞ (κ_k = ε_k/(1−ε_k), ε_k geometric). [The
original statement used Lemma T's fixed-coefficient transfer
(1+Cε)²·a*; Lemma T′ replaces it after the random-coefficient review
gap.]

**The cascade does not amplify variance: the total is at most a
scene-scale- and depth-independent constant C_var := Π_n τ_n times the
sum of per-level single-sample injections.** Numerically, quote C_var,
not the exponent: at the unjittered defaults with a* = 1 (arbitrary
parent correlation) the infinite-depth product is
C_var = Π τ_n ≈ 72.9 (τ_n ≈ 14.04, 2.34, 1.49, 1.22, 1.10, → 1), while
the closed form exp(Σ(3κ_n+κ_n²)) ≈ 4.5×10⁶ is a deliberately loose
analytic envelope of the same product — an upper bound's upper bound,
never a value to report. The penumbra condition — the same ε_n
engine that bounds every bias term — also bounds the variance compounding.
Contraction (τ_n < 1) now requires a* < (1−2κ_n−κ_n²)/(1+κ_n), which is
satisfiable only for κ_n < √2−1 ≈ 0.414. At the defaults this rules out
contraction at n = 0 (κ₀ ≈ 2.41) and effectively at n = 1 (threshold
a* < 0.04, below the bilinear minimum Σβ² ≥ 1/4); from n ≈ 2 on
(threshold ≈ 0.58) near-independent parents do contract and deep levels
contribute geometrically less to V_0. The old fixed-coefficient
condition a* < 1/(1+Cε)² is superseded.

Connection to GRIS Thm 1: our per-level Var[Σ_q w_q] is what enters their
b-term (Eq 30); Lemma S bounds it by (μ+λJ̄)² − μ²-type quantities, giving a
per-level instantiation of their bound with explicit constants instead of
the vacuous chained C_f.

## 4. Temporal reservoir as an AR(1) chain (predicts E8)

Fixed entry, stationary scene, with temporal confidence cap M (written
"cap" below; --mcap0 in the CLI, M_n in the lab log), γ := 1/(1+cap). Each
frame the
fresh candidate (value Z_t, drive noise σ₁²) merges with prev via
m_new = γ, m_prev = 1−γ. On the retained branch (c unchanged):

    W_t = (1−γ)·W_{t−1} + γ·p̂(Z_t)/p̂(c)      — an AR(1) with coefficient 1−γ.

Predictions vs. the cap-sweep measurements (lab log [E12](../experiments.md);
coarser sweep in E8; L0, static segment):

| quantity | model | cap=2 | cap=8 | cap=32 |
|---|---|---|---|---|
| lag-1 autocorr | 1−γ = cap/(1+cap) | pred .667 / meas .645 | .889 / .848 | .970 / .905 |
| flicker CV | ∝ √(γ/(2−γ)): exponent ½ in (1+cap) | measured exponent ≈ 0.38±0.05, single fit across caps (E12) | — | — |
| adaptation (no invalidation) | ~1/γ = 1+cap frames per e-fold | consistent with E8's "ON ≈ M frames" (all caps) | — | — |
| adaptation (hard invalidation) | M→0 ⇒ γ=1, ~cascade-depth frames | measured 3 frames, E9 ✓ | — | — |

Reading: cap=2 matches the model closely; higher caps measure *below* the model because
sample *switching* (prob ≈ γ per frame, p̂-weighted) and cross-level coupling
decorrelate beyond the pure W-recursion — both push in the observed
direction. The flicker exponent gap (0.38 vs 0.5) is the switch-jump shot
noise plus the parent-chain noise floor inherited through the fresh
candidate's tail; a two-component model (W-AR(1) + switch process ∝ √γ)
brackets the measurements. Status: the single-component model captures the
scaling to ~15% and *predicts the direction of every discrepancy*; the
two-component refinement is future work.

The blink asymmetry of E8 falls out: turn-OFF is near-instant (2–3 frames,
E8) because temporal
re-evaluation rescales c multiplicatively (no chain dynamics involved);
turn-ON is chain-limited at rate γ unless invalidation resets M (γ→1).

## 5. What remains open

- **Temporal×hierarchy combined variance (opened by external review
  2026-07-23b):** with temporal reuse, parents consulted by the next
  frame's merges carry temporal-merged W ≠ 1; Lemmas S/T do not cover
  this chain. Empirically the capped cascade chain is stable (2D, caps up
  to 64), and the flat control measures what UNBOUNDED stored-W chaining
  does (E16: exponential divergence at cap 32) — the per-level collapse +
  M-cap are the structural mitigations, but a moment bound for the capped
  temporal chain is an open problem.
- The a* bracket's empirical position (inter-level ESS measurement — CPU
  oracle instrumentation, cheap).
- Two-component temporal model (switch + AR(1)) fitted to the E8 sweep.
- Var_cand,n refinement: μL_max is crude; the stratified-jitter candidate is
  better than Bernoulli within smooth bins.
- Spatial reuse (when added) enters Lemma T as a second averaging with its
  own correlation bracket — same machinery applies.
