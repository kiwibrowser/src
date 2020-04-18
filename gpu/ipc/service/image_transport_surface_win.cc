// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/ipc/service/image_transport_surface.h"

#include <memory>

#include "base/win/windows_version.h"
#include "components/viz/common/features.h"
#include "gpu/command_buffer/service/gpu_preferences.h"
#include "gpu/ipc/service/direct_composition_surface_win.h"
#include "gpu/ipc/service/gpu_vsync_provider_win.h"
#include "gpu/ipc/service/pass_through_image_transport_surface.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/gl/gl_bindings.h"
#include "ui/gl/gl_implementation.h"
#include "ui/gl/gl_surface_egl.h"
#include "ui/gl/gl_switches.h"
#include "ui/gl/init/gl_factory.h"
#include "ui/gl/vsync_provider_win.h"

namespace gpu {

namespace {
bool IsGpuVSyncSignalSupported() {
  // TODO(crbug.com/787814): D3DVsync needs to be rewritten to work with
  // OOP-D. It's no longer requires any IPC, just an ExternalBeginFrameSource,
  // so it should be much simpler.
  if (base::FeatureList::IsEnabled(features::kVizDisplayCompositor))
    return false;

  // TODO(stanisc): http://crbug.com/467617 Limit to Windows 8.1+ for now
  // because of locking issue caused by waiting for VSync on Win7 and Win 8.0.
  return base::win::GetVersion() >= base::win::VERSION_WIN8_1 &&
         base::FeatureList::IsEnabled(features::kD3DVsync);
}

}  // namespace

// static
scoped_refptr<gl::GLSurface> ImageTransportSurface::CreateNativeSurface(
    base::WeakPtr<ImageTransportSurfaceDelegate> delegate,
    SurfaceHandle surface_handle,
    gl::GLSurfaceFormat format) {
  DCHECK_NE(surface_handle, kNullSurfaceHandle);

  scoped_refptr<gl::GLSurface> surface;
  bool override_vsync_for_multi_window_swap = false;
  if (gl::GetGLImplementation() == gl::kGLImplementationEGLGLES2) {
    std::unique_ptr<gfx::VSyncProvider> vsync_provider;

    if (IsGpuVSyncSignalSupported())
      vsync_provider.reset(new GpuVSyncProviderWin(delegate, surface_handle));
    else
      vsync_provider.reset(new gl::VSyncProviderWin(surface_handle));

    if (gl::GLSurfaceEGL::IsDirectCompositionSupported()) {
      scoped_refptr<DirectCompositionSurfaceWin> egl_surface =
          base::MakeRefCounted<DirectCompositionSurfaceWin>(
              std::move(vsync_provider), delegate, surface_handle);
      if (!egl_surface->Initialize())
        return nullptr;
      surface = egl_surface;
    } else {
      surface = gl::InitializeGLSurface(
          base::MakeRefCounted<gl::NativeViewGLSurfaceEGL>(
              surface_handle, std::move(vsync_provider)));
      if (!surface)
        return nullptr;
      // This is unnecessary with DirectComposition because that doesn't block
      // swaps, but instead blocks the first draw into a surface during the next
      // frame.
      override_vsync_for_multi_window_swap = true;
    }
  } else {
    surface = gl::init::CreateViewGLSurface(surface_handle);
    if (!surface)
      return nullptr;
  }

  return scoped_refptr<gl::GLSurface>(new PassThroughImageTransportSurface(
      delegate, surface.get(), override_vsync_for_multi_window_swap));
}

}  // namespace gpu
