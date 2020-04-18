// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_COMPOSITOR_EXTERNAL_BEGIN_FRAME_CONTROLLER_CLIENT_IMPL_H_
#define CONTENT_BROWSER_COMPOSITOR_EXTERNAL_BEGIN_FRAME_CONTROLLER_CLIENT_IMPL_H_

#include "mojo/public/cpp/bindings/binding.h"
#include "services/viz/privileged/interfaces/compositing/external_begin_frame_controller.mojom.h"

namespace ui {
class Compositor;
}  // namespace ui

namespace content {

// ExternalBeginFrameControllerClient implementation that notifies a
// ui::Compositor about BeginFrame events.
class ExternalBeginFrameControllerClientImpl
    : public viz::mojom::ExternalBeginFrameControllerClient {
 public:
  explicit ExternalBeginFrameControllerClientImpl(ui::Compositor* compositor);
  ~ExternalBeginFrameControllerClientImpl() override;

  viz::mojom::ExternalBeginFrameControllerClientPtr GetBoundPtr();

  viz::mojom::ExternalBeginFrameControllerAssociatedRequest
  GetControllerRequest();

  viz::mojom::ExternalBeginFrameControllerAssociatedPtr& GetController();

 private:
  // viz::ExternalBeginFrameControllerClient implementation.
  void OnNeedsBeginFrames(bool needs_begin_frames) override;
  void OnDisplayDidFinishFrame(const viz::BeginFrameAck& ack) override;

  ui::Compositor* compositor_;

  mojo::Binding<viz::mojom::ExternalBeginFrameControllerClient> binding_;
  viz::mojom::ExternalBeginFrameControllerAssociatedPtr controller_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_COMPOSITOR_GPU_PROCESS_TRANSPORT_FACTORY_H_
