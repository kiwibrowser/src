// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/client/display/renderer_proxy.h"

#include "base/bind.h"
#include "base/logging.h"
#include "remoting/client/display/gl_renderer.h"
#include "remoting/client/queued_task_poster.h"
#include "remoting/client/ui/view_matrix.h"

namespace remoting {

RendererProxy::RendererProxy(
    scoped_refptr<base::SingleThreadTaskRunner> task_runner)
    : task_runner_(task_runner),
      ui_task_poster_(new remoting::QueuedTaskPoster(task_runner_)),
      weak_factory_(this) {}

RendererProxy::~RendererProxy() = default;

void RendererProxy::Initialize(base::WeakPtr<GlRenderer> renderer) {
  renderer_ = renderer;
}

void RendererProxy::SetTransformation(const ViewMatrix& transformation) {
  // Viewport and cursor movements need to be synchronized into the same frame.
  RunTaskOnProperThread(base::Bind(&GlRenderer::OnPixelTransformationChanged,
                                   renderer_, transformation.ToMatrixArray()),
                        true);
}

void RendererProxy::SetCursorPosition(float x, float y) {
  RunTaskOnProperThread(base::Bind(&GlRenderer::OnCursorMoved, renderer_, x, y),
                        true);
}

void RendererProxy::SetCursorVisibility(bool visible) {
  // Cursor visibility and position should be synchronized.
  RunTaskOnProperThread(
      base::Bind(&GlRenderer::OnCursorVisibilityChanged, renderer_, visible),
      true);
}

void RendererProxy::StartInputFeedback(float x, float y, float diameter) {
  RunTaskOnProperThread(
      base::Bind(&GlRenderer::OnCursorInputFeedback, renderer_, x, y, diameter),
      false);
}

base::WeakPtr<RendererProxy> RendererProxy::GetWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

void RendererProxy::RunTaskOnProperThread(const base::Closure& task,
                                          bool needs_synchronization) {
  if (task_runner_->BelongsToCurrentThread()) {
    task.Run();
    return;
  }

  if (needs_synchronization) {
    ui_task_poster_->AddTask(task);
    return;
  }

  task_runner_->PostTask(FROM_HERE, task);
}

}  // namespace remoting
