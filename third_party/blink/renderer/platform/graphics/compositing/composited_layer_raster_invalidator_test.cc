// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/graphics/compositing/composited_layer_raster_invalidator.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_artifact.h"
#include "third_party/blink/renderer/platform/testing/fake_display_item_client.h"
#include "third_party/blink/renderer/platform/testing/paint_property_test_helpers.h"
#include "third_party/blink/renderer/platform/testing/runtime_enabled_features_test_helpers.h"
#include "third_party/blink/renderer/platform/wtf/dtoa/utils.h"

namespace blink {

static const IntRect kDefaultLayerBounds(-9999, -7777, 18888, 16666);

class CompositedLayerRasterInvalidatorTest
    : public testing::Test,
      private ScopedSlimmingPaintV2ForTest {
 public:
  CompositedLayerRasterInvalidatorTest() : ScopedSlimmingPaintV2ForTest(true) {}

  static PropertyTreeState DefaultPropertyTreeState() {
    return PropertyTreeState::Root();
  }

  CompositedLayerRasterInvalidatorTest& Chunk(int type) {
    DEFINE_STATIC_LOCAL(FakeDisplayItemClient, fake_client, ());
    fake_client.ClearIsJustCreated();
    // The enum arithmetics and magic numbers are to produce different values
    // of paint chunk and raster invalidation properties.
    PaintChunk::Id id(fake_client, static_cast<DisplayItem::Type>(
                                       DisplayItem::kDrawingFirst + type));
    data_.chunks.emplace_back(0, 0, id, DefaultPropertyTreeState());
    data_.chunks.back().bounds =
        FloatRect(type * 110, type * 220, type * 220 + 200, type * 110 + 200);
    return *this;
  }

  CompositedLayerRasterInvalidatorTest& Properties(
      const TransformPaintPropertyNode* t,
      const ClipPaintPropertyNode* c = ClipPaintPropertyNode::Root(),
      const EffectPaintPropertyNode* e = EffectPaintPropertyNode::Root()) {
    auto& state = data_.chunks.back().properties;
    state.SetTransform(t);
    state.SetClip(c);
    state.SetEffect(e);
    return *this;
  }

  CompositedLayerRasterInvalidatorTest& Properties(
      const RefCountedPropertyTreeState& state) {
    data_.chunks.back().properties = state;
    return *this;
  }

  CompositedLayerRasterInvalidatorTest& Uncacheable() {
    data_.chunks.back().is_cacheable = false;
    return *this;
  }

  CompositedLayerRasterInvalidatorTest& Bounds(const FloatRect& bounds) {
    data_.chunks.back().bounds = bounds;
    return *this;
  }

  CompositedLayerRasterInvalidatorTest& RasterInvalidationCount(int count) {
    size_t size = data_.chunks.size();
    DCHECK_GT(size, 0u);
    data_.raster_invalidation_rects.resize(size);
    data_.raster_invalidation_trackings.resize(size);
    int index = static_cast<int>(size - 1);
    for (int i = 0; i < count; ++i) {
      IntRect rect(index * 11, index * 22, index * 22 + 100 + i,
                   index * 11 + 100 + i);
      data_.raster_invalidation_rects.back().push_back(FloatRect(rect));
      data_.raster_invalidation_trackings.back().push_back(
          RasterInvalidationInfo{
              &data_.chunks.back().id.client, "Test", rect,
              static_cast<PaintInvalidationReason>(
                  static_cast<int>(PaintInvalidationReason::kFull) + index +
                  i)});
    }
    return *this;
  }

  PaintArtifact Build() {
    return PaintArtifact(DisplayItemList(0), std::move(data_));
  }

  static const Vector<RasterInvalidationInfo> TrackedRasterInvalidations(
      CompositedLayerRasterInvalidator& invalidator) {
    DCHECK(invalidator.GetTracking());
    return invalidator.GetTracking()->Invalidations();
  }

  static IntRect ChunkRectToLayer(const FloatRect& rect,
                                  const IntPoint& chunk_offset_from_layer) {
    FloatRect r = rect;
    r.MoveBy(chunk_offset_from_layer);
    return EnclosingIntRect(r);
  }

  CompositedLayerRasterInvalidator::RasterInvalidationFunction
      kNoopRasterInvalidation = [this](const IntRect& rect) {};

  PaintChunksAndRasterInvalidations data_;
};

#define EXPECT_DISPLAY_ITEM_INVALIDATIONS(invalidations, invalidation_index, \
                                          artifact, chunk_index)             \
  do {                                                                       \
    const auto& chunk = (artifact).PaintChunks()[chunk_index];               \
    const auto* rects = (artifact).GetRasterInvalidationRects(chunk);        \
    ASSERT_TRUE(rects);                                                      \
    const auto* tracking = (artifact).GetRasterInvalidationTracking(chunk);  \
    ASSERT_TRUE(tracking);                                                   \
    for (size_t i = 0; i < rects->size(); ++i) {                             \
      SCOPED_TRACE(invalidation_index + i);                                  \
      const auto& info = (invalidations)[invalidation_index + i];            \
      EXPECT_EQ(                                                             \
          ChunkRectToLayer((*rects)[i], -kDefaultLayerBounds.Location()),    \
          info.rect);                                                        \
      EXPECT_EQ(&chunk.id.client, info.client);                              \
      EXPECT_EQ((*tracking)[i].reason, info.reason);                         \
    }                                                                        \
  } while (false)

#define EXPECT_CHUNK_INVALIDATION_WITH_LAYER_OFFSET(                      \
    invalidations, index, chunk, expected_reason, layer_offset)           \
  do {                                                                    \
    const auto& info = (invalidations)[index];                            \
    EXPECT_EQ(ChunkRectToLayer((chunk).bounds, layer_offset), info.rect); \
    EXPECT_EQ(&(chunk).id.client, info.client);                           \
    EXPECT_EQ(expected_reason, info.reason);                              \
  } while (false)

#define EXPECT_CHUNK_INVALIDATION(invalidations, index, chunk, reason) \
  EXPECT_CHUNK_INVALIDATION_WITH_LAYER_OFFSET(                         \
      invalidations, index, chunk, reason, -kDefaultLayerBounds.Location())

#define EXPECT_INCREMENTAL_INVALIDATION(invalidations, index, chunk,         \
                                        chunk_rect)                          \
  do {                                                                       \
    const auto& info = (invalidations)[index];                               \
    EXPECT_EQ(ChunkRectToLayer(chunk_rect, -kDefaultLayerBounds.Location()), \
              info.rect);                                                    \
    EXPECT_EQ(&(chunk).id.client, info.client);                              \
    EXPECT_EQ(PaintInvalidationReason::kIncremental, info.reason);           \
  } while (false)

TEST_F(CompositedLayerRasterInvalidatorTest, LayerBounds) {
  CompositedLayerRasterInvalidator invalidator(kNoopRasterInvalidation);
  invalidator.SetTracksRasterInvalidations(true);
  auto artifact = Chunk(0).Build();

  invalidator.Generate(artifact, kDefaultLayerBounds,
                       DefaultPropertyTreeState());
  // No raster invalidations needed for a new layer.
  EXPECT_TRUE(TrackedRasterInvalidations(invalidator).IsEmpty());

  invalidator.Generate(artifact, kDefaultLayerBounds,
                       DefaultPropertyTreeState());
  // No raster invalidations needed if layer origin doesn't change.
  EXPECT_TRUE(TrackedRasterInvalidations(invalidator).IsEmpty());

  auto new_layer_bounds = kDefaultLayerBounds;
  new_layer_bounds.Move(66, 77);
  invalidator.Generate(artifact, new_layer_bounds, DefaultPropertyTreeState());
  // Change of layer origin causes change of chunk0's transform to layer.
  const auto& invalidations = TrackedRasterInvalidations(invalidator);
  ASSERT_EQ(2u, invalidations.size());
  EXPECT_CHUNK_INVALIDATION(invalidations, 0, artifact.PaintChunks()[0],
                            PaintInvalidationReason::kPaintProperty);
  EXPECT_CHUNK_INVALIDATION_WITH_LAYER_OFFSET(
      invalidations, 1, artifact.PaintChunks()[0],
      PaintInvalidationReason::kPaintProperty, -new_layer_bounds.Location());
}

TEST_F(CompositedLayerRasterInvalidatorTest, ReorderChunks) {
  CompositedLayerRasterInvalidator invalidator(kNoopRasterInvalidation);
  auto artifact = Chunk(0).Chunk(1).Chunk(2).Build();
  invalidator.SetTracksRasterInvalidations(true);
  invalidator.Generate(artifact, kDefaultLayerBounds,
                       DefaultPropertyTreeState());
  EXPECT_TRUE(TrackedRasterInvalidations(invalidator).IsEmpty());

  // Swap chunk 1 and 2. All chunks have their own local raster invalidations.
  auto new_artifact = Chunk(0)
                          .RasterInvalidationCount(2)
                          .Chunk(2)
                          .RasterInvalidationCount(4)
                          .Chunk(1)
                          .RasterInvalidationCount(3)
                          .Bounds(FloatRect(11, 22, 33, 44))
                          .Build();
  invalidator.Generate(new_artifact, kDefaultLayerBounds,
                       DefaultPropertyTreeState());
  const auto& invalidations = TrackedRasterInvalidations(invalidator);
  ASSERT_EQ(8u, invalidations.size());
  // The first chunk should always match because otherwise we won't reuse the
  // CompositedLayerRasterInvalidator (which is according to the first chunk's
  // id). For matched chunk, we issue raster invalidations if any found by
  // PaintController.
  EXPECT_DISPLAY_ITEM_INVALIDATIONS(invalidations, 0, new_artifact, 0);
  EXPECT_DISPLAY_ITEM_INVALIDATIONS(invalidations, 2, new_artifact, 1);
  // Invalidated new chunk 2's old (as chunks[1]) and new (as new_artifact[2])
  // bounds.
  EXPECT_CHUNK_INVALIDATION(invalidations, 6, artifact.PaintChunks()[1],
                            PaintInvalidationReason::kChunkReordered);
  EXPECT_CHUNK_INVALIDATION(invalidations, 7, new_artifact.PaintChunks()[2],
                            PaintInvalidationReason::kChunkReordered);
}

TEST_F(CompositedLayerRasterInvalidatorTest, ReorderChunkSubsequences) {
  CompositedLayerRasterInvalidator invalidator(kNoopRasterInvalidation);
  auto artifact = Chunk(0).Chunk(1).Chunk(2).Chunk(3).Chunk(4).Build();
  invalidator.SetTracksRasterInvalidations(true);
  invalidator.Generate(artifact, kDefaultLayerBounds,
                       DefaultPropertyTreeState());
  EXPECT_TRUE(TrackedRasterInvalidations(invalidator).IsEmpty());

  // Swap chunk (1,2) and (3,4). All chunks have their own local raster
  // invalidations.
  auto new_artifact = Chunk(0)
                          .RasterInvalidationCount(2)
                          .Chunk(3)
                          .RasterInvalidationCount(3)
                          .Chunk(4)
                          .RasterInvalidationCount(4)
                          .Chunk(1)
                          .RasterInvalidationCount(1)
                          .Bounds(FloatRect(11, 22, 33, 44))
                          .Chunk(2)
                          .RasterInvalidationCount(2)
                          .Build();
  invalidator.Generate(new_artifact, kDefaultLayerBounds,
                       DefaultPropertyTreeState());
  const auto& invalidations = TrackedRasterInvalidations(invalidator);
  ASSERT_EQ(12u, invalidations.size());
  // The first chunk should always match because otherwise we won't reuse the
  // CompositedLayerRasterInvalidator (which is according to the first chunk's
  // id). For matched chunk, we issue raster invalidations if any found by
  // PaintController.
  EXPECT_DISPLAY_ITEM_INVALIDATIONS(invalidations, 0, new_artifact, 0);
  EXPECT_DISPLAY_ITEM_INVALIDATIONS(invalidations, 2, new_artifact, 1);
  EXPECT_DISPLAY_ITEM_INVALIDATIONS(invalidations, 5, new_artifact, 2);
  // Invalidated new chunk 3's old (as chunks[1]) and new (as new_artifact[3])
  // bounds.
  EXPECT_CHUNK_INVALIDATION(invalidations, 9, artifact.PaintChunks()[1],
                            PaintInvalidationReason::kChunkReordered);
  EXPECT_CHUNK_INVALIDATION(invalidations, 10, new_artifact.PaintChunks()[3],
                            PaintInvalidationReason::kChunkReordered);
  // Invalidated new chunk 4's new bounds. Didn't invalidate old bounds because
  // it's the same as the new bounds.
  EXPECT_CHUNK_INVALIDATION(invalidations, 11, new_artifact.PaintChunks()[4],
                            PaintInvalidationReason::kChunkReordered);
}

TEST_F(CompositedLayerRasterInvalidatorTest, AppearAndDisappear) {
  CompositedLayerRasterInvalidator invalidator(kNoopRasterInvalidation);
  auto artifact = Chunk(0).Chunk(1).Chunk(2).Build();
  invalidator.SetTracksRasterInvalidations(true);
  invalidator.Generate(artifact, kDefaultLayerBounds,
                       DefaultPropertyTreeState());
  EXPECT_TRUE(TrackedRasterInvalidations(invalidator).IsEmpty());

  // Chunk 1 and 2 disappeared, 3 and 4 appeared. All chunks have their own
  // local raster invalidations.
  auto new_artifact = Chunk(0)
                          .RasterInvalidationCount(2)
                          .Chunk(3)
                          .RasterInvalidationCount(3)
                          .Chunk(4)
                          .RasterInvalidationCount(3)
                          .Build();
  invalidator.Generate(new_artifact, kDefaultLayerBounds,
                       DefaultPropertyTreeState());
  const auto& invalidations = TrackedRasterInvalidations(invalidator);
  ASSERT_EQ(6u, invalidations.size());
  EXPECT_DISPLAY_ITEM_INVALIDATIONS(invalidations, 0, new_artifact, 0);
  EXPECT_CHUNK_INVALIDATION(invalidations, 2, new_artifact.PaintChunks()[1],
                            PaintInvalidationReason::kChunkAppeared);
  EXPECT_CHUNK_INVALIDATION(invalidations, 3, new_artifact.PaintChunks()[2],
                            PaintInvalidationReason::kChunkAppeared);
  EXPECT_CHUNK_INVALIDATION(invalidations, 4, artifact.PaintChunks()[1],
                            PaintInvalidationReason::kChunkDisappeared);
  EXPECT_CHUNK_INVALIDATION(invalidations, 5, artifact.PaintChunks()[2],
                            PaintInvalidationReason::kChunkDisappeared);
}

TEST_F(CompositedLayerRasterInvalidatorTest, AppearAtEnd) {
  CompositedLayerRasterInvalidator invalidator(kNoopRasterInvalidation);
  auto artifact = Chunk(0).Build();
  invalidator.SetTracksRasterInvalidations(true);
  invalidator.Generate(artifact, kDefaultLayerBounds,
                       DefaultPropertyTreeState());
  EXPECT_TRUE(TrackedRasterInvalidations(invalidator).IsEmpty());

  auto new_artifact = Chunk(0)
                          .RasterInvalidationCount(2)
                          .Chunk(1)
                          .RasterInvalidationCount(3)
                          .Chunk(2)
                          .RasterInvalidationCount(3)
                          .Build();
  invalidator.Generate(new_artifact, kDefaultLayerBounds,
                       DefaultPropertyTreeState());
  const auto& invalidations = TrackedRasterInvalidations(invalidator);
  ASSERT_EQ(4u, invalidations.size());
  EXPECT_DISPLAY_ITEM_INVALIDATIONS(invalidations, 0, new_artifact, 0);
  EXPECT_CHUNK_INVALIDATION(invalidations, 2, new_artifact.PaintChunks()[1],
                            PaintInvalidationReason::kChunkAppeared);
  EXPECT_CHUNK_INVALIDATION(invalidations, 3, new_artifact.PaintChunks()[2],
                            PaintInvalidationReason::kChunkAppeared);
}

TEST_F(CompositedLayerRasterInvalidatorTest, UncacheableChunks) {
  CompositedLayerRasterInvalidator invalidator(kNoopRasterInvalidation);
  auto artifact = Chunk(0).Chunk(1).Uncacheable().Chunk(2).Build();

  invalidator.SetTracksRasterInvalidations(true);
  invalidator.Generate(artifact, kDefaultLayerBounds,
                       DefaultPropertyTreeState());
  EXPECT_TRUE(TrackedRasterInvalidations(invalidator).IsEmpty());

  auto new_artifact = Chunk(0)
                          .RasterInvalidationCount(2)
                          .Chunk(2)
                          .RasterInvalidationCount(3)
                          .Chunk(1)
                          .RasterInvalidationCount(3)
                          .Uncacheable()
                          .Build();
  invalidator.Generate(new_artifact, kDefaultLayerBounds,
                       DefaultPropertyTreeState());
  const auto& invalidations = TrackedRasterInvalidations(invalidator);
  ASSERT_EQ(7u, invalidations.size());
  EXPECT_DISPLAY_ITEM_INVALIDATIONS(invalidations, 0, new_artifact, 0);
  EXPECT_DISPLAY_ITEM_INVALIDATIONS(invalidations, 2, new_artifact, 1);
  EXPECT_CHUNK_INVALIDATION(invalidations, 5, new_artifact.PaintChunks()[2],
                            PaintInvalidationReason::kChunkUncacheable);
  EXPECT_CHUNK_INVALIDATION(invalidations, 6, artifact.PaintChunks()[1],
                            PaintInvalidationReason::kChunkUncacheable);
}

// Tests the path based on ClipPaintPropertyNode::Changed().
TEST_F(CompositedLayerRasterInvalidatorTest, ClipPropertyChangeRounded) {
  CompositedLayerRasterInvalidator invalidator(kNoopRasterInvalidation);
  FloatRoundedRect::Radii radii(FloatSize(1, 2), FloatSize(2, 3),
                                FloatSize(3, 4), FloatSize(4, 5));
  FloatRoundedRect clip_rect(FloatRect(-1000, -1000, 2000, 2000), radii);
  scoped_refptr<ClipPaintPropertyNode> clip0 =
      CreateClip(ClipPaintPropertyNode::Root(),
                 TransformPaintPropertyNode::Root(), clip_rect);
  scoped_refptr<ClipPaintPropertyNode> clip2 =
      CreateClip(clip0, TransformPaintPropertyNode::Root(), clip_rect);

  PropertyTreeState layer_state(TransformPaintPropertyNode::Root(), clip0.get(),
                                EffectPaintPropertyNode::Root());
  auto artifact =
      Chunk(0)
          .Properties(layer_state)
          .Chunk(1)
          .Properties(layer_state)
          .Chunk(2)
          .Properties(TransformPaintPropertyNode::Root(), clip2.get())
          .Build();

  GeometryMapperClipCache::ClearCache();
  invalidator.SetTracksRasterInvalidations(true);
  invalidator.Generate(artifact, kDefaultLayerBounds, layer_state);
  EXPECT_TRUE(TrackedRasterInvalidations(invalidator).IsEmpty());

  // Change both clip0 and clip2.
  auto new_artifact = Chunk(0)
                          .Properties(artifact.PaintChunks()[0].properties)
                          .Chunk(1)
                          .Properties(artifact.PaintChunks()[1].properties)
                          .Chunk(2)
                          .Properties(artifact.PaintChunks()[2].properties)
                          .Build();
  FloatRoundedRect new_clip_rect(FloatRect(-2000, -2000, 4000, 4000), radii);
  clip0->Update(clip0->Parent(),
                ClipPaintPropertyNode::State{clip0->LocalTransformSpace(),
                                             new_clip_rect});
  clip2->Update(clip2->Parent(),
                ClipPaintPropertyNode::State{clip2->LocalTransformSpace(),
                                             new_clip_rect});

  GeometryMapperClipCache::ClearCache();
  invalidator.Generate(new_artifact, kDefaultLayerBounds, layer_state);
  const auto& invalidations = TrackedRasterInvalidations(invalidator);
  ASSERT_EQ(1u, invalidations.size());
  // Property change in the layer state should not trigger raster invalidation.
  // |clip2| change should trigger raster invalidation.
  EXPECT_CHUNK_INVALIDATION(invalidations, 0, new_artifact.PaintChunks()[2],
                            PaintInvalidationReason::kPaintProperty);
  invalidator.SetTracksRasterInvalidations(false);
  clip2->ClearChangedToRoot();

  // Change chunk1's properties to use a different property tree state.
  auto new_artifact1 = Chunk(0)
                           .Properties(artifact.PaintChunks()[0].properties)
                           .Chunk(1)
                           .Properties(artifact.PaintChunks()[2].properties)
                           .Chunk(2)
                           .Properties(artifact.PaintChunks()[2].properties)
                           .Build();

  GeometryMapperClipCache::ClearCache();
  invalidator.SetTracksRasterInvalidations(true);
  invalidator.Generate(new_artifact1, kDefaultLayerBounds, layer_state);
  const auto& invalidations1 = TrackedRasterInvalidations(invalidator);
  ASSERT_EQ(1u, invalidations1.size());
  EXPECT_CHUNK_INVALIDATION(invalidations1, 0, new_artifact1.PaintChunks()[1],
                            PaintInvalidationReason::kPaintProperty);
  invalidator.SetTracksRasterInvalidations(false);
}

// Tests the path detecting change of PaintChunkInfo::chunk_to_layer_clip.
TEST_F(CompositedLayerRasterInvalidatorTest, ClipPropertyChangeSimple) {
  CompositedLayerRasterInvalidator invalidator(kNoopRasterInvalidation);
  FloatRoundedRect clip_rect(-1000, -1000, 2000, 2000);
  scoped_refptr<ClipPaintPropertyNode> clip0 =
      CreateClip(ClipPaintPropertyNode::Root(),
                 TransformPaintPropertyNode::Root(), clip_rect);
  scoped_refptr<ClipPaintPropertyNode> clip1 =
      CreateClip(clip0, TransformPaintPropertyNode::Root(), clip_rect);

  PropertyTreeState layer_state = PropertyTreeState::Root();
  auto artifact =
      Chunk(0)
          .Properties(TransformPaintPropertyNode::Root(), clip0.get())
          .Bounds(clip_rect.Rect())
          .Chunk(1)
          .Properties(TransformPaintPropertyNode::Root(), clip1.get())
          .Bounds(clip_rect.Rect())
          .Build();

  GeometryMapperClipCache::ClearCache();
  invalidator.SetTracksRasterInvalidations(true);
  invalidator.Generate(artifact, kDefaultLayerBounds, layer_state);
  EXPECT_TRUE(TrackedRasterInvalidations(invalidator).IsEmpty());

  // Change clip1 to bigger, which is still bound by clip0, resulting no actual
  // visual change.
  FloatRoundedRect new_clip_rect1(-2000, -2000, 4000, 4000);
  clip1->Update(clip1->Parent(),
                ClipPaintPropertyNode::State{clip1->LocalTransformSpace(),
                                             new_clip_rect1});
  auto new_artifact1 = Chunk(0)
                           .Properties(artifact.PaintChunks()[0].properties)
                           .Bounds(artifact.PaintChunks()[0].bounds)
                           .Chunk(1)
                           .Properties(artifact.PaintChunks()[1].properties)
                           .Bounds(artifact.PaintChunks()[1].bounds)
                           .Build();

  GeometryMapperClipCache::ClearCache();
  invalidator.Generate(new_artifact1, kDefaultLayerBounds, layer_state);
  EXPECT_TRUE(TrackedRasterInvalidations(invalidator).IsEmpty());
  clip1->ClearChangedToRoot();

  // Change clip1 to smaller.
  FloatRoundedRect new_clip_rect2(-500, -500, 1000, 1000);
  clip1->Update(clip1->Parent(),
                ClipPaintPropertyNode::State{clip1->LocalTransformSpace(),
                                             new_clip_rect2});
  auto new_artifact2 = Chunk(0)
                           .Properties(artifact.PaintChunks()[0].properties)
                           .Bounds(artifact.PaintChunks()[0].bounds)
                           .Chunk(1)
                           .Properties(artifact.PaintChunks()[1].properties)
                           .Bounds(new_clip_rect2.Rect())
                           .Build();

  GeometryMapperClipCache::ClearCache();
  invalidator.Generate(new_artifact2, kDefaultLayerBounds, layer_state);
  const auto& invalidations = TrackedRasterInvalidations(invalidator);
  ASSERT_EQ(4u, invalidations.size());
  // |clip1| change should trigger incremental raster invalidation.
  EXPECT_INCREMENTAL_INVALIDATION(invalidations, 0,
                                  new_artifact2.PaintChunks()[1],
                                  IntRect(-1000, -1000, 2000, 500));
  EXPECT_INCREMENTAL_INVALIDATION(invalidations, 1,
                                  new_artifact2.PaintChunks()[1],
                                  IntRect(-1000, -500, 500, 1000));
  EXPECT_INCREMENTAL_INVALIDATION(invalidations, 2,
                                  new_artifact2.PaintChunks()[1],
                                  IntRect(500, -500, 500, 1000));
  EXPECT_INCREMENTAL_INVALIDATION(invalidations, 3,
                                  new_artifact2.PaintChunks()[1],
                                  IntRect(-1000, 500, 2000, 500));
  invalidator.SetTracksRasterInvalidations(false);
  clip1->ClearChangedToRoot();

  // Change clip1 bigger at one side.
  FloatRoundedRect new_clip_rect3(-500, -500, 2000, 1000);
  clip1->Update(clip1->Parent(),
                ClipPaintPropertyNode::State{clip1->LocalTransformSpace(),
                                             new_clip_rect3});
  auto new_artifact3 = Chunk(0)
                           .Properties(artifact.PaintChunks()[0].properties)
                           .Bounds(artifact.PaintChunks()[0].bounds)
                           .Chunk(1)
                           .Properties(artifact.PaintChunks()[1].properties)
                           .Bounds(new_clip_rect3.Rect())
                           .Build();

  GeometryMapperClipCache::ClearCache();
  invalidator.SetTracksRasterInvalidations(true);
  invalidator.Generate(new_artifact3, kDefaultLayerBounds, layer_state);
  const auto& invalidations1 = TrackedRasterInvalidations(invalidator);
  ASSERT_EQ(1u, invalidations1.size());
  // |clip1| change should trigger incremental raster invalidation.
  EXPECT_INCREMENTAL_INVALIDATION(invalidations1, 0,
                                  new_artifact3.PaintChunks()[1],
                                  IntRect(500, -500, 500, 1000));
  invalidator.SetTracksRasterInvalidations(false);
  clip1->ClearChangedToRoot();
}

TEST_F(CompositedLayerRasterInvalidatorTest, TransformPropertyChange) {
  CompositedLayerRasterInvalidator invalidator(kNoopRasterInvalidation);

  auto layer_transform = CreateTransform(TransformPaintPropertyNode::Root(),
                                         TransformationMatrix().Scale(5));
  auto transform0 = CreateTransform(layer_transform,
                                    TransformationMatrix().Translate(10, 20));
  auto transform1 =
      CreateTransform(transform0, TransformationMatrix().Translate(-50, -60));

  PropertyTreeState layer_state(layer_transform.get(),
                                ClipPaintPropertyNode::Root(),
                                EffectPaintPropertyNode::Root());
  auto artifact = Chunk(0)
                      .Properties(transform0.get())
                      .Chunk(1)
                      .Properties(transform1.get())
                      .Build();

  GeometryMapperTransformCache::ClearCache();
  invalidator.SetTracksRasterInvalidations(true);
  invalidator.Generate(artifact, kDefaultLayerBounds, layer_state);
  EXPECT_TRUE(TrackedRasterInvalidations(invalidator).IsEmpty());

  // Change layer_transform should not cause raster invalidation in the layer.
  layer_transform->Update(
      layer_transform->Parent(),
      TransformPaintPropertyNode::State{TransformationMatrix().Scale(10)});
  auto new_artifact = Chunk(0)
                          .Properties(artifact.PaintChunks()[0].properties)
                          .Chunk(1)
                          .Properties(artifact.PaintChunks()[1].properties)
                          .Build();

  GeometryMapperTransformCache::ClearCache();
  invalidator.Generate(new_artifact, kDefaultLayerBounds, layer_state);
  EXPECT_TRUE(TrackedRasterInvalidations(invalidator).IsEmpty());

  // Inserting another node between layer_transform and transform0 and letting
  // the new node become the transform of the layer state should not cause
  // raster invalidation in the layer. This simulates a composited layer is
  // scrolled from its original location.
  auto new_layer_transform = CreateTransform(
      layer_transform, TransformationMatrix().Translate(-100, -200));
  layer_state = PropertyTreeState(new_layer_transform.get(),
                                  ClipPaintPropertyNode::Root(),
                                  EffectPaintPropertyNode::Root());
  transform0->Update(new_layer_transform,
                     TransformPaintPropertyNode::State{transform0->Matrix()});
  auto new_artifact1 = Chunk(0)
                           .Properties(artifact.PaintChunks()[0].properties)
                           .Chunk(1)
                           .Properties(artifact.PaintChunks()[1].properties)
                           .Build();

  GeometryMapperTransformCache::ClearCache();
  invalidator.Generate(new_artifact1, kDefaultLayerBounds, layer_state);
  EXPECT_TRUE(TrackedRasterInvalidations(invalidator).IsEmpty());

  // Removing transform nodes above the layer state should not cause raster
  // invalidation in the layer.
  layer_state = DefaultPropertyTreeState();
  transform0->Update(layer_state.Transform(),
                     TransformPaintPropertyNode::State{transform0->Matrix()});
  auto new_artifact2 = Chunk(0)
                           .Properties(artifact.PaintChunks()[0].properties)
                           .Chunk(1)
                           .Properties(artifact.PaintChunks()[1].properties)
                           .Build();

  GeometryMapperTransformCache::ClearCache();
  invalidator.Generate(new_artifact2, kDefaultLayerBounds, layer_state);
  EXPECT_TRUE(TrackedRasterInvalidations(invalidator).IsEmpty());

  // Change transform0 and transform1, while keeping the combined transform0
  // and transform1 unchanged for chunk 2. We should invalidate only chunk 0
  // for changed paint property.
  transform0->Update(
      layer_state.Transform(),
      TransformPaintPropertyNode::State{
          TransformationMatrix(transform0->Matrix()).Translate(20, 30)});
  transform1->Update(
      transform0,
      TransformPaintPropertyNode::State{
          TransformationMatrix(transform1->Matrix()).Translate(-20, -30)});
  auto new_artifact3 = Chunk(0)
                           .Properties(artifact.PaintChunks()[0].properties)
                           .Chunk(1)
                           .Properties(artifact.PaintChunks()[1].properties)
                           .Build();

  GeometryMapperTransformCache::ClearCache();
  invalidator.Generate(new_artifact3, kDefaultLayerBounds, layer_state);
  const auto& invalidations = TrackedRasterInvalidations(invalidator);
  ASSERT_EQ(2u, invalidations.size());
  EXPECT_CHUNK_INVALIDATION_WITH_LAYER_OFFSET(
      invalidations, 0, new_artifact3.PaintChunks()[0],
      PaintInvalidationReason::kPaintProperty,
      -kDefaultLayerBounds.Location() + IntSize(10, 20));
  EXPECT_CHUNK_INVALIDATION_WITH_LAYER_OFFSET(
      invalidations, 1, new_artifact3.PaintChunks()[0],
      PaintInvalidationReason::kPaintProperty,
      -kDefaultLayerBounds.Location() + IntSize(30, 50));
  invalidator.SetTracksRasterInvalidations(false);
}

TEST_F(CompositedLayerRasterInvalidatorTest, TransformPropertyTinyChange) {
  CompositedLayerRasterInvalidator invalidator(kNoopRasterInvalidation);

  auto layer_transform = CreateTransform(TransformPaintPropertyNode::Root(),
                                         TransformationMatrix().Scale(5));
  auto chunk_transform = CreateTransform(
      layer_transform, TransformationMatrix().Translate(10, 20));

  PropertyTreeState layer_state(layer_transform.get(),
                                ClipPaintPropertyNode::Root(),
                                EffectPaintPropertyNode::Root());
  auto artifact = Chunk(0).Properties(chunk_transform.get()).Build();

  GeometryMapperTransformCache::ClearCache();
  invalidator.SetTracksRasterInvalidations(true);
  invalidator.Generate(artifact, kDefaultLayerBounds, layer_state);
  EXPECT_TRUE(TrackedRasterInvalidations(invalidator).IsEmpty());

  // Change chunk_transform by tiny difference, which should be ignored.
  chunk_transform->Update(layer_state.Transform(),
                          TransformPaintPropertyNode::State{
                              TransformationMatrix(chunk_transform->Matrix())
                                  .Translate(0.0000001, -0.0000001)
                                  .Scale(1.0000001)
                                  .Rotate(0.0000001)});
  auto new_artifact = Chunk(0).Properties(chunk_transform.get()).Build();

  GeometryMapperTransformCache::ClearCache();
  invalidator.Generate(new_artifact, kDefaultLayerBounds, layer_state);
  EXPECT_TRUE(TrackedRasterInvalidations(invalidator).IsEmpty());

  // Tiny differences should accumulate and cause invalidation when the
  // accumulation is large enough.
  bool invalidated = false;
  for (int i = 0; i < 100 && !invalidated; i++) {
    chunk_transform->Update(layer_state.Transform(),
                            TransformPaintPropertyNode::State{
                                TransformationMatrix(chunk_transform->Matrix())
                                    .Translate(0.0000001, -0.0000001)
                                    .Scale(1.0000001)
                                    .Rotate(0.0000001)});
    auto new_artifact = Chunk(0).Properties(chunk_transform.get()).Build();

    GeometryMapperTransformCache::ClearCache();
    invalidator.Generate(new_artifact, kDefaultLayerBounds, layer_state);
    invalidated = !TrackedRasterInvalidations(invalidator).IsEmpty();
  }
  EXPECT_TRUE(invalidated);
}

}  // namespace blink
