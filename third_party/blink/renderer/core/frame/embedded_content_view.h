// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_EMBEDDED_CONTENT_VIEW_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_EMBEDDED_CONTENT_VIEW_H_

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/paint/paint_phase.h"
#include "third_party/blink/renderer/platform/geometry/int_size.h"
#include "third_party/blink/renderer/platform/heap/handle.h"

namespace blink {

class CullRect;
class GraphicsContext;
class IntRect;
class IntSize;

// EmbeddedContentView is a pure virtual class which is implemented by
// LocalFrameView, RemoteFrameView, and WebPluginContainerImpl.
class CORE_EXPORT EmbeddedContentView : public GarbageCollectedMixin {
 public:
  virtual ~EmbeddedContentView() = default;

  virtual bool IsFrameView() const { return false; }
  virtual bool IsLocalFrameView() const { return false; }
  virtual bool IsPluginView() const { return false; }

  virtual void AttachToLayout() = 0;
  virtual void DetachFromLayout() = 0;
  virtual bool IsAttached() const = 0;
  virtual void SetParentVisible(bool) = 0;
  virtual void SetFrameRect(const IntRect&) = 0;
  virtual void FrameRectsChanged() = 0;
  virtual IntRect FrameRect() const = 0;
  virtual void Paint(GraphicsContext&,
                     const GlobalPaintFlags,
                     const CullRect&,
                     const IntSize& paint_offset = IntSize()) const = 0;
  // Called when the size of the view changes.  Implementations of
  // EmbeddedContentView should call LayoutEmbeddedContent::UpdateGeometry in
  // addition to any internal logic.
  virtual void UpdateGeometry() = 0;
  virtual void Show() = 0;
  virtual void Hide() = 0;
  virtual void Dispose() = 0;
};

}  // namespace blink
#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_EMBEDDED_CONTENT_VIEW_H_
