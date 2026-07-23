// Reservoir Cascades — WebGPU compute port of the CPU oracle (core/restir.cpp).
// f32 throughout; SDF sphere tracing (shadertoy parity) instead of the CPU
// oracle's analytic intersections. Algorithm structure mirrors the CPU path:
// per-parent reprojected bin selection, validate-before-select with β
// renormalization, per-level W collapse, temporal re-evaluation via immutable
// (cRef, emitLum0), block boundary jitter + hard resets driven from JS.

struct FrameU {
  frame        : u32,
  mode         : u32,   // 0 = full, 1 = vanilla (deterministic value-passing)
  invalidate   : u32,   // 1 => drop temporal history this frame
  shapeCount   : u32,
  rho          : f32,
  lambda       : f32,
  mcap0        : f32,
  mcapMax      : f32,
  t0           : f32,   // jittered level-0 interval length (pixels)
  emissionScale: f32,   // blink drive for S3
  seed         : u32,
  levels       : u32,
  P0           : u32,
  B0           : u32,
  _pad0        : u32,
  _pad1        : u32,
};

struct LevelU { n : u32, _p0 : u32, _p1 : u32, _p2 : u32 };

struct Shape {
  kindPad  : vec4<u32>,  // x: 0 circle, 1 box
  geo      : vec4<f32>,  // center.xy, (radius,_)/(half.xy)
  anglePad : vec4<f32>,  // x: angle
  emission : vec4<f32>,
};

struct Rsv {
  d0 : vec4<f32>,  // omega, y.x, y.y, W
  d1 : vec4<f32>,  // cRef.rgb, M
  d2 : vec4<f32>,  // emitLum0, flags(bit0 hasY), 0, 0
};

@group(0) @binding(0) var<uniform> U : FrameU;
@group(0) @binding(1) var<storage, read> shapes : array<Shape>;
@group(0) @binding(2) var<uniform> L : LevelU;

@group(1) @binding(0) var<storage, read_write> Rcur : array<Rsv>;
@group(1) @binding(1) var<storage, read> Rpar : array<Rsv>;   // level n+1, this frame
@group(1) @binding(2) var<storage, read> Rprev : array<Rsv>;  // level n, last frame

const PI2 : f32 = 6.28318530718;

// ---------- rng (pcg) ----------
fn pcg(v : u32) -> u32 {
  let s = v * 747796405u + 2891336453u;
  let w = ((s >> ((s >> 28u) + 4u)) ^ s) * 277803737u;
  return (w >> 22u) ^ w;
}
fn rnd(state : ptr<function, u32>) -> f32 {
  *state = pcg(*state);
  return f32(*state) * 2.3283064e-10;
}

// ---------- scene ----------
fn shapeSdf(si : u32, p : vec2<f32>) -> f32 {
  let s = shapes[si];
  if (s.kindPad.x == 0u) {
    return length(p - s.geo.xy) - s.geo.z;
  }
  let a = -s.anglePad.x;
  let ca = cos(a); let sa = sin(a);
  let rp = p - s.geo.xy;
  let lp = vec2<f32>(ca * rp.x - sa * rp.y, sa * rp.x + ca * rp.y);
  let q = abs(lp) - vec2<f32>(s.geo.z, s.geo.w);
  return length(max(q, vec2<f32>(0.0))) + min(max(q.x, q.y), 0.0);
}
fn sceneSdf(p : vec2<f32>) -> vec2<f32> { // (dist, shapeIdx)
  var best = 1e30; var bi = -1.0;
  for (var i = 0u; i < U.shapeCount; i++) {
    let d = shapeSdf(i, p);
    if (d < best) { best = d; bi = f32(i); }
  }
  return vec2<f32>(best, bi);
}
// Sphere trace within [t0,t1). Returns (t, shapeIdx) or shapeIdx<0 on escape.
fn march(o : vec2<f32>, d : vec2<f32>, tBeg : f32, tEnd : f32) -> vec2<f32> {
  var t = tBeg;
  for (var it = 0; it < 160; it++) {
    if (t >= tEnd) { break; }
    let ds = sceneSdf(o + d * t);
    if (ds.x < 0.1) { return vec2<f32>(t, ds.y); }
    t += max(ds.x, 0.2);
  }
  return vec2<f32>(t, -1.0);
}
fn emissionAt(si : u32) -> vec3<f32> {
  return shapes[si].emission.rgb * U.emissionScale;
}
// Current emitter luminance at stored point y (temporal re-evaluation).
fn emitLumNow(y : vec2<f32>) -> f32 {
  let ds = sceneSdf(y);
  if (abs(ds.x) > 1.5 || ds.y < 0.0) { return 0.0; }
  let e = emissionAt(u32(ds.y));
  return (e.r + e.g + e.b) / 3.0;
}
fn lum(c : vec3<f32>) -> f32 { return (c.r + c.g + c.b) / 3.0; }
fn phat(c : vec3<f32>) -> f32 { return lum(c) + U.lambda; }

// ---------- cascade helpers ----------
fn gridN(n : u32) -> u32 { return U.P0 >> n; }
fn bins(n : u32) -> u32 { return U.B0 << (2u * n); }
fn intervalStart(n : u32) -> f32 {
  return U.t0 * (pow(4.0, f32(n)) - 1.0) / 3.0;
}
fn probePos(n : u32, ij : vec2<u32>) -> vec2<f32> {
  let s = f32(1u << n);
  return (vec2<f32>(ij) + vec2<f32>(0.5)) * s;
}
fn binOf(n : u32, theta : f32) -> u32 {
  var t = theta / PI2;
  t = t - floor(t);
  return min(u32(t * f32(bins(n))), bins(n) - 1u);
}
fn levelIndex(n : u32, i : u32, j : u32, b : u32) -> u32 {
  return (j * gridN(n) + i) * bins(n) + b;
}

// Live radiance of a stored sample under the current scene.
fn liveC(r : Rsv) -> vec3<f32> {
  let hasY = (u32(r.d2.y) & 1u) == 1u;
  if (hasY && r.d2.x > 0.0) {
    return r.d1.rgb * (emitLumNow(r.d0.yz) / r.d2.x);
  }
  return r.d1.rgb;
}

// ---------- merge kernel: one dispatch per level, top-down ----------
@compute @workgroup_size(64)
fn mergeLevel(@builtin(global_invocation_id) gid : vec3<u32>) {
  let n = L.n;
  let G = gridN(n);
  let B = bins(n);
  let total = G * G * B;
  let k = gid.x;
  if (k >= total) { return; }
  let b = k % B;
  let i = (k / B) % G;
  let j = k / (B * G);
  let p = probePos(n, vec2<u32>(i, j));
  let vanilla = U.mode == 1u;

  var st : u32 = pcg(U.seed ^ (U.frame * 2654435761u) ^ (n * 97u) ^ k);

  var xi = 0.5;
  if (!vanilla) { xi = rnd(&st); }
  let omega = PI2 * (f32(b) + xi) / f32(B);
  let dir = vec2<f32>(cos(omega), sin(omega));
  let tBeg = intervalStart(n);
  let tEnd = intervalStart(n + 1u);
  let hit = march(p, dir, max(tBeg, 0.3), tEnd);

  var cRef = vec3<f32>(0.0);
  var y = vec2<f32>(0.0);
  var hasY = false;
  var emitLum0 = 0.0;

  if (hit.y >= 0.0) {
    cRef = emissionAt(u32(hit.y));
    y = p + dir * hit.x;
    hasY = true;
    emitLum0 = lum(cRef);
  } else if (n + 1u < U.levels) {
    // bilinear parents in level n+1 grid, canonical order
    let Gp = gridN(n + 1u);
    let sp = f32(1u << (n + 1u));
    let uv = p / sp - vec2<f32>(0.5);
    var i0 = i32(floor(uv.x)); var j0 = i32(floor(uv.y));
    var fu = uv.x - f32(i0);  var fv = uv.y - f32(j0);
    if (i0 < 0) { i0 = 0; fu = 0.0; }
    if (j0 < 0) { j0 = 0; fv = 0.0; }
    if (i0 > i32(Gp) - 2) { i0 = i32(Gp) - 2; fu = 1.0; }
    if (j0 > i32(Gp) - 2) { j0 = i32(Gp) - 2; fv = 1.0; }
    var pidx : array<vec2<u32>, 4>;
    pidx[0] = vec2<u32>(u32(i0), u32(j0));
    pidx[1] = vec2<u32>(u32(i0 + 1), u32(j0));
    pidx[2] = vec2<u32>(u32(i0), u32(j0 + 1));
    pidx[3] = vec2<u32>(u32(i0 + 1), u32(j0 + 1));
    var beta = array<f32, 4>(
      (1.0 - fu) * (1.0 - fv), fu * (1.0 - fv), (1.0 - fu) * fv, fu * fv);

    if (vanilla) {
      // value-passing over the 4 aligned child bins (vanilla RC merge)
      var acc = vec3<f32>(0.0);
      for (var q = 0u; q < 4u; q++) {
        var childSum = vec3<f32>(0.0);
        for (var cb = 0u; cb < 4u; cb++) {
          let kp = levelIndex(n + 1u, pidx[q].x, pidx[q].y, 4u * b + cb);
          childSum += Rpar[kp].d1.rgb * Rpar[kp].d0.w;
        }
        acc += childSum * 0.25 * beta[q];
      }
      cRef = acc;
    } else {
      // ---- merge-as-RIS with reprojection + renormalized validation ----
      let tRep = sqrt(intervalStart(n + 1u) * intervalStart(n + 2u));
      let zRep = p + dir * tRep;
      let doValidate = U.rho > 0.0 && rnd(&st) < U.rho;
      var w : array<f32, 4>;
      var Jq : array<f32, 4>;
      var valid : array<bool, 4>;
      var kpAr : array<u32, 4>;
      var betaValid = 0.0;
      for (var q = 0u; q < 4u; q++) {
        let qpos = probePos(n + 1u, pidx[q]);
        let toZ = zRep - qpos;
        let bp = binOf(n + 1u, atan2(toZ.y, toZ.x));
        kpAr[q] = levelIndex(n + 1u, pidx[q].x, pidx[q].y, bp);
        let rq = Rpar[kpAr[q]];
        Jq[q] = 1.0;
        valid[q] = true;
        let qHasY = (u32(rq.d2.y) & 1u) == 1u;
        if (qHasY) {
          let yq = rq.d0.yz;
          let rQ = length(yq - qpos);
          let rP = length(yq - p);
          Jq[q] = select(0.0, rQ / rP, rP > 1e-6);
          if (doValidate) {
            let dv = yq - p;
            let dist = length(dv);
            let tE = dist - max(1.0, 0.002 * dist);
            let tB = tEnd * 0.999;
            if (dist > 1e-6 && tE > tB) {
              let h = march(p, dv / dist, tB, tE);
              if (h.y >= 0.0) { valid[q] = false; }
            }
          }
        }
        if (valid[q]) { betaValid += beta[q]; }
      }
      var wsum = 0.0;
      for (var q = 0u; q < 4u; q++) {
        let rq = Rpar[kpAr[q]];
        var m = 0.0;
        if (valid[q] && betaValid > 0.0) { m = beta[q] / betaValid; }
        let cl = liveC(rq);
        var wq = m * phat(cl) * Jq[q] * rq.d0.w;
        if (!(wq > 0.0)) { wq = 0.0; }
        w[q] = wq;
        wsum += wq;
      }
      if (wsum > 0.0) {
        let u = rnd(&st) * wsum;
        var sel = 3u;
        var run = 0.0;
        for (var q = 0u; q < 4u; q++) {
          run += w[q];
          if (u < run) { sel = q; break; }
        }
        let rs = Rpar[kpAr[sel]];
        let cl = liveC(rs);
        let Wsel = wsum / phat(cl);
        cRef = cl * Wsel;                 // W collapse (§3.5)
        y = rs.d0.yz;
        hasY = (u32(rs.d2.y) & 1u) == 1u;
        emitLum0 = rs.d2.x;
      }
    }
  }

  var outR : Rsv;
  outR.d0 = vec4<f32>(omega, y.x, y.y, 1.0);
  outR.d1 = vec4<f32>(cRef, 1.0);
  outR.d2 = vec4<f32>(emitLum0, select(0.0, 1.0, hasY), 0.0, 0.0);

  // ---- temporal merge (full mode only) ----
  if (!vanilla && U.invalidate == 0u) {
    let pv = Rprev[k];
    let cap = min(U.mcap0 * f32(1u << n), U.mcapMax);
    let Mprev = min(pv.d1.w, cap);
    if (Mprev > 0.0) {
      let cPrevLive = liveC(pv);
      let mN = 1.0 / (1.0 + Mprev);
      let mP = Mprev / (1.0 + Mprev);
      let wN = mN * phat(outR.d1.rgb) * outR.d0.w;
      let wP = mP * phat(cPrevLive) * pv.d0.w;
      let ws = wN + wP;
      if (ws > 0.0) {
        let takeNew = rnd(&st) * ws < wN;
        var merged : Rsv;
        if (takeNew) { merged = outR; } else { merged = pv; }
        let cSel = select(cPrevLive, outR.d1.rgb, takeNew);
        merged.d0.w = ws / phat(cSel);
        merged.d1.w = 1.0 + Mprev;
        outR = merged;
      } else {
        outR.d1.w = 1.0 + Mprev;
        outR.d0.w = 0.0;
      }
    }
  }
  Rcur[k] = outR;
}

// ---------- gather: level 0 -> image ----------
@group(1) @binding(3) var outTex : texture_storage_2d<rgba16float, write>;

@compute @workgroup_size(8, 8)
fn gather(@builtin(global_invocation_id) gid : vec3<u32>) {
  if (gid.x >= U.P0 || gid.y >= U.P0) { return; }
  let p = vec2<f32>(gid.xy) + vec2<f32>(0.5);
  let ds = sceneSdf(p);
  var acc = vec3<f32>(0.0);
  if (ds.x < 0.0) {
    acc = emissionAt(u32(ds.y));
  } else {
    let B = bins(0u);
    for (var b = 0u; b < B; b++) {
      let r = Rcur[levelIndex(0u, gid.x, gid.y, b)];
      if (U.mode == 1u) {
        acc += r.d1.rgb;
      } else {
        acc += liveC(r) * r.d0.w;
      }
    }
    acc = acc / f32(B);
  }
  textureStore(outTex, vec2<i32>(gid.xy), vec4<f32>(acc, 1.0));
}

// ---------- fullscreen present ----------
struct VSOut { @builtin(position) pos : vec4<f32>, @location(0) uv : vec2<f32> };

@vertex
fn vsMain(@builtin(vertex_index) vi : u32) -> VSOut {
  var out : VSOut;
  let xy = vec2<f32>(f32((vi << 1u) & 2u), f32(vi & 2u));
  out.pos = vec4<f32>(xy * 2.0 - 1.0, 0.0, 1.0);
  out.uv = vec2<f32>(xy.x, 1.0 - xy.y);
  return out;
}

@group(0) @binding(0) var presTex : texture_2d<f32>;

@fragment
fn fsMain(in : VSOut) -> @location(0) vec4<f32> {
  let dim = vec2<f32>(textureDimensions(presTex));
  let c = textureLoad(presTex, vec2<i32>(in.uv * dim), 0).rgb;
  let t = c / (vec3<f32>(1.0) + c);            // Reinhard
  return vec4<f32>(pow(t, vec3<f32>(1.0 / 2.2)), 1.0);
}
