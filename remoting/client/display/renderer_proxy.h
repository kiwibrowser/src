// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_CLIENT_UI_RENDERER_PROXY_H_
#define REMOTING_CLIENT_UI_RENDERER_PROXY_H_

#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/single_thread_task_runner.h"

namespace remoting {

class GlRenderer;
class QueuedTaskPoster;
class ViewMatrix;

// A class to proxy calls to GlRenderer from one thread to another.
// TODO(yuweih): This should be removed once we have moved Drawables out of
// GlRenderer.
class RendererProxy {
 public:
  // task_runner: The task runner that |renderer_| should be run on.
  RendererProxy(scoped_refptr<base::SingleThreadTaskRunner> task_runner);
  ~RendererProxy();

  // Initialize with the renderer to be proxied.
  void Initialize(base::WeakPtr<GlRenderer> renderer);

  void SetTransformation(const ViewMatrix& transformation);
  void SetCursorPosition(float x, float y);
  void SetCursorVisibility(bool visible);
  void StartInputFeedback(float x, float y, float diameter);

  base::WeakPtr<RendererProxy> GetWeakPtr();

 private:
  // Runs the |task| on the thread of |task_runner_|. All tasks run with
  // |needs_synchronization| set to true inside the same tick will be run on
  // |task_runner_| within the same tick.
  void RunTaskOnProperThread(const base::Closure& task,
                             bool needs_synchronization);

  base::WeakPtr<GlRenderer> renderer_;
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  std::unique_ptr<remoting::QueuedTaskPoster> ui_task_poster_;

  base::WeakPtrFactory<RendererProxy> weak_factory_;

  // RenderStubProxy is neither copyable nor movable.
  RendererProxy(const RendererProxy&) = delete;
  RendererProxy& operator=(const RendererProxy&) = delete;
};

}  // namespace remoting
#endif  // REMOTING_CLIENT_UI_RENDERER_PROXY_H_
