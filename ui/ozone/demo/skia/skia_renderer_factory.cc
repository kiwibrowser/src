// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/demo/skia/skia_renderer_factory.h"

#include <memory>

#include "base/command_line.h"
#include "ui/gl/gl_surface.h"
#include "ui/gl/init/gl_factory.h"
#include "ui/ozone/demo/skia/skia_gl_renderer.h"
#include "ui/ozone/demo/skia/skia_surfaceless_gl_renderer.h"
#include "ui/ozone/public/ozone_platform.h"

namespace ui {
namespace {

const char kDisableSurfaceless[] = "disable-surfaceless";

scoped_refptr<gl::GLSurface> CreateGLSurface(gfx::AcceleratedWidget widget) {
  scoped_refptr<gl::GLSurface> surface;
  if (!base::CommandLine::ForCurrentProcess()->HasSwitch(kDisableSurfaceless))
    surface = gl::init::CreateSurfacelessViewGLSurface(widget);
  if (!surface)
    surface = gl::init::CreateViewGLSurface(widget);
  return surface;
}

}  // namespace

SkiaRendererFactory::SkiaRendererFactory() {}

SkiaRendererFactory::~SkiaRendererFactory() {}

bool SkiaRendererFactory::Initialize() {
  OzonePlatform::InitParams params;
  params.single_process = true;
  OzonePlatform::InitializeForGPU(params);
  OzonePlatform::GetInstance()->AfterSandboxEntry();

  if (!gl::init::InitializeGLOneOff() ||
      !gpu_helper_.Initialize(base::ThreadTaskRunnerHandle::Get())) {
    LOG(FATAL) << "Failed to initialize GL";
  }

  return true;
}

std::unique_ptr<Renderer> SkiaRendererFactory::CreateRenderer(
    gfx::AcceleratedWidget widget,
    const gfx::Size& size) {
  scoped_refptr<gl::GLSurface> surface = CreateGLSurface(widget);
  if (!surface)
    LOG(FATAL) << "Failed to create GL surface";
  if (surface->IsSurfaceless()) {
    return std::make_unique<SurfacelessSkiaGlRenderer>(widget, surface, size);
  }
  return std::make_unique<SkiaGlRenderer>(widget, surface, size);
}

}  // namespace ui
