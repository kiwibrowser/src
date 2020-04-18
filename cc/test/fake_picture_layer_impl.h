// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_TEST_FAKE_PICTURE_LAYER_IMPL_H_
#define CC_TEST_FAKE_PICTURE_LAYER_IMPL_H_

#include <stddef.h>

#include <memory>

#include "base/memory/ptr_util.h"
#include "cc/layers/picture_layer_impl.h"
#include "cc/raster/raster_source.h"

namespace cc {

class FakePictureLayerImpl : public PictureLayerImpl {
 public:
  using TileRequirementCheck = bool (PictureLayerTiling::*)(const Tile*) const;

  static std::unique_ptr<FakePictureLayerImpl> Create(LayerTreeImpl* tree_impl,
                                                      int id) {
    Layer::LayerMaskType mask_type = Layer::LayerMaskType::NOT_MASK;
    return base::WrapUnique(new FakePictureLayerImpl(tree_impl, id, mask_type));
  }

  static std::unique_ptr<FakePictureLayerImpl> CreateMask(
      LayerTreeImpl* tree_impl,
      int id) {
    Layer::LayerMaskType mask_type = Layer::LayerMaskType::MULTI_TEXTURE_MASK;
    return base::WrapUnique(new FakePictureLayerImpl(tree_impl, id, mask_type));
  }

  static std::unique_ptr<FakePictureLayerImpl> CreateSingleTextureMask(
      LayerTreeImpl* tree_impl,
      int id) {
    Layer::LayerMaskType mask_type = Layer::LayerMaskType::SINGLE_TEXTURE_MASK;
    return base::WrapUnique(new FakePictureLayerImpl(tree_impl, id, mask_type));
  }

  // Create layer from a raster source that covers the entire layer.
  static std::unique_ptr<FakePictureLayerImpl> CreateWithRasterSource(
      LayerTreeImpl* tree_impl,
      int id,
      scoped_refptr<RasterSource> raster_source) {
    Layer::LayerMaskType mask_type = Layer::LayerMaskType::NOT_MASK;
    return base::WrapUnique(
        new FakePictureLayerImpl(tree_impl, id, raster_source, mask_type));
  }

  // Create layer from a raster source that only covers part of the layer.
  static std::unique_ptr<FakePictureLayerImpl> CreateWithPartialRasterSource(
      LayerTreeImpl* tree_impl,
      int id,
      scoped_refptr<RasterSource> raster_source,
      const gfx::Size& layer_bounds) {
    Layer::LayerMaskType mask_type = Layer::LayerMaskType::NOT_MASK;
    return base::WrapUnique(new FakePictureLayerImpl(
        tree_impl, id, raster_source, mask_type, layer_bounds));
  }

  // Create layer from a raster source that covers the entire layer and is a
  // mask.
  static std::unique_ptr<FakePictureLayerImpl> CreateMaskWithRasterSource(
      LayerTreeImpl* tree_impl,
      int id,
      scoped_refptr<RasterSource> raster_source) {
    Layer::LayerMaskType mask_type = Layer::LayerMaskType::MULTI_TEXTURE_MASK;
    return base::WrapUnique(
        new FakePictureLayerImpl(tree_impl, id, raster_source, mask_type));
  }

  static std::unique_ptr<FakePictureLayerImpl>
  CreateSingleTextureMaskWithRasterSource(
      LayerTreeImpl* tree_impl,
      int id,
      scoped_refptr<RasterSource> raster_source) {
    Layer::LayerMaskType mask_type = Layer::LayerMaskType::SINGLE_TEXTURE_MASK;
    return base::WrapUnique(
        new FakePictureLayerImpl(tree_impl, id, raster_source, mask_type));
  }

  std::unique_ptr<LayerImpl> CreateLayerImpl(LayerTreeImpl* tree_impl) override;
  void PushPropertiesTo(LayerImpl* layer_impl) override;
  void AppendQuads(viz::RenderPass* render_pass,
                   AppendQuadsData* append_quads_data) override;
  gfx::Size CalculateTileSize(const gfx::Size& content_bounds) const override;

  void DidBecomeActive() override;
  size_t did_become_active_call_count() {
    return did_become_active_call_count_;
  }

  bool HasValidTilePriorities() const override;
  void set_has_valid_tile_priorities(bool has_valid_priorities) {
    has_valid_tile_priorities_ = has_valid_priorities;
    use_set_valid_tile_priorities_flag_ = true;
  }

  size_t CountTilesRequired(
      TileRequirementCheck is_tile_required_callback) const;
  size_t CountTilesRequiredForActivation() const;
  size_t CountTilesRequiredForDraw() const;

  using PictureLayerImpl::AddTiling;
  using PictureLayerImpl::CleanUpTilingsOnActiveLayer;
  using PictureLayerImpl::CanHaveTilings;
  using PictureLayerImpl::MinimumContentsScale;
  using PictureLayerImpl::SanityCheckTilingState;
  using PictureLayerImpl::UpdateRasterSource;

  using PictureLayerImpl::UpdateIdealScales;
  using PictureLayerImpl::MaximumTilingContentsScale;

  void AddTilingUntilNextDraw(float scale) {
    last_append_quads_tilings_.push_back(
        AddTiling(gfx::AxisTransform2d(scale, gfx::Vector2dF())));
  }

  float raster_page_scale() const { return raster_page_scale_; }
  void set_raster_page_scale(float scale) { raster_page_scale_ = scale; }

  float ideal_contents_scale() const { return ideal_contents_scale_; }
  float raster_contents_scale() const { return raster_contents_scale_; }

  PictureLayerTiling* HighResTiling() const;
  PictureLayerTiling* LowResTiling() const;
  size_t num_tilings() const { return tilings_->num_tilings(); }

  PictureLayerTilingSet* tilings() { return tilings_.get(); }
  RasterSource* raster_source() { return raster_source_.get(); }
  void SetRasterSourceOnPending(scoped_refptr<RasterSource> raster_source,
                                const Region& invalidation);
  size_t append_quads_count() { return append_quads_count_; }

  const Region& invalidation() const { return invalidation_; }
  void set_invalidation(const Region& region) { invalidation_ = region; }

  gfx::Rect viewport_rect_for_tile_priority_in_content_space() {
    return viewport_rect_for_tile_priority_in_content_space_;
  }

  void set_fixed_tile_size(const gfx::Size& size) { fixed_tile_size_ = size; }

  void CreateAllTiles();
  void SetAllTilesReady();
  void SetAllTilesReadyInTiling(PictureLayerTiling* tiling);
  void SetTileReady(Tile* tile);
  PictureLayerTilingSet* GetTilings() { return tilings_.get(); }

  // Add the given tiling as a "used" tiling during AppendQuads. This ensures
  // that future calls to UpdateTiles don't delete the tiling.
  void MarkAllTilingsUsed() {
    last_append_quads_tilings_.clear();
    for (size_t i = 0; i < tilings_->num_tilings(); ++i)
      last_append_quads_tilings_.push_back(tilings_->tiling_at(i));
  }

  size_t release_resources_count() const { return release_resources_count_; }
  size_t release_tile_resources_count() const {
    return release_tile_resources_count_;
  }

  void ReleaseResources() override;
  void ReleaseTileResources() override;

  bool only_used_low_res_last_append_quads() const {
    return only_used_low_res_last_append_quads_;
  }

 protected:
  FakePictureLayerImpl(LayerTreeImpl* tree_impl,
                       int id,
                       scoped_refptr<RasterSource> raster_source,
                       Layer::LayerMaskType mask_type);
  FakePictureLayerImpl(LayerTreeImpl* tree_impl,
                       int id,
                       scoped_refptr<RasterSource> raster_source,
                       Layer::LayerMaskType mask_type,
                       const gfx::Size& layer_bounds);
  FakePictureLayerImpl(LayerTreeImpl* tree_impl,
                       int id,
                       Layer::LayerMaskType mask_type);

 private:
  gfx::Size fixed_tile_size_;

  size_t append_quads_count_ = 0;
  size_t did_become_active_call_count_ = 0;
  bool has_valid_tile_priorities_ = false;
  bool use_set_valid_tile_priorities_flag_ = false;
  size_t release_resources_count_ = 0;
  size_t release_tile_resources_count_ = 0;
};

}  // namespace cc

#endif  // CC_TEST_FAKE_PICTURE_LAYER_IMPL_H_
