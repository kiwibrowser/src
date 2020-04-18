// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/graphics/surface_layer_bridge.h"

#include "base/feature_list.h"
#include "cc/layers/layer.h"
#include "cc/layers/solid_color_layer.h"
#include "cc/layers/surface_layer.h"
#include "components/viz/common/surfaces/surface_id.h"
#include "components/viz/common/surfaces/surface_info.h"
#include "media/base/media_switches.h"
#include "third_party/blink/public/platform/interface_provider.h"
#include "third_party/blink/public/platform/modules/frame_sinks/embedded_frame_sink.mojom-blink.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/web_layer_tree_view.h"
#include "third_party/blink/renderer/platform/mojo/mojo_helper.h"
#include "third_party/blink/renderer/platform/wtf/functional.h"
#include "ui/gfx/geometry/size.h"

namespace blink {

SurfaceLayerBridge::SurfaceLayerBridge(WebLayerTreeView* layer_tree_view,
                                       WebSurfaceLayerBridgeObserver* observer)
    : observer_(observer),
      binding_(this),
      frame_sink_id_(Platform::Current()->GenerateFrameSinkId()),
      parent_frame_sink_id_(layer_tree_view ? layer_tree_view->GetFrameSinkId()
                                            : viz::FrameSinkId()) {
  mojom::blink::EmbeddedFrameSinkProviderPtr provider;
  Platform::Current()->GetInterfaceProvider()->GetInterface(
      mojo::MakeRequest(&provider));
  // TODO(xlai): Ensure OffscreenCanvas commit() is still functional when a
  // frame-less HTML canvas's document is reparenting under another frame.
  // See crbug.com/683172.
  blink::mojom::blink::EmbeddedFrameSinkClientPtr client;
  binding_.Bind(mojo::MakeRequest(&client));
  provider->RegisterEmbeddedFrameSink(parent_frame_sink_id_, frame_sink_id_,
                                      std::move(client));
}

SurfaceLayerBridge::~SurfaceLayerBridge() {
  observer_ = nullptr;
}

void SurfaceLayerBridge::ClearSurfaceId() {
  current_surface_id_ = viz::SurfaceId();
  cc::SurfaceLayer* surface_layer =
      static_cast<cc::SurfaceLayer*>(cc_layer_.get());

  if (!surface_layer)
    return;

  // We reset the Ids if we lose the context_provider (case: GPU process ended)
  // If we destroyed the surface_layer before that point, we need not update
  // the ids.
  surface_layer->SetPrimarySurfaceId(viz::SurfaceId(),
                                     cc::DeadlinePolicy::UseDefaultDeadline());
  surface_layer->SetFallbackSurfaceId(viz::SurfaceId());
}

void SurfaceLayerBridge::CreateSolidColorLayer() {
  cc_layer_ = cc::SolidColorLayer::Create();
  cc_layer_->SetBackgroundColor(SK_ColorTRANSPARENT);

  if (observer_)
    observer_->RegisterContentsLayer(cc_layer_.get());
}

void SurfaceLayerBridge::OnFirstSurfaceActivation(
    const viz::SurfaceInfo& surface_info) {
  if (!current_surface_id_.is_valid() && surface_info.is_valid()) {
    // First time a SurfaceId is received.
    current_surface_id_ = surface_info.id();
    if (cc_layer_) {
      if (observer_)
        observer_->UnregisterContentsLayer(cc_layer_.get());
      cc_layer_->RemoveFromParent();
    }

    scoped_refptr<cc::SurfaceLayer> surface_layer = cc::SurfaceLayer::Create();
    surface_layer->SetPrimarySurfaceId(
        surface_info.id(), cc::DeadlinePolicy::UseDefaultDeadline());
    surface_layer->SetFallbackSurfaceId(surface_info.id());
    surface_layer->SetStretchContentToFillBounds(true);
    surface_layer->SetIsDrawable(true);
    cc_layer_ = surface_layer;

    if (observer_)
      observer_->RegisterContentsLayer(cc_layer_.get());
  } else if (current_surface_id_ != surface_info.id()) {
    // A different SurfaceId is received, prompting change to existing
    // SurfaceLayer.
    current_surface_id_ = surface_info.id();
    cc::SurfaceLayer* surface_layer =
        static_cast<cc::SurfaceLayer*>(cc_layer_.get());
    surface_layer->SetPrimarySurfaceId(
        surface_info.id(), cc::DeadlinePolicy::UseDefaultDeadline());
    surface_layer->SetFallbackSurfaceId(surface_info.id());
  }

  if (observer_) {
    observer_->OnWebLayerUpdated();
    observer_->OnSurfaceIdUpdated(surface_info.id());
  }

  cc_layer_->SetBounds(surface_info.size_in_pixels());
}

}  // namespace blink
