// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_COMPOSITING_CONTENT_LAYER_CLIENT_IMPL_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_COMPOSITING_CONTENT_LAYER_CLIENT_IMPL_H_

#include "cc/layers/content_layer_client.h"
#include "cc/layers/layer_client.h"
#include "cc/layers/picture_layer.h"
#include "third_party/blink/renderer/platform/graphics/compositing/composited_layer_raster_invalidator.h"
#include "third_party/blink/renderer/platform/graphics/graphics_layer_client.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/wtf/hash_map.h"
#include "third_party/blink/renderer/platform/wtf/noncopyable.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

class JSONArray;
class JSONObject;
class PaintArtifact;
class PaintChunkSubset;

class PLATFORM_EXPORT ContentLayerClientImpl : public cc::ContentLayerClient,
                                               public cc::LayerClient {
  WTF_MAKE_NONCOPYABLE(ContentLayerClientImpl);
  USING_FAST_MALLOC(ContentLayerClientImpl);

 public:
  ContentLayerClientImpl();
  ~ContentLayerClientImpl() override;

  // cc::ContentLayerClient
  gfx::Rect PaintableRegion() override {
    return gfx::Rect(raster_invalidator_.LayerBounds().size());
  }
  scoped_refptr<cc::DisplayItemList> PaintContentsToDisplayList(
      PaintingControlSetting) override {
    return cc_display_item_list_;
  }
  bool FillsBoundsCompletely() const override { return false; }
  size_t GetApproximateUnsharedMemoryUsage() const override {
    // TODO(jbroman): Actually calculate memory usage.
    return 0;
  }

  // cc::LayerClient
  std::unique_ptr<base::trace_event::TracedValue> TakeDebugInfo(
      cc::Layer*) override;
  void DidChangeScrollbarsHiddenIfOverlay(bool) override {}

  void SetTracksRasterInvalidations(bool should_track) {
    raster_invalidator_.SetTracksRasterInvalidations(should_track);
  }

  bool Matches(const PaintChunk& paint_chunk) const {
    return raster_invalidator_.Matches(paint_chunk);
  }

  struct LayerAsJSONContext {
    LayerAsJSONContext(LayerTreeFlags flags) : flags(flags) {}

    const LayerTreeFlags flags;
    int next_transform_id = 1;
    std::unique_ptr<JSONArray> transforms_json;
    HashMap<const TransformPaintPropertyNode*, int> transform_id_map;
    HashMap<int, int> rendering_context_map;
  };
  std::unique_ptr<JSONObject> LayerAsJSON(LayerAsJSONContext&) const;

  scoped_refptr<cc::PictureLayer> UpdateCcPictureLayer(
      const PaintArtifact&,
      const PaintChunkSubset&,
      const gfx::Rect& layer_bounds,
      const PropertyTreeState&);

 private:
  scoped_refptr<cc::PictureLayer> cc_picture_layer_;
  scoped_refptr<cc::DisplayItemList> cc_display_item_list_;
  CompositedLayerRasterInvalidator raster_invalidator_;
  PropertyTreeState layer_state_;

  String debug_name_;
#if DCHECK_IS_ON()
  std::unique_ptr<JSONArray> paint_chunk_debug_data_;
#endif

  base::WeakPtrFactory<ContentLayerClientImpl> weak_ptr_factory_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_COMPOSITING_CONTENT_LAYER_CLIENT_IMPL_H_
