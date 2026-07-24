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
Until that page of analysis is written out, Lemma T is proved as stated
for fixed coefficients; the random-coefficient extension is an explicit
open item.

**Inter-level correlation (the proposal's "risk #1"; cf. E8's red-line
test), quantified:** it moves a* within [Σm², 1] —
from 4× variance reduction per merge toward none — but *never past 1*:
correlation slows convergence, it cannot cause divergence. This is the
rigorous form of the proposal's "downgraded from variance explosion
to slower convergence". The empirical
position of a* is measurable via inter-level ESS (future work; the E8
autocorrelation is the temporal analogue).

## 3. Theorem V (depth-uniform variance non-amplification — single-frame)

("Theorem V" = variance; distinct from Prop V, the validation-bias result of
[lemma-3_2.md](lemma-3_2.md) §5.)

Let ι_n := λ(1+Cε_n)μ_n + Var_cand,n be the per-level injection (defensive
selection noise + the own-candidate Bernoulli noise, Var_cand,n ≤ μ_n·L_max
crude). Combining Lemmas S and T by the law of total variance and unrolling
n = N−1..0:

    V_0 ≤ Σ_n [ Π_{k<n} (1+Cε_k)²·a*_k ] · ι_n
        ≤ e^{6Cε₀} · Σ_n ι_n,          since Σ_k ε_k < 3ε₀ (lemma-3_2 §0).

**The cascade does not amplify variance: the total is at most a
scene-scale- and depth-independent constant e^{6Cε₀} times the sum of
per-level single-sample injections.** The penumbra condition — the same ε_n
engine that bounds every bias term — also bounds the variance compounding.
With independent-ish parents (a* < 1/(1+Cε)²) the product *contracts* and
deep levels contribute geometrically less to V_0.

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
