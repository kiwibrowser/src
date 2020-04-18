// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/vr/testapp/gl_renderer.h"

#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "chrome/browser/vr/testapp/vr_test_context.h"
#include "ui/gl/gl_bindings.h"
#include "ui/gl/gl_context.h"
#include "ui/gl/gl_surface.h"
#include "ui/gl/init/gl_factory.h"

namespace vr {

namespace {

void OnPresentedFrame(const gfx::PresentationFeedback& feedback) {
  // Do nothing for now.
}

}  // namespace

GlRenderer::GlRenderer(const scoped_refptr<gl::GLSurface>& surface,
                       vr::VrTestContext* vr)
    : surface_(surface), vr_(vr), weak_ptr_factory_(this) {}

GlRenderer::~GlRenderer() {}

bool GlRenderer::Initialize() {
  context_ = gl::init::CreateGLContext(nullptr, surface_.get(),
                                       gl::GLContextAttribs());
  if (!context_.get()) {
    LOG(ERROR) << "Failed to create GL context";
    return false;
  }

  if (!context_->MakeCurrent(surface_.get())) {
    LOG(ERROR) << "Failed to make GL context current";
    return false;
  }

  vr_->OnGlInitialized();

  PostRenderFrameTask(gfx::SwapResult::SWAP_ACK);
  return true;
}

void GlRenderer::RenderFrame() {
  context_->MakeCurrent(surface_.get());

  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  vr_->DrawFrame();
  PostRenderFrameTask(
      surface_->SwapBuffers(base::BindRepeating(&OnPresentedFrame)));
}

void GlRenderer::PostRenderFrameTask(gfx::SwapResult result) {
  base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&GlRenderer::RenderFrame, weak_ptr_factory_.GetWeakPtr()),
      base::TimeDelta::FromSecondsD(1.0 / 60));
}

}  // namespace vr
