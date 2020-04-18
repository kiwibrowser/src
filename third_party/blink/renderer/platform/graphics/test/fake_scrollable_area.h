// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_TEST_FAKE_SCROLLABLE_AREA_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_GRAPHICS_TEST_FAKE_SCROLLABLE_AREA_H_

#include "base/single_thread_task_runner.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/scheduler/test/renderer_scheduler_test_support.h"
#include "third_party/blink/renderer/platform/scheduler/public/thread_scheduler.h"
#include "third_party/blink/renderer/platform/scroll/scrollable_area.h"
#include "third_party/blink/renderer/platform/scroll/scrollbar_theme_mock.h"

namespace {
blink::ScrollbarThemeMock scrollbar_theme_;
}

namespace blink {

class FakeScrollableArea : public GarbageCollectedFinalized<FakeScrollableArea>,
                           public ScrollableArea {
  USING_GARBAGE_COLLECTED_MIXIN(FakeScrollableArea);

 public:
  static FakeScrollableArea* Create() { return new FakeScrollableArea; }

  CompositorElementId GetCompositorElementId() const override {
    return CompositorElementId();
  }
  bool IsActive() const override { return false; }
  int ScrollSize(ScrollbarOrientation) const override { return 100; }
  bool IsScrollCornerVisible() const override { return false; }
  IntRect ScrollCornerRect() const override { return IntRect(); }
  IntRect VisibleContentRect(
      IncludeScrollbarsInRect = kExcludeScrollbars) const override {
    return IntRect(ScrollOffsetInt().Width(), ScrollOffsetInt().Height(), 10,
                   10);
  }
  IntSize ContentsSize() const override { return IntSize(100, 100); }
  bool ScrollbarsCanBeActive() const override { return false; }
  IntRect ScrollableAreaBoundingBox() const override { return IntRect(); }
  void ScrollControlWasSetNeedsPaintInvalidation() override {}
  bool UserInputScrollable(ScrollbarOrientation) const override { return true; }
  bool ShouldPlaceVerticalScrollbarOnLeft() const override { return false; }
  int PageStep(ScrollbarOrientation) const override { return 0; }
  IntSize MinimumScrollOffsetInt() const override { return IntSize(); }
  IntSize MaximumScrollOffsetInt() const override {
    return ContentsSize() - IntSize(VisibleWidth(), VisibleHeight());
  }

  void UpdateScrollOffset(const ScrollOffset& offset, ScrollType) override {
    scroll_offset_ = offset;
  }
  ScrollOffset GetScrollOffset() const override { return scroll_offset_; }
  IntSize ScrollOffsetInt() const override {
    return FlooredIntSize(scroll_offset_);
  }

  scoped_refptr<base::SingleThreadTaskRunner> GetTimerTaskRunner() const final {
    if (!timer_task_runner_) {
      timer_task_runner_ =
          blink::scheduler::GetSingleThreadTaskRunnerForTesting();
    }
    return timer_task_runner_;
  }

  ScrollbarTheme& GetPageScrollbarTheme() const override {
    return scrollbar_theme_;
  }

  void Trace(blink::Visitor* visitor) override {
    ScrollableArea::Trace(visitor);
  }

 private:
  ScrollOffset scroll_offset_;
  mutable scoped_refptr<base::SingleThreadTaskRunner> timer_task_runner_;
};

}  // namespace blink

#endif  // FakeScrollableArea
