// Independent hand-written vanilla Radiance Cascades (flatland, direct emission).
// This is the regression oracle for the unified framework's degenerate mode
// (§3.6): same seed ⇒ bit-identical output required.
#pragma once
#include "cascade.h"
#include "image.h"
#include "scene.h"

namespace rc {

// Renders fluence/2π per pixel (mean of level-0 merged bin radiance).
Image renderVanillaRC(const Scene& scene, const CascadeCfg& cfg);

} // namespace rc
