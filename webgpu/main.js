// Reservoir Cascades — WebGPU demo harness.
// Shares scene JSONs with the CPU oracle (served from ../scenes/).

const P0 = 256, B0 = 4, T0 = 4.0;

const $ = id => document.getElementById(id);
const status = msg => { $('status').textContent = msg; };

function levelsFor(size) {
  const diag = Math.SQRT2 * size;
  let n = 0;
  const start = m => T0 * (Math.pow(4, m) - 1) / 3;
  while (start(n + 1) < diag) n++;
  return n + 1;
}

// splitmix-ish hash for JS-side per-block jitter (independent of GPU rng)
function hash32(a) {
  a |= 0; a = Math.imul(a ^ (a >>> 16), 0x45d9f3b);
  a = Math.imul(a ^ (a >>> 16), 0x45d9f3b);
  return ((a ^ (a >>> 16)) >>> 0) / 4294967296;
}

async function loadScene(name) {
  const res = await fetch(`../scenes/${name}.json`);
  const js = await res.json();
  const shapes = js.shapes.map(s => ({
    kind: s.type === 'circle' ? 0 : 1,
    center: [s.center[0] * P0, s.center[1] * P0],
    geo: s.type === 'circle' ? [s.radius * P0, 0] : [s.half[0] * P0, s.half[1] * P0],
    angle: s.angle || 0,
    emission: s.emission || [0, 0, 0],
  }));
  const masks = {};
  for (const [k, m] of Object.entries(js.masks || {}))
    masks[k] = { min: [m.min[0] * P0, m.min[1] * P0], max: [m.max[0] * P0, m.max[1] * P0] };
  return { name: js.name, shapes, masks };
}

async function main() {
  if (!navigator.gpu) { status('WebGPU not available in this browser'); return; }
  const adapter = await navigator.gpu.requestAdapter();
  if (!adapter) { status('No WebGPU adapter'); return; }
  const device = await adapter.requestDevice();
  let gpuErr = false;
  device.addEventListener('uncapturederror', e => {
    if (!gpuErr) { gpuErr = true; status('GPU error: ' + e.error.message.slice(0, 500)); }
  });
  const canvas = $('canvas');
  canvas.width = P0; canvas.height = P0;
  const ctx = canvas.getContext('webgpu');
  const format = navigator.gpu.getPreferredCanvasFormat();
  ctx.configure({ device, format });

  const wgsl = await (await fetch('shaders.wgsl')).text();
  const module = device.createShaderModule({ code: wgsl });
  const info = await module.getCompilationInfo();
  for (const m of info.messages) if (m.type === 'error') { status('WGSL: ' + m.message); return; }

  const N = levelsFor(P0);
  const entries = P0 * P0 * B0;         // identical count per level
  const RSV_BYTES = 48;

  const mkRsvBuf = () => device.createBuffer({
    size: entries * RSV_BYTES,
    usage: GPUBufferUsage.STORAGE | GPUBufferUsage.COPY_DST,
  });
  // [parity][level]
  const Rbufs = [[], []];
  for (let par = 0; par < 2; par++)
    for (let n = 0; n < N; n++) Rbufs[par].push(mkRsvBuf());

  const frameU = device.createBuffer({ size: 64, usage: GPUBufferUsage.UNIFORM | GPUBufferUsage.COPY_DST });
  const shapeBuf = device.createBuffer({ size: 16 * 64, usage: GPUBufferUsage.STORAGE | GPUBufferUsage.COPY_DST });
  const levelUs = [];
  for (let n = 0; n < N; n++) {
    const b = device.createBuffer({ size: 16, usage: GPUBufferUsage.UNIFORM | GPUBufferUsage.COPY_DST });
    device.queue.writeBuffer(b, 0, new Uint32Array([n, 0, 0, 0]));
    levelUs.push(b);
  }

  const outTex = device.createTexture({
    size: [P0, P0], format: 'rgba16float',
    usage: GPUTextureUsage.STORAGE_BINDING | GPUTextureUsage.TEXTURE_BINDING | GPUTextureUsage.COPY_SRC,
  });

  const mergePipe = device.createComputePipeline({
    layout: 'auto', compute: { module, entryPoint: 'mergeLevel' },
  });
  const gatherPipe = device.createComputePipeline({
    layout: 'auto', compute: { module, entryPoint: 'gather' },
  });
  const renderPipe = device.createRenderPipeline({
    layout: 'auto',
    vertex: { module, entryPoint: 'vsMain' },
    fragment: { module, entryPoint: 'fsMain', targets: [{ format }] },
    primitive: { topology: 'triangle-list' },
  });

  // bind groups: merge g0 per level; merge g1 per (parity, level); gather; render
  const mergeG0 = levelUs.map(lu => device.createBindGroup({
    layout: mergePipe.getBindGroupLayout(0),
    entries: [
      { binding: 0, resource: { buffer: frameU } },
      { binding: 1, resource: { buffer: shapeBuf } },
      { binding: 2, resource: { buffer: lu } },
    ],
  }));
  const mergeG1 = [[], []];
  for (let par = 0; par < 2; par++)
    for (let n = 0; n < N; n++) {
      const cur = Rbufs[par], prev = Rbufs[1 - par];
      mergeG1[par].push(device.createBindGroup({
        layout: mergePipe.getBindGroupLayout(1),
        entries: [
          { binding: 0, resource: { buffer: cur[n] } },
          // top level never reads its parent binding; bind a distinct buffer
          // (same-buffer read_write + read invalidates the command buffer)
          { binding: 1, resource: { buffer: n + 1 < N ? cur[n + 1] : prev[n] } },
          { binding: 2, resource: { buffer: prev[n] } },
        ],
      }));
    }
  const gatherG0 = device.createBindGroup({
    layout: gatherPipe.getBindGroupLayout(0),
    entries: [
      { binding: 0, resource: { buffer: frameU } },
      { binding: 1, resource: { buffer: shapeBuf } },
    ],
  });
  const gatherG1 = [0, 1].map(par => device.createBindGroup({
    layout: gatherPipe.getBindGroupLayout(1),
    entries: [
      { binding: 0, resource: { buffer: Rbufs[par][0] } },
      { binding: 3, resource: outTex.createView() },
    ],
  }));
  const renderG0 = device.createBindGroup({
    layout: renderPipe.getBindGroupLayout(0),
    entries: [{ binding: 0, resource: outTex.createView() }],
  });

  // readback for mask metrics
  const readBuf = device.createBuffer({
    size: P0 * P0 * 8, usage: GPUBufferUsage.COPY_DST | GPUBufferUsage.MAP_READ,
  });

  let scene = null, frame = 0, seed = 1, pendingInvalidate = true;

  async function setScene(name) {
    scene = await loadScene(name);
    const arr = new ArrayBuffer(16 * 4 * scene.shapes.length);
    const u32 = new Uint32Array(arr), f32 = new Float32Array(arr);
    scene.shapes.forEach((s, i) => {
      const o = i * 16;
      u32[o] = s.kind;
      f32[o + 4] = s.center[0]; f32[o + 5] = s.center[1];
      f32[o + 6] = s.geo[0]; f32[o + 7] = s.geo[1];
      f32[o + 8] = s.angle;
      f32[o + 12] = s.emission[0]; f32[o + 13] = s.emission[1]; f32[o + 14] = s.emission[2];
    });
    device.queue.writeBuffer(shapeBuf, 0, arr);
    frame = 0; pendingInvalidate = true;
    $('metrics').textContent = '';
  }

  await setScene($('scene').value);

  let lastT = performance.now(), fpsAcc = 0, fpsN = 0;
  let lastEmitScale = 1;

  async function tick() {
    const mode = $('mode').value === 'vanilla' ? 1 : 0;
    const rho = parseFloat($('rho').value);
    const mcap0 = parseInt($('mcap').value);
    const blockLen = parseInt($('block').value);
    const blink = $('blink').checked;
    $('rhoV').textContent = rho.toFixed(2);

    // blink drive (S3): square wave. ~1s half-period at 120fps so the
    // 3-frame adaptation is watchable rather than a strobe.
    const BLINK_P = 120;
    let emitScale = 1, changed = false;
    if (blink) {
      emitScale = (Math.floor(frame / BLINK_P) % 2 === 1) ? 0 : 1;
      changed = frame > 0 && frame % BLINK_P === 0;
    }
    lastEmitScale = emitScale;

    // block boundary jitter (full mode); hard reset at block switches
    let t0 = T0, splitSwitch = false;
    if (mode === 0 && blockLen > 0) {
      const blk = Math.floor(frame / blockLen);
      t0 = T0 * Math.pow(4, hash32(seed * 7919 + blk) - 0.5);
      splitSwitch = frame > 0 && frame % blockLen === 0;
    }
    const invalidate = (pendingInvalidate || changed || splitSwitch) ? 1 : 0;
    pendingInvalidate = false;

    const ub = new ArrayBuffer(64);
    const u32 = new Uint32Array(ub), f32 = new Float32Array(ub);
    u32[0] = frame; u32[1] = mode; u32[2] = invalidate; u32[3] = scene.shapes.length;
    f32[4] = rho; f32[5] = 0.05; f32[6] = mcap0; f32[7] = 64;
    f32[8] = t0; f32[9] = emitScale;
    u32[10] = seed; u32[11] = N; u32[12] = P0; u32[13] = B0;
    device.queue.writeBuffer(frameU, 0, ub);

    const par = frame & 1;
    const enc = device.createCommandEncoder();
    for (let n = N - 1; n >= 0; n--) {
      const pass = enc.beginComputePass();
      pass.setPipeline(mergePipe);
      pass.setBindGroup(0, mergeG0[n]);
      pass.setBindGroup(1, mergeG1[par][n]);
      pass.dispatchWorkgroups(Math.ceil(entries / 64));
      pass.end();
    }
    {
      const pass = enc.beginComputePass();
      pass.setPipeline(gatherPipe);
      pass.setBindGroup(0, gatherG0);
      pass.setBindGroup(1, gatherG1[par]);
      pass.dispatchWorkgroups(Math.ceil(P0 / 8), Math.ceil(P0 / 8));
      pass.end();
    }
    {
      const view = ctx.getCurrentTexture().createView();
      const pass = enc.beginRenderPass({
        colorAttachments: [{ view, loadOp: 'clear', storeOp: 'store', clearValue: { r: 0, g: 0, b: 0, a: 1 } }],
      });
      pass.setPipeline(renderPipe);
      pass.setBindGroup(0, renderG0);
      pass.draw(3);
      pass.end();
    }
    const wantMetrics = frame % 120 === 100;
    if (wantMetrics) {
      enc.copyTextureToBuffer({ texture: outTex }, { buffer: readBuf, bytesPerRow: P0 * 8 }, [P0, P0]);
    }
    device.queue.submit([enc.finish()]);

    if (wantMetrics && Object.keys(scene.masks).length) {
      await readBuf.mapAsync(GPUMapMode.READ);
      const half = new Uint16Array(readBuf.getMappedRange());
      const f16 = h => {
        const s = (h & 0x8000) ? -1 : 1, e = (h >> 10) & 0x1f, m = h & 0x3ff;
        if (e === 0) return s * m * Math.pow(2, -24);
        if (e === 31) return m ? NaN : s * Infinity;
        return s * (1 + m / 1024) * Math.pow(2, e - 15);
      };
      let out = [];
      for (const [name, m] of Object.entries(scene.masks)) {
        let sum = 0, cnt = 0;
        for (let y = Math.ceil(m.min[1]); y < m.max[1]; y++)
          for (let x = Math.ceil(m.min[0]); x < m.max[0]; x++) {
            const o = (y * P0 + x) * 4;
            sum += (f16(half[o]) + f16(half[o + 1]) + f16(half[o + 2])) / 3;
            cnt++;
          }
        out.push(`E(${name})=${(sum / cnt).toFixed(5)}`);
      }
      readBuf.unmap();
      $('metrics').textContent = out.join('   ') + (lastEmitScale === 0 ? '   [light OFF]' : '');
    }

    const now = performance.now();
    fpsAcc += now - lastT; lastT = now; fpsN++;
    if (fpsN === 30) {
      $('fps').textContent = (1000 / (fpsAcc / fpsN)).toFixed(0) + ' fps';
      fpsAcc = 0; fpsN = 0;
    }
    frame++;
    requestAnimationFrame(safeTick);
  }
  function safeTick() {
    tick().catch(e => { status('tick error: ' + (e.message || e)); console.error(e); });
  }

  $('scene').addEventListener('change', e => setScene(e.target.value));
  $('mode').addEventListener('change', () => { pendingInvalidate = true; });
  status(`ready — ${N} levels, ${P0}² probes, ${B0} bins @ L0`);
  requestAnimationFrame(safeTick);
}

main().catch(e => status('error: ' + e.message));
