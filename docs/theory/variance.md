# Variance analysis — the missing half of bias→variance

Status: v0.1, 2026-07-23. Discharges the top open item of
[gris-anchoring.md](gris-anchoring.md) §6: the chain-variance recursion that
replaces the depth-exponential a.s. C_f bound, plus the Markov analysis of the
temporal reservoir. Notation as in [lemma-3_2.md](lemma-3_2.md); scalar
analysis on luminance (RGB picks up a chroma factor κ = maxchannel/lum ≤ 3,
entering squared; noted where it matters).

## 0. Setup

Per (probe p, bin b, level n), single-frame no-temporal first. Escaped-case
tail: X_n = c_s·W_sel, W_sel = (Σ_q w_q)/p̂(c_s), w_q = m_q p̂(c_q) J_q W_q,
p̂(c) = c + λ (luminance), Σm_q = 1, J_q ≤ 1+Cε_n (Lemma J), W_q = 1 fresh.
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
is the §3.5 chromaticity stabilizer doing quantitative work). λ injects
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

**Risk #1, quantified:** inter-level correlation moves a* within [Σm², 1] —
from 4× variance reduction per merge toward none — but *never past 1*:
correlation slows convergence, it cannot cause divergence. This is the
rigorous form of the proposal's "downgraded from variance explosion
to slower convergence". The empirical
position of a* is measurable via inter-level ESS (future work; the E8
autocorrelation is the temporal analogue).

## 3. Theorem V (depth-uniform variance non-amplification)

Let ι_n := λ(1+Cε_n)μ_n + Var_cand,n be the per-level injection (defensive
selection noise + the own-candidate Bernoulli noise, Var_cand,n ≤ μ_n·L_max
crude). Combining Lemmas S and T by the law of total variance and unrolling
n = N−1..0:

    V_0 ≤ Σ_n [ Π_{k<n} (1+Cε_k)²·a*_k ] · ι_n
        ≤ e^{4Cε₀} · Σ_n ι_n,          since Σ_k ε_k < 2ε₀.

**The cascade does not amplify variance: the total is at most a
scene-scale- and depth-independent constant e^{4Cε₀} times the sum of
per-level single-sample injections.** The penumbra condition — the same ε_n
engine that bounds every bias term — also bounds the variance compounding.
With independent-ish parents (a* < 1/(1+Cε)²) the product *contracts* and
deep levels contribute geometrically less to V_0.

Connection to GRIS Thm 1: our per-level Var[Σ_q w_q] is what enters their
b-term (Eq 30); Lemma S bounds it by (μ+λJ̄)² − μ²-type quantities, giving a
per-level instantiation of their bound with explicit constants instead of
the vacuous chained C_f.

## 4. Temporal reservoir as an AR(1) chain (predicts E8)

Fixed entry, stationary scene, cap M' = cap, γ := 1/(1+cap). Each frame the
fresh candidate (value Z_t, drive noise σ₁²) merges with prev via
m_new = γ, m_prev = 1−γ. On the retained branch (c unchanged):

    W_t = (1−γ)·W_{t−1} + γ·p̂(Z_t)/p̂(c)      — an AR(1) with coefficient 1−γ.

Predictions vs. E8 measurements (L0, static segment):

| quantity | model | cap=2 | cap=8 | cap=32 |
|---|---|---|---|---|
| lag-1 autocorr | 1−γ = cap/(1+cap) | pred .667 / meas .651 | .889 / .849 | .970 / .908 |
| flicker CV | ∝ √(γ/(2−γ)) ⇒ exponent ½ in (1+cap) | measured exponent ≈ 0.38±0.05 |||
| adaptation (no invalidation) | ~1/γ = 1+cap frames per e-fold | consistent with E8's "ON ≈ M frames" |||
| adaptation (hard invalidation) | M→0 ⇒ γ=1, ~cascade-depth frames | measured 3 frames ✓ |||

Reading: cap=2 is dead-on; higher caps measure *below* the model because
sample *switching* (prob ≈ γ per frame, p̂-weighted) and cross-level coupling
decorrelate beyond the pure W-recursion — both push in the observed
direction. The flicker exponent gap (0.38 vs 0.5) is the switch-jump shot
noise plus the parent-chain noise floor inherited through the fresh
candidate's tail; a two-component model (W-AR(1) + switch process ∝ √γ)
brackets the measurements. Honest status: the single-component model
captures the scaling to ~15% and *predicts the direction of every
discrepancy*; the two-component refinement is future work.

The blink asymmetry of E8 falls out: turn-OFF is instant because temporal
re-evaluation rescales c multiplicatively (no chain dynamics involved);
turn-ON is chain-limited at rate γ unless invalidation resets M (γ→1).

## 5. What remains open

- The a* bracket's empirical position (inter-level ESS measurement — CPU
  oracle instrumentation, cheap).
- Two-component temporal model (switch + AR(1)) fitted to the E8 sweep.
- Var_cand,n refinement: μL_max is crude; the stratified-jitter candidate is
  better than Bernoulli within smooth bins.
- Spatial reuse (when added) enters Lemma T as a second averaging with its
  own correlation bracket — same machinery applies.
