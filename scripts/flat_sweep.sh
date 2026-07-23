#!/bin/bash
# Flat-vs-cascade equal-budget sweep (§6 "does the cascade earn its
# structure?"). Emits out/flat_sweep.csv; aggregate with flat_agg.py.
# Runs are independent → parallelized with xargs -P.
set -e
cd "$(dirname "$0")/.."
make >/dev/null
CSV=out/flat_sweep.csv
echo "scene,mode,radius,k,rho,mcap,seed,leakPct,recPct,mape,frameMape,cand,valid" > $CSV

JOBS=()
for scene in scenes/s1.json scenes/s2.json; do
  for seed in 1 2 3 4 5 6; do
    # cascade reference points (identical pipeline, identical metrics)
    for rho in 0 0.25 0.5; do
      JOBS+=("compare --scene $scene --mode cascade --rho $rho --seed $seed --frames 128 --csv $CSV")
    done
    # flat: temporal-only anchor (radius/rho/mcap-32 irrelevant at r=0)
    JOBS+=("compare --scene $scene --mode flat --radius 0 --seed $seed --frames 128 --csv $CSV")
    JOBS+=("compare --scene $scene --mode flat --radius 0 --mcap 32 --seed $seed --frames 128 --csv $CSV")
    # flat: spatial sweep — radius x rho x cap
    for r in 2 4 8 16 32; do
      for rho in 0 0.5 1; do
        for mcap in 8 32; do
          JOBS+=("compare --scene $scene --mode flat --radius $r --rho $rho --mcap $mcap --seed $seed --frames 128 --csv $CSV")
        done
      done
    done
  done
done

printf '%s\n' "${JOBS[@]}" | xargs -P "${1:-6}" -I{} sh -c './build/rc {} >/dev/null'
echo "done: $(($(wc -l < $CSV) - 1)) rows in $CSV"
