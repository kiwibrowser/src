// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_SCROLLBAR_MANAGER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_SCROLLBAR_MANAGER_H_

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/scroll/scrollable_area.h"

namespace blink {

class CORE_EXPORT ScrollbarManager {
  DISALLOW_NEW();

  // Helper class to manage the life cycle of Scrollbar objects.
 public:
  ScrollbarManager(ScrollableArea&);

  void Dispose();

  Scrollbar* HorizontalScrollbar() const {
    return h_bar_is_attached_ ? h_bar_.Get() : nullptr;
  }
  Scrollbar* VerticalScrollbar() const {
    return v_bar_is_attached_ ? v_bar_.Get() : nullptr;
  }
  bool HasHorizontalScrollbar() const { return HorizontalScrollbar(); }
  bool HasVerticalScrollbar() const { return VerticalScrollbar(); }

  // These functions are used to create/destroy scrollbars.
  virtual void SetHasHorizontalScrollbar(bool has_scrollbar) = 0;
  virtual void SetHasVerticalScrollbar(bool has_scrollbar) = 0;

  virtual void Trace(blink::Visitor*);

 protected:
  // TODO(ymalik): This can be made non-virtual since there's a lot of
  // common code in subclasses.
  virtual Scrollbar* CreateScrollbar(ScrollbarOrientation) = 0;
  virtual void DestroyScrollbar(ScrollbarOrientation) = 0;

 protected:
  Member<ScrollableArea> scrollable_area_;

  // The scrollbars associated with m_scrollableArea. Both can nullptr.
  Member<Scrollbar> h_bar_;
  Member<Scrollbar> v_bar_;

  unsigned h_bar_is_attached_ : 1;
  unsigned v_bar_is_attached_ : 1;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_SCROLLBAR_MANAGER_H_
