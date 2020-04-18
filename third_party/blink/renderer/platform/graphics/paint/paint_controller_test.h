// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_PAINT_CONTROLLER_TEST_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_PAINT_CONTROLLER_TEST_H_

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/platform/graphics/paint/clip_recorder.h"
#include "third_party/blink/renderer/platform/graphics/paint/drawing_recorder.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_controller.h"
#include "third_party/blink/renderer/platform/testing/fake_display_item_client.h"
#include "third_party/blink/renderer/platform/testing/paint_property_test_helpers.h"

namespace blink {

class GraphicsContext;

class PaintControllerTestBase : public testing::Test {
 public:
  PaintControllerTestBase()
      : root_paint_property_client_("root"),
        root_paint_chunk_id_(root_paint_property_client_,
                             DisplayItem::kUninitializedType),
        paint_controller_(PaintController::Create()) {}

  static void DrawNothing(GraphicsContext& context,
                          const DisplayItemClient& client,
                          DisplayItem::Type type) {
    if (DrawingRecorder::UseCachedDrawingIfPossible(context, client, type))
      return;
    DrawingRecorder recorder(context, client, type);
  }

  template <typename Rect>
  static void DrawRect(GraphicsContext& context,
                       const DisplayItemClient& client,
                       DisplayItem::Type type,
                       const Rect& bounds) {
    if (DrawingRecorder::UseCachedDrawingIfPossible(context, client, type))
      return;
    DrawingRecorder recorder(context, client, type);
    context.DrawRect(RoundedIntRect(FloatRect(bounds)));
  }

  void InitRootChunk() {
    if (RuntimeEnabledFeatures::SlimmingPaintV175Enabled()) {
      GetPaintController().UpdateCurrentPaintChunkProperties(
          root_paint_chunk_id_, DefaultPaintChunkProperties());
    }
  }

 protected:
  PaintController& GetPaintController() { return *paint_controller_; }

  int NumCachedNewItems() const {
    return paint_controller_->num_cached_new_items_;
  }

#if DCHECK_IS_ON()
  int NumSequentialMatches() const {
    return paint_controller_->num_sequential_matches_;
  }
  int NumOutOfOrderMatches() const {
    return paint_controller_->num_out_of_order_matches_;
  }
  int NumIndexedItems() const { return paint_controller_->num_indexed_items_; }
#endif

  void InvalidateAll() { paint_controller_->InvalidateAllForTesting(); }

  using SubsequenceMarkers = PaintController::SubsequenceMarkers;
  SubsequenceMarkers* GetSubsequenceMarkers(const DisplayItemClient& client) {
    return paint_controller_->GetSubsequenceMarkers(client);
  }

  static bool ClientCacheIsValid(const PaintController& paint_controller,
                                 const DisplayItemClient& client) {
    return paint_controller.ClientCacheIsValid(client);
  }

  bool ClientCacheIsValid(const DisplayItemClient& client) const {
    return ClientCacheIsValid(*paint_controller_, client);
  }

  const ChunkRasterInvalidationRects* GetRasterInvalidationRects(size_t i) {
    return GetPaintController().GetPaintArtifact().GetRasterInvalidationRects(
        i);
  }

 private:
  FakeDisplayItemClient root_paint_property_client_;
  PaintChunk::Id root_paint_chunk_id_;
  std::unique_ptr<PaintController> paint_controller_;
};

class TestDisplayItem final : public DisplayItem {
 public:
  TestDisplayItem(const DisplayItemClient& client, Type type)
      : DisplayItem(client, type, sizeof(*this)) {}

  void Replay(GraphicsContext&) const final { NOTREACHED(); }
  void AppendToDisplayItemList(const FloatSize&,
                               cc::DisplayItemList&) const final {
    NOTREACHED();
  }
};

#define EXPECT_DISPLAY_LIST(actual, expected_size, ...)                   \
  do {                                                                    \
    EXPECT_EQ((size_t)expected_size, actual.size());                      \
    if (expected_size != actual.size())                                   \
      break;                                                              \
    const TestDisplayItem expected[] = {__VA_ARGS__};                     \
    for (size_t i = 0; i < expected_size; ++i) {                          \
      SCOPED_TRACE(                                                       \
          String::Format("%d: Expected:(client=%p:\"%s\" type=%d) "       \
                         "Actual:(client=%p:%s type=%d)",                 \
                         (int)i, &expected[i].Client(),                   \
                         expected[i].Client().DebugName().Ascii().data(), \
                         (int)expected[i].GetType(), &actual[i].Client(), \
                         actual[i].Client().DebugName().Ascii().data(),   \
                         (int)actual[i].GetType()));                      \
      EXPECT_EQ(&expected[i].Client(), &actual[i].Client());              \
      EXPECT_EQ(expected[i].GetType(), actual[i].GetType());              \
    }                                                                     \
  } while (false);

// Shorter names for frequently used display item types in tests.
const DisplayItem::Type kBackgroundType = DisplayItem::kBoxDecorationBackground;
const DisplayItem::Type kForegroundType =
    static_cast<DisplayItem::Type>(DisplayItem::kDrawingPaintPhaseFirst + 4);
const DisplayItem::Type kDocumentBackgroundType =
    DisplayItem::kDocumentBackground;
const DisplayItem::Type kScrollHitTestType = DisplayItem::kScrollHitTest;
const DisplayItem::Type kClipType = DisplayItem::kClipFirst;

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_PAINT_PAINT_CONTROLLER_TEST_H_
