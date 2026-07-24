#!/usr/bin/env python3
"""Aggregate out/flat_sweep.csv -> per-config mean±sd + markdown table.
Columns: scene,mode,radius,k,rho,mcap,seed,leakPct,recPct,mape,frameMape,cand,
         valid,candLen,validLen
candLen/validLen are total traced distance (px); the table reports mean
candidate traced length per ray (candLen/cand) — equal ray count, unequal work
(cascade interval-bounded vs. flat full-length). Older CSVs without those two
columns still aggregate (the traced column shows "—").
"""
import csv, json, math, sys
from collections import defaultdict

rows = list(csv.DictReader(open("out/flat_sweep.csv")))
groups = defaultdict(list)
for r in rows:
    key = (r["scene"], r["mode"], float(r["radius"]), float(r["rho"]),
           int(r["mcap"]))
    groups[key].append(r)

def ms(vals):
    m = sum(vals) / len(vals)
    sd = math.sqrt(sum((v - m) ** 2 for v in vals) / max(1, len(vals) - 1))
    return m, sd

agg = []
for key, g in sorted(groups.items()):
    scene, mode, radius, rho, mcap = key
    leak = [float(r["leakPct"]) for r in g if float(r["leakPct"]) >= 0]
    rec = [float(r["recPct"]) for r in g if float(r["recPct"]) >= 0]
    mape = [float(r["mape"]) for r in g]
    fmape = [float(r["frameMape"]) for r in g]
    extra = [100.0 * float(r["valid"]) / float(r["cand"]) for r in g]
    tcand = [float(r.get("candLen", 0) or 0) / float(r["cand"])
             for r in g if float(r["cand"]) > 0 and r.get("candLen")]
    row = dict(scene=scene, mode=mode, radius=radius, rho=rho, mcap=mcap,
               n=len(g))
    if leak: row["leak"], row["leak_sd"] = ms(leak)
    if rec: row["rec"], row["rec_sd"] = ms(rec)
    row["mape"], row["mape_sd"] = ms(mape)
    row["fmape"], _ = ms(fmape)
    row["extraRays"], _ = ms(extra)
    if tcand: row["tracedCand"], _ = ms(tcand)
    agg.append(row)

json.dump(agg, open("out/flat_sweep_agg.json", "w"), indent=1)

def fmt(r, k, sk):
    return f"{r[k]:.1f}±{r[sk]:.1f}" if k in r else "—"

for scene in ["s1_thin_occluder", "s2_pinhole_small_source"]:
    print(f"\n## {scene}")
    print("| mode | r | rho | cap | leak% | rec% | MAPE | frameMAPE | +rays% "
          "| cand px/ray |")
    print("|---|---|---|---|---|---|---|---|---|---|")
    for r in agg:
        if r["scene"] != scene: continue
        metric = fmt(r, "leak", "leak_sd") if scene.startswith("s1") \
            else fmt(r, "rec", "rec_sd")
        lk = fmt(r, "leak", "leak_sd")
        rc = fmt(r, "rec", "rec_sd")
        tc = f"{r['tracedCand']:.1f}" if "tracedCand" in r else "—"
        print(f"| {r['mode']} | {r['radius']:.0f} | {r['rho']:.2f} "
              f"| {r['mcap']} | {lk} | {rc} | {r['mape']:.3f}±{r['mape_sd']:.3f} "
              f"| {r['fmape']:.3f} | {r['extraRays']:.0f} | {tc} |")
