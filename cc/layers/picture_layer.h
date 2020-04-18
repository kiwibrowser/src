// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_LAYERS_PICTURE_LAYER_H_
#define CC_LAYERS_PICTURE_LAYER_H_

#include "base/macros.h"
#include "cc/base/devtools_instrumentation.h"
#include "cc/base/invalidation_region.h"
#include "cc/benchmarks/micro_benchmark_controller.h"
#include "cc/layers/layer.h"

namespace cc {

class ContentLayerClient;
class DisplayItemList;
class RecordingSource;

class CC_EXPORT PictureLayer : public Layer {
 public:
  static scoped_refptr<PictureLayer> Create(ContentLayerClient* client);

  void ClearClient();

  void SetNearestNeighbor(bool nearest_neighbor);
  bool nearest_neighbor() const {
    return picture_layer_inputs_.nearest_neighbor;
  }

  void SetTransformedRasterizationAllowed(bool allowed);
  bool transformed_rasterization_allowed() const {
    return picture_layer_inputs_.transformed_rasterization_allowed;
  }

  // Layer interface.
  std::unique_ptr<LayerImpl> CreateLayerImpl(LayerTreeImpl* tree_impl) override;
  void SetLayerTreeHost(LayerTreeHost* host) override;
  void PushPropertiesTo(LayerImpl* layer) override;
  void SetNeedsDisplayRect(const gfx::Rect& layer_rect) override;
  bool Update() override;
  void SetLayerMaskType(LayerMaskType mask_type) override;
  sk_sp<SkPicture> GetPicture() const override;

  bool HasSlowPaths() const override;
  bool HasNonAAPaint() const override;

  void RunMicroBenchmark(MicroBenchmark* benchmark) override;

  ContentLayerClient* client() { return picture_layer_inputs_.client; }

  RecordingSource* GetRecordingSourceForTesting() {
    return recording_source_.get();
  }

  const DisplayItemList* GetDisplayItemList();

  LayerMaskType mask_type() { return mask_type_; }

 protected:
  // Encapsulates all data, callbacks or interfaces received from the embedder.
  struct PictureLayerInputs {
    PictureLayerInputs();
    ~PictureLayerInputs();

    ContentLayerClient* client = nullptr;
    bool nearest_neighbor = false;
    bool transformed_rasterization_allowed = false;
    gfx::Rect recorded_viewport;
    scoped_refptr<DisplayItemList> display_list;
    size_t painter_reported_memory_usage = 0;
  };

  explicit PictureLayer(ContentLayerClient* client);
  // Allow tests to inject a recording source.
  PictureLayer(ContentLayerClient* client,
               std::unique_ptr<RecordingSource> source);
  ~PictureLayer() override;

  bool HasDrawableContent() const override;

  PictureLayerInputs picture_layer_inputs_;

 private:
  friend class TestSerializationPictureLayer;

  void DropRecordingSourceContentIfInvalid();

  bool ShouldUseTransformedRasterization() const;

  std::unique_ptr<RecordingSource> recording_source_;
  devtools_instrumentation::
      ScopedLayerObjectTracker instrumentation_object_tracker_;

  Region last_updated_invalidation_;

  int update_source_frame_number_;
  LayerMaskType mask_type_;

  DISALLOW_COPY_AND_ASSIGN(PictureLayer);
};

}  // namespace cc

#endif  // CC_LAYERS_PICTURE_LAYER_H_
