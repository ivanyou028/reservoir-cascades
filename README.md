# Reservoir Cascades

**Principled multi-scale sample reuse by resampling Radiance Cascade merges.**

Radiance Cascades (RC) discretize the radiance field over a hierarchy whose
resolutions follow the penumbra condition — but their deterministic
interpolation merge causes structural bias: light leaks, ringing, and a hard
angular-resolution ceiling. This project replaces that merge with
ReSTIR/GRIS-style resampled importance sampling, converting the structural
bias into variance that spatiotemporal reuse amortizes. The penumbra
condition that dictates RC's discretization turns out to *bound* the bias
and variance of the resampled merge: reuse radius ∝ 2ⁿ per level is a
theorem, not a tuning knob.

- **Interactive demo (WebGPU):** https://ivanyou028.github.io/reservoir-cascades/webgpu/
- **Paper preprint:** [10.5281/zenodo.21506521](https://doi.org/10.5281/zenodo.21506521) (arXiv version coming)
- **Theory notes:** [docs/theory/](docs/theory/) — full statements and proofs
  behind the paper's §5 (GRIS anchoring, integrand-mismatch, variance,
  temporal AR(1) model)
- **Lab log:** [docs/experiments.md](docs/experiments.md) — every experiment
  with configs and seeds (E1–E12)

## Quick start

CPU reference implementation (zero dependencies, C++17):

```sh
make            # builds build/rc with clang++
make test       # bit-exact regression: degenerate mode == vanilla RC

# render a scene: reference + vanilla RC + ours, with metrics
./build/rc eval --scene scenes/s2.json --frames 128 --rho 0.5

# dynamic-light benchmark (step response, flicker, autocorrelation)
./build/rc s3 --frames 192 --blink 32 --mcap0 32
```

WebGPU demo (any static server from the repo root):

```sh
python3 -m http.server 8123
# open http://localhost:8123/webgpu/
```

## Layout

| path | contents |
|---|---|
| `core/` | backend-agnostic algorithm: scene/tracing, vanilla RC (independent oracle), the unified reservoir framework |
| `cpu-ref/` | CLI driver: `selftest`, `eval`, `s3` |
| `webgpu/` | WGSL compute implementation + interactive demo (120 fps @ 256²) |
| `scenes/` | S1–S3 scene definitions (JSON, shared by both backends) |
| `metrics/` | MAPE / mask-energy metrics |
| `docs/theory/` | theory documents (lemmas, propositions, proofs) |

## Reproducing the paper's numbers

Every number in the paper is regenerable: Table 1 is `eval` over seeds 1–6
at `--rho {0.25,0.5,1}` (the `vanfix` row is the community bilinear-fix
baseline, reported by the same command); Table 2 adds `--stoch` /
`--no-temporal`; Figure 2's sweeps are scripted in the lab log (E5–E9).
The flat world-space reuse control (Table 3, lab log E16) is
`scripts/flat_sweep.sh` — 420 runs of the `compare` subcommand at the
cascade's exact candidate budget — aggregated by `scripts/flat_agg.py`.
The degenerate-mode selftest is the standing guarantee that "vanilla RC"
means *exactly* vanilla RC.

## Status

2D flatland, direct emission — the correctness laboratory. 3D and
multi-bounce are milestone M4 (see the paper's discussion section).

## Citation

```bibtex
@misc{You2026ReservoirCascades,
  title  = {Reservoir Cascades: Principled Multi-Scale Sample Reuse
            by Resampling Radiance Cascade Merges},
  author = {You, Ivan},
  year   = {2026},
  note   = {Preprint, doi:10.5281/zenodo.21506521},
}
```
