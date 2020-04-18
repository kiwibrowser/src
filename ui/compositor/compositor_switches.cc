// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/compositor/compositor_switches.h"

#include "base/command_line.h"
#include "build/build_config.h"

namespace switches {

// Enable compositing individual elements via hardware overlays when
// permitted by device.
// Setting the flag to "single-fullscreen" will try to promote a single
// fullscreen overlay and use it as main framebuffer where possible.
const char kEnableHardwareOverlays[] = "enable-hardware-overlays";

// Forces tests to produce pixel output when they normally wouldn't.
const char kEnablePixelOutputInTests[] = "enable-pixel-output-in-tests";

// Limits the compositor to output a certain number of frames per second,
// maximum.
const char kLimitFps[] = "limit-fps";

const char kUIEnableRGBA4444Textures[] = "ui-enable-rgba-4444-textures";

const char kUIEnableZeroCopy[] = "ui-enable-zero-copy";
const char kUIDisableZeroCopy[] = "ui-disable-zero-copy";

const char kUIShowPaintRects[] = "ui-show-paint-rects";

const char kUISlowAnimations[] = "ui-slow-animations";

const char kDisableVsyncForTests[] = "disable-vsync-for-tests";

}  // namespace switches

namespace features {

// If enabled, all draw commands recorded on canvas are done in pixel aligned
// measurements. This also enables scaling of all elements in views and layers
// to be done via corner points. See https://crbug.com/720596 for details.
const base::Feature kEnablePixelCanvasRecording {
  "enable-pixel-canvas-recording",
#if defined(OS_CHROMEOS)
      base::FEATURE_ENABLED_BY_DEFAULT
#else
      base::FEATURE_DISABLED_BY_DEFAULT
#endif
};

}  // namespace features

namespace ui {

bool IsUIZeroCopyEnabled() {
  // Match the behavior of IsZeroCopyUploadEnabled() in content/browser/gpu.
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
#if defined(OS_MACOSX)
  return !command_line.HasSwitch(switches::kUIDisableZeroCopy);
#else
  return command_line.HasSwitch(switches::kUIEnableZeroCopy);
#endif
}

bool IsPixelCanvasRecordingEnabled() {
  return base::FeatureList::IsEnabled(features::kEnablePixelCanvasRecording);
}

}  // namespace ui
