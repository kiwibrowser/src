// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_WS_VIDEO_DETECTOR_IMPL_H_
#define SERVICES_UI_WS_VIDEO_DETECTOR_IMPL_H_

#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/ui/public/interfaces/video_detector.mojom.h"
#include "services/viz/public/interfaces/compositing/video_detector_observer.mojom.h"

namespace viz {
class HostFrameSinkManager;
}

namespace ui {
namespace ws {

// WS-side representation of viz::VideoDetector. Internally forwards observer
// requests to viz.
// TODO(crbug.com/780514): If we restart viz the observers won't receive
// notifications anymore. This needs to be fixed.
class VideoDetectorImpl : public mojom::VideoDetector {
 public:
  explicit VideoDetectorImpl(
      viz::HostFrameSinkManager* host_frame_sink_manager);
  ~VideoDetectorImpl() override;

  void AddBinding(mojom::VideoDetectorRequest request);

  // mojom::VideoDetector implementation.
  void AddObserver(viz::mojom::VideoDetectorObserverPtr observer) override;

 private:
  viz::HostFrameSinkManager* host_frame_sink_manager_;
  mojo::BindingSet<mojom::VideoDetector> binding_set_;

  DISALLOW_COPY_AND_ASSIGN(VideoDetectorImpl);
};

}  // namespace ws
}  // namespace ui

#endif  // SERVICES_UI_WS_VIDEO_DETECTOR_IMPL_H_
