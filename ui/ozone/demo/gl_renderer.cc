// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/demo/gl_renderer.h"

#include "base/location.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/trace_event/trace_event.h"
#include "ui/gl/gl_bindings.h"
#include "ui/gl/gl_context.h"
#include "ui/gl/gl_surface.h"
#include "ui/gl/init/gl_factory.h"

namespace ui {

GlRenderer::GlRenderer(gfx::AcceleratedWidget widget,
                       const scoped_refptr<gl::GLSurface>& surface,
                       const gfx::Size& size)
    : RendererBase(widget, size), surface_(surface), weak_ptr_factory_(this) {}

GlRenderer::~GlRenderer() {}

bool GlRenderer::Initialize() {
  context_ = gl::init::CreateGLContext(nullptr, surface_.get(),
                                       gl::GLContextAttribs());
  if (!context_.get()) {
    LOG(ERROR) << "Failed to create GL context";
    return false;
  }

  surface_->Resize(size_, 1.f, gl::GLSurface::ColorSpace::UNSPECIFIED, true);

  if (!context_->MakeCurrent(surface_.get())) {
    LOG(ERROR) << "Failed to make GL context current";
    return false;
  }

  // Schedule the initial render.
  PostRenderFrameTask(gfx::SwapResult::SWAP_ACK);
  return true;
}

void GlRenderer::RenderFrame() {
  TRACE_EVENT0("ozone", "GlRenderer::RenderFrame");

  float fraction = NextFraction();

  context_->MakeCurrent(surface_.get());

  glViewport(0, 0, size_.width(), size_.height());
  glClearColor(1.f - fraction, fraction, 0.f, 1.f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  if (surface_->SupportsAsyncSwap()) {
    surface_->SwapBuffersAsync(base::Bind(&GlRenderer::PostRenderFrameTask,
                                          weak_ptr_factory_.GetWeakPtr()),
                               base::Bind(&GlRenderer::OnPresentation,
                                          weak_ptr_factory_.GetWeakPtr()));
  } else {
    PostRenderFrameTask(surface_->SwapBuffers(base::Bind(
        &GlRenderer::OnPresentation, weak_ptr_factory_.GetWeakPtr())));
  }
}

void GlRenderer::PostRenderFrameTask(gfx::SwapResult result) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::Bind(&GlRenderer::RenderFrame, weak_ptr_factory_.GetWeakPtr()));
}

void GlRenderer::OnPresentation(const gfx::PresentationFeedback& feedback) {
  DCHECK(surface_->SupportsPresentationCallback());
  LOG_IF(ERROR, feedback.timestamp.is_null()) << "Last frame is discarded!";
}

}  // namespace ui
