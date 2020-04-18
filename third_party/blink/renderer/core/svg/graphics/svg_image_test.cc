// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/svg/graphics/svg_image.h"

#include <memory>

#include "base/memory/ptr_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/scheduler/test/renderer_scheduler_test_support.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/layout/layout_view.h"
#include "third_party/blink/renderer/core/paint/paint_layer.h"
#include "third_party/blink/renderer/core/svg/graphics/svg_image_chrome_client.h"
#include "third_party/blink/renderer/platform/geometry/float_rect.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_canvas.h"
#include "third_party/blink/renderer/platform/graphics/paint/paint_flags.h"
#include "third_party/blink/renderer/platform/shared_buffer.h"
#include "third_party/blink/renderer/platform/testing/unit_test_helpers.h"
#include "third_party/blink/renderer/platform/timer.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/utils/SkNullCanvas.h"

namespace blink {
class SVGImageTest : public testing::Test {
 public:
  SVGImage& GetImage() { return *image_; }

  void Load(const char* data, bool should_pause) {
    observer_ = new PauseControlImageObserver(should_pause);
    image_ = SVGImage::Create(observer_);
    image_->SetData(SharedBuffer::Create(data, strlen(data)), true);
  }

  void PumpFrame() {
    Image* image = image_.get();
    std::unique_ptr<SkCanvas> null_canvas = SkMakeNullCanvas();
    SkiaPaintCanvas canvas(null_canvas.get());
    PaintFlags flags;
    FloatRect dummy_rect(0, 0, 100, 100);
    image->Draw(&canvas, flags, dummy_rect, dummy_rect,
                kDoNotRespectImageOrientation,
                Image::kDoNotClampImageToSourceRect, Image::kSyncDecode);
  }

 private:
  class PauseControlImageObserver
      : public GarbageCollectedFinalized<PauseControlImageObserver>,
        public ImageObserver {
    USING_GARBAGE_COLLECTED_MIXIN(PauseControlImageObserver);

   public:
    PauseControlImageObserver(bool should_pause)
        : should_pause_(should_pause) {}

    void DecodedSizeChangedTo(const Image*, size_t new_size) override {}

    bool ShouldPauseAnimation(const Image*) override { return should_pause_; }
    void AnimationAdvanced(const Image*) override {}

    void ChangedInRect(const Image*, const IntRect&) override {}

    void AsyncLoadCompleted(const blink::Image*) override {}

    void Trace(blink::Visitor* visitor) override {
      ImageObserver::Trace(visitor);
    }

   private:
    bool should_pause_;
  };
  Persistent<PauseControlImageObserver> observer_;
  scoped_refptr<SVGImage> image_;
};

const char kAnimatedDocument[] =
    "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 16 16'>"
    "<style>"
    "@keyframes rot {"
    " from { transform: rotate(0deg); } to { transform: rotate(-360deg); }"
    "}"
    ".spinner {"
    " transform-origin: 50%% 50%%;"
    " animation-name: rot;"
    " animation-duration: 4s;"
    " animation-iteration-count: infinite;"
    " animation-timing-function: linear;"
    "}"
    "</style>"
    "<path class='spinner' fill='none' d='M 8,1.125 A 6.875,6.875 0 1 1 "
    "1.125,8' stroke-width='2' stroke='blue'/>"
    "</svg>";

TEST_F(SVGImageTest, TimelineSuspendAndResume) {
  const bool kShouldPause = true;
  Load(kAnimatedDocument, kShouldPause);
  SVGImageChromeClient& chrome_client = GetImage().ChromeClientForTesting();
  TaskRunnerTimer<SVGImageChromeClient>* timer =
      new TaskRunnerTimer<SVGImageChromeClient>(
          scheduler::GetSingleThreadTaskRunnerForTesting(), &chrome_client,
          &SVGImageChromeClient::AnimationTimerFired);
  chrome_client.SetTimer(base::WrapUnique(timer));

  // Simulate a draw. Cause a frame (timer) to be scheduled.
  PumpFrame();
  EXPECT_TRUE(GetImage().MaybeAnimated());
  EXPECT_TRUE(timer->IsActive());

  // Fire the timer/trigger a frame update. Since the observer always returns
  // true for shouldPauseAnimation, this will result in the timeline being
  // suspended.
  test::RunDelayedTasks(TimeDelta::FromMilliseconds(1) +
                        TimeDelta::FromSecondsD(timer->NextFireInterval()));
  EXPECT_TRUE(chrome_client.IsSuspended());
  EXPECT_FALSE(timer->IsActive());

  // Simulate a draw. This should resume the animation again.
  PumpFrame();
  EXPECT_TRUE(timer->IsActive());
  EXPECT_FALSE(chrome_client.IsSuspended());
}

TEST_F(SVGImageTest, ResetAnimation) {
  const bool kShouldPause = false;
  Load(kAnimatedDocument, kShouldPause);
  SVGImageChromeClient& chrome_client = GetImage().ChromeClientForTesting();
  TaskRunnerTimer<SVGImageChromeClient>* timer =
      new TaskRunnerTimer<SVGImageChromeClient>(
          scheduler::GetSingleThreadTaskRunnerForTesting(), &chrome_client,
          &SVGImageChromeClient::AnimationTimerFired);
  chrome_client.SetTimer(base::WrapUnique(timer));

  // Simulate a draw. Cause a frame (timer) to be scheduled.
  PumpFrame();
  EXPECT_TRUE(GetImage().MaybeAnimated());
  EXPECT_TRUE(timer->IsActive());

  // Reset the animation. This will suspend the timeline but not cancel the
  // timer.
  GetImage().ResetAnimation();
  EXPECT_TRUE(chrome_client.IsSuspended());
  EXPECT_TRUE(timer->IsActive());

  // Fire the timer/trigger a frame update. The timeline will remain
  // suspended and no frame will be scheduled.
  test::RunDelayedTasks(TimeDelta::FromMillisecondsD(1) +
                        TimeDelta::FromSecondsD(timer->NextFireInterval()));
  EXPECT_TRUE(chrome_client.IsSuspended());
  EXPECT_FALSE(timer->IsActive());

  // Simulate a draw. This should resume the animation again.
  PumpFrame();
  EXPECT_FALSE(chrome_client.IsSuspended());
  EXPECT_TRUE(timer->IsActive());
}

TEST_F(SVGImageTest, SupportsSubsequenceCaching) {
  const bool kShouldPause = true;
  Load(kAnimatedDocument, kShouldPause);
  PumpFrame();
  LocalFrame* local_frame =
      ToLocalFrame(GetImage().GetPageForTesting()->MainFrame());
  EXPECT_TRUE(local_frame->GetDocument()->IsSVGDocument());
  LayoutObject* svg_root = local_frame->View()->GetLayoutView()->FirstChild();
  EXPECT_TRUE(svg_root->IsSVGRoot());
  EXPECT_TRUE(
      ToLayoutBoxModelObject(svg_root)->Layer()->SupportsSubsequenceCaching());
}

TEST_F(SVGImageTest, JankTrackerDisabled) {
  const bool kDontPause = false;
  Load("<svg xmlns='http://www.w3.org/2000/svg'></svg>", kDontPause);
  LocalFrame* local_frame =
      ToLocalFrame(GetImage().GetPageForTesting()->MainFrame());
  EXPECT_TRUE(local_frame->GetDocument()->IsSVGDocument());
  auto& jank_tracker = local_frame->View()->GetJankTracker();
  EXPECT_FALSE(jank_tracker.IsActive());
}

}  // namespace blink
