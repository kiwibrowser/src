// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_TESTING_TEST_PAINT_ARTIFACT_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_TESTING_TEST_PAINT_ARTIFACT_H_

#include <memory>
#include "base/memory/scoped_refptr.h"
#include "third_party/blink/renderer/platform/graphics/color.h"
#include "third_party/blink/renderer/platform/graphics/paint/display_item_list.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_artifact.h"
#include "third_party/blink/renderer/platform/graphics/paint/transform_paint_property_node.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace cc {
class Layer;
}

namespace blink {

class ClipPaintPropertyNode;
class EffectPaintPropertyNode;
class FloatRect;
class PaintArtifact;
class TransformPaintPropertyNode;

// Useful for quickly making a paint artifact in unit tests.
// Must remain in scope while the paint artifact is used, because it owns the
// display item clients.
//
// Usage:
//   TestPaintArtifact artifact;
//   artifact.Chunk(paintProperties)
//       .RectDrawing(bounds, color)
//       .RectDrawing(bounds2, color2);
//   artifact.Chunk(otherPaintProperties)
//       .RectDrawing(bounds3, color3);
//   DoSomethingWithArtifact(artifact);
class TestPaintArtifact {
  STACK_ALLOCATED();

 public:
  TestPaintArtifact();
  ~TestPaintArtifact();

  // Add to the artifact.
  TestPaintArtifact& Chunk(scoped_refptr<const TransformPaintPropertyNode>,
                           scoped_refptr<const ClipPaintPropertyNode>,
                           scoped_refptr<const EffectPaintPropertyNode>);
  TestPaintArtifact& Chunk(const PropertyTreeState&);
  TestPaintArtifact& RectDrawing(const FloatRect& bounds, Color);
  TestPaintArtifact& ForeignLayer(const FloatPoint&,
                                  const IntSize&,
                                  scoped_refptr<cc::Layer>);
  TestPaintArtifact& ScrollHitTest(
      scoped_refptr<const TransformPaintPropertyNode> scroll_offset);
  TestPaintArtifact& KnownToBeOpaque();

  // Add to the artifact, with specified display item client. These are used
  // to test incremental paint artifact updates.
  TestPaintArtifact& Chunk(DisplayItemClient&,
                           scoped_refptr<const TransformPaintPropertyNode>,
                           scoped_refptr<const ClipPaintPropertyNode>,
                           scoped_refptr<const EffectPaintPropertyNode>);
  TestPaintArtifact& Chunk(DisplayItemClient&, const PropertyTreeState&);
  TestPaintArtifact& RectDrawing(DisplayItemClient&,
                                 const FloatRect& bounds,
                                 Color);
  TestPaintArtifact& ForeignLayer(DisplayItemClient&,
                                  const FloatPoint&,
                                  const IntSize&,
                                  scoped_refptr<cc::Layer>);
  TestPaintArtifact& ScrollHitTest(
      DisplayItemClient&,
      scoped_refptr<const TransformPaintPropertyNode> scroll_offset);

  // Can't add more things once this is called.
  const PaintArtifact& Build();

  // Create a new display item client which is owned by this TestPaintArtifact.
  DisplayItemClient& NewClient();

  DisplayItemClient& Client(size_t) const;

 private:
  class DummyRectClient;
  Vector<std::unique_ptr<DummyRectClient>> dummy_clients_;

  // Exists if m_built is false.
  DisplayItemList display_item_list_;
  PaintChunksAndRasterInvalidations paint_chunks_data_;

  // Exists if m_built is true.
  PaintArtifact paint_artifact_;

  bool built_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_TESTING_TEST_PAINT_ARTIFACT_H_
