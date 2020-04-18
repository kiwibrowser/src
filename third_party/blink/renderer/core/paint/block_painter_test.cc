// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/paint/block_painter.h"

#include <gtest/gtest.h>
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/paint/paint_controller_paint_test.h"
#include "third_party/blink/renderer/platform/graphics/paint/drawing_display_item.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_chunk.h"
#include "third_party/blink/renderer/platform/graphics/paint/scroll_hit_test_display_item.h"

namespace blink {

using BlockPainterTest = PaintControllerPaintTest;

INSTANTIATE_SPV2_TEST_CASE_P(BlockPainterTest);

TEST_P(BlockPainterTest, ScrollHitTestProperties) {
  SetBodyInnerHTML(R"HTML(
    <style>
      ::-webkit-scrollbar { display: none; }
      body { margin: 0 }
      #container { width: 200px; height: 200px;
                  overflow: scroll; background: blue; }
      #child { width: 100px; height: 300px; background: green; }
    </style>
    <div id='container'>
      <div id='child'></div>
    </div>
  )HTML");

  auto& container = *GetLayoutObjectByElementId("container");
  auto& child = *GetLayoutObjectByElementId("child");

  // The scroll hit test should be after the container background but before the
  // scrolled contents.
  EXPECT_DISPLAY_LIST(RootPaintController().GetDisplayItemList(), 4,
                      TestDisplayItem(GetLayoutView(), kDocumentBackgroundType),
                      TestDisplayItem(container, kBackgroundType),
                      TestDisplayItem(container, kScrollHitTestType),
                      TestDisplayItem(child, kBackgroundType));
  const auto& paint_chunks =
      RootPaintController().GetPaintArtifact().PaintChunks();
  EXPECT_EQ(4u, paint_chunks.size());
  const auto& root_chunk = RootPaintController().PaintChunks()[0];
  EXPECT_EQ(GetLayoutView().Layer(), &root_chunk.id.client);
  const auto& container_chunk = RootPaintController().PaintChunks()[1];
  EXPECT_EQ(ToLayoutBoxModelObject(container).Layer(),
            &container_chunk.id.client);
  // The container's scroll hit test.
  const auto& scroll_hit_test_chunk = RootPaintController().PaintChunks()[2];
  EXPECT_EQ(&container, &scroll_hit_test_chunk.id.client);
  EXPECT_EQ(kScrollHitTestType, scroll_hit_test_chunk.id.type);
  // The scrolled contents.
  const auto& contents_chunk = RootPaintController().PaintChunks()[3];
  EXPECT_EQ(&container, &contents_chunk.id.client);

  // The document should not scroll so there should be no scroll offset
  // transform.
  auto* root_transform = root_chunk.properties.Transform();
  EXPECT_EQ(nullptr, root_transform->ScrollNode());

  // The container's background chunk should not scroll and therefore should use
  // the root transform. Its local transform is actually a paint offset
  // transform.
  auto* container_transform = container_chunk.properties.Transform()->Parent();
  EXPECT_EQ(root_transform, container_transform);
  EXPECT_EQ(nullptr, container_transform->ScrollNode());

  // The scroll hit test should not be scrolled and should not be clipped.
  // Its local transform is actually a paint offset transform.
  auto* scroll_hit_test_transform =
      scroll_hit_test_chunk.properties.Transform()->Parent();
  EXPECT_EQ(nullptr, scroll_hit_test_transform->ScrollNode());
  EXPECT_EQ(root_transform, scroll_hit_test_transform);
  auto* scroll_hit_test_clip = scroll_hit_test_chunk.properties.Clip();
  EXPECT_EQ(FloatRect(0, 0, 800, 600), scroll_hit_test_clip->ClipRect().Rect());

  // The scrolled contents should be scrolled and clipped.
  auto* contents_transform = contents_chunk.properties.Transform();
  auto* contents_scroll = contents_transform->ScrollNode();
  EXPECT_EQ(IntRect(0, 0, 200, 300), contents_scroll->ContentsRect());
  EXPECT_EQ(IntRect(0, 0, 200, 200), contents_scroll->ContainerRect());
  auto* contents_clip = contents_chunk.properties.Clip();
  EXPECT_EQ(FloatRect(0, 0, 200, 200), contents_clip->ClipRect().Rect());

  // The scroll hit test display item maintains a reference to a scroll offset
  // translation node and the contents should be scrolled by this node.
  const auto& scroll_hit_test_display_item =
      static_cast<const ScrollHitTestDisplayItem&>(
          RootPaintController()
              .GetDisplayItemList()[scroll_hit_test_chunk.begin_index]);
  EXPECT_EQ(contents_transform,
            &scroll_hit_test_display_item.scroll_offset_node());
}

TEST_P(BlockPainterTest, FrameScrollHitTestProperties) {
  SetBodyInnerHTML(R"HTML(
    <style>
      ::-webkit-scrollbar { display: none; }
      body { margin: 0; }
      #child { width: 100px; height: 2000px; background: green; }
    </style>
    <div id='child'></div>
  )HTML");

  auto& html = *GetDocument().documentElement()->GetLayoutObject();
  auto& child = *GetLayoutObjectByElementId("child");

  // The scroll hit test should be after the document background but before the
  // scrolled contents.
  EXPECT_DISPLAY_LIST(RootPaintController().GetDisplayItemList(), 3,
                      TestDisplayItem(GetLayoutView(), kDocumentBackgroundType),
                      TestDisplayItem(GetLayoutView(), kScrollHitTestType),
                      TestDisplayItem(child, kBackgroundType));

  const auto& paint_chunks =
      RootPaintController().GetPaintArtifact().PaintChunks();
  EXPECT_EQ(3u, paint_chunks.size());
  const auto& root_chunk = RootPaintController().PaintChunks()[0];
  EXPECT_EQ(GetLayoutView().Layer(), &root_chunk.id.client);
  const auto& scroll_hit_test_chunk = RootPaintController().PaintChunks()[1];
  EXPECT_EQ(&GetLayoutView(), &scroll_hit_test_chunk.id.client);
  EXPECT_EQ(kScrollHitTestType, scroll_hit_test_chunk.id.type);
  // The scrolled contents.
  const auto& contents_chunk = RootPaintController().PaintChunks()[2];
  EXPECT_EQ(ToLayoutBoxModelObject(html).Layer(), &contents_chunk.id.client);

  // The scroll hit test should not be scrolled and should not be clipped.
  auto* scroll_hit_test_transform =
      scroll_hit_test_chunk.properties.Transform();
  EXPECT_EQ(nullptr, scroll_hit_test_transform->ScrollNode());
  auto* scroll_hit_test_clip = scroll_hit_test_chunk.properties.Clip();
  EXPECT_EQ(LayoutRect::InfiniteIntRect(),
            scroll_hit_test_clip->ClipRect().Rect());

  // The scrolled contents should be scrolled and clipped.
  auto* contents_transform = contents_chunk.properties.Transform();
  auto* contents_scroll = contents_transform->ScrollNode();
  EXPECT_EQ(IntRect(0, 0, 800, 2000), contents_scroll->ContentsRect());
  EXPECT_EQ(IntRect(0, 0, 800, 600), contents_scroll->ContainerRect());
  auto* contents_clip = contents_chunk.properties.Clip();
  EXPECT_EQ(FloatRect(0, 0, 800, 600), contents_clip->ClipRect().Rect());

  // The scroll hit test display item maintains a reference to a scroll offset
  // translation node and the contents should be scrolled by this node.
  const auto& scroll_hit_test_display_item =
      static_cast<const ScrollHitTestDisplayItem&>(
          RootPaintController()
              .GetDisplayItemList()[scroll_hit_test_chunk.begin_index]);
  EXPECT_EQ(contents_transform,
            &scroll_hit_test_display_item.scroll_offset_node());
}

}  // namespace blink
