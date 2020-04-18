// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/browser/deferred_gpu_command_service.h"
#include "base/command_line.h"
#include "base/test/test_suite.h"
#include "content/public/common/content_switches.h"
#include "ui/gl/gl_surface.h"
#include "ui/gl/test/gl_surface_test_support.h"

int main(int argc, char** argv) {
  base::CommandLine::ForCurrentProcess()->AppendSwitch(
      switches::kSingleProcess);
  gl::GLSurfaceTestSupport::InitializeNoExtensionsOneOff();
  android_webview::DeferredGpuCommandService::GetInstance();
  return base::TestSuite(argc, argv).Run();
}
