// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_INPUT_WIDGET_INPUT_HANDLER_IMPL_H_
#define CONTENT_RENDERER_INPUT_WIDGET_INPUT_HANDLER_IMPL_H_

#include "base/single_thread_task_runner.h"
#include "content/common/input/input_handler.mojom.h"
#include "mojo/public/cpp/bindings/associated_binding.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace content {
class MainThreadEventQueue;
class RenderWidget;
class WidgetInputHandlerManager;

// This class provides an implementation of the mojo WidgetInputHandler
// interface. If threaded compositing is used this thread will live on
// the compositor thread and proxy events to the main thread. This
// is done so that events stay in order relative to other events.
class WidgetInputHandlerImpl : public mojom::WidgetInputHandler {
 public:
  WidgetInputHandlerImpl(
      scoped_refptr<WidgetInputHandlerManager> manager,
      scoped_refptr<base::SingleThreadTaskRunner> main_thread_task_runner,
      scoped_refptr<MainThreadEventQueue> input_event_queue,
      base::WeakPtr<RenderWidget> render_widget);
  ~WidgetInputHandlerImpl() override;

  void SetAssociatedBinding(
      mojom::WidgetInputHandlerAssociatedRequest interface_request);
  void SetBinding(mojom::WidgetInputHandlerRequest interface_request);

  void SetFocus(bool focused) override;
  void MouseCaptureLost() override;
  void SetEditCommandsForNextKeyEvent(
      const std::vector<EditCommand>& commands) override;
  void CursorVisibilityChanged(bool visible) override;
  void ImeSetComposition(const base::string16& text,
                         const std::vector<ui::ImeTextSpan>& ime_text_spans,
                         const gfx::Range& range,
                         int32_t start,
                         int32_t end) override;
  void ImeCommitText(const base::string16& text,
                     const std::vector<ui::ImeTextSpan>& ime_text_spans,
                     const gfx::Range& range,
                     int32_t relative_cursor_position) override;
  void ImeFinishComposingText(bool keep_selection) override;
  void RequestTextInputStateUpdate() override;
  void RequestCompositionUpdates(bool immediate_request,
                                 bool monitor_request) override;
  void DispatchEvent(std::unique_ptr<content::InputEvent>,
                     DispatchEventCallback callback) override;
  void DispatchNonBlockingEvent(std::unique_ptr<content::InputEvent>) override;
  void AttachSynchronousCompositor(
      mojom::SynchronousCompositorControlHostPtr control_host,
      mojom::SynchronousCompositorHostAssociatedPtrInfo host,
      mojom::SynchronousCompositorAssociatedRequest compositor_request)
      override;

 private:
  bool ShouldProxyToMainThread() const;
  void RunOnMainThread(base::OnceClosure closure);
  void Release();

  scoped_refptr<base::SingleThreadTaskRunner> main_thread_task_runner_;
  scoped_refptr<WidgetInputHandlerManager> input_handler_manager_;
  scoped_refptr<MainThreadEventQueue> input_event_queue_;
  base::WeakPtr<RenderWidget> render_widget_;

  mojo::Binding<mojom::WidgetInputHandler> binding_;
  mojo::AssociatedBinding<mojom::WidgetInputHandler> associated_binding_;

  DISALLOW_COPY_AND_ASSIGN(WidgetInputHandlerImpl);
};

}  // namespace content

#endif  // CONTENT_RENDERER_INPUT_WIDGET_INPUT_HANDLER_IMPL_H_
