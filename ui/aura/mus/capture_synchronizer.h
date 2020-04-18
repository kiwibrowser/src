// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_MUS_CAPTURE_SYNCHRONIZER_H_
#define UI_AURA_MUS_CAPTURE_SYNCHRONIZER_H_

#include <stdint.h>

#include "base/macros.h"
#include "ui/aura/aura_export.h"
#include "ui/aura/client/capture_client_observer.h"
#include "ui/aura/window_observer.h"

namespace ui {
namespace mojom {
class WindowTree;
}
}

namespace aura {
class CaptureSynchronizerDelegate;
class WindowMus;

namespace client {
class CaptureClient;
}

// CaptureSynchronizer is resonsible for keeping capture in sync between aura
// and the mus server.
class AURA_EXPORT CaptureSynchronizer : public WindowObserver,
                                        public client::CaptureClientObserver {
 public:
  CaptureSynchronizer(CaptureSynchronizerDelegate* delegate,
                      ui::mojom::WindowTree* window_tree);
  ~CaptureSynchronizer() override;

  WindowMus* capture_window() { return capture_window_; }

  // Called when the server side wants to change capture to |window|.
  void SetCaptureFromServer(WindowMus* window);

  // Called when the |capture_client| has been set or will be unset.
  void AttachToCaptureClient(client::CaptureClient* capture_client);
  void DetachFromCaptureClient(client::CaptureClient* capture_client);

 private:
  // Internal implementation for capture changes. Adds/removes observer as
  // necessary and sets |capture_window_| to |window|.
  void SetCaptureWindow(WindowMus* window);

  // WindowObserver:
  void OnWindowDestroying(Window* window) override;

  // client::CaptureClientObserver:
  void OnCaptureChanged(Window* lost_capture, Window* gained_capture) override;

  CaptureSynchronizerDelegate* delegate_;
  ui::mojom::WindowTree* window_tree_;

  // Window that currently has capture.
  WindowMus* capture_window_ = nullptr;

  // Used when setting capture from the server to avoid setting capture back
  // on the server. If |setting_capture_| is true SetCaptureFromServer() was
  // called and |window_setting_capture_to_| is the window capture is being
  // set on.
  bool setting_capture_ = false;
  WindowMus* window_setting_capture_to_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(CaptureSynchronizer);
};

}  // namespace aura

#endif  // UI_AURA_MUS_CAPTURE_SYNCHRONIZER_H_
