// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/page/page_animator.h"

#include "base/auto_reset.h"
#include "third_party/blink/renderer/core/animation/document_animations.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/page/chrome_client.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/core/svg/svg_document_extensions.h"
#include "third_party/blink/renderer/platform/instrumentation/tracing/trace_event.h"

namespace blink {

PageAnimator::PageAnimator(Page& page)
    : page_(page),
      servicing_animations_(false),
      updating_layout_and_style_for_painting_(false) {}

PageAnimator* PageAnimator::Create(Page& page) {
  return new PageAnimator(page);
}

void PageAnimator::Trace(blink::Visitor* visitor) {
  visitor->Trace(page_);
}

void PageAnimator::ServiceScriptedAnimations(
    base::TimeTicks monotonic_animation_start_time) {
  base::AutoReset<bool> servicing(&servicing_animations_, true);
  Clock().UpdateTime(monotonic_animation_start_time);

  HeapVector<Member<Document>, 32> documents;
  for (Frame* frame = page_->MainFrame(); frame;
       frame = frame->Tree().TraverseNext()) {
    if (frame->IsLocalFrame())
      documents.push_back(ToLocalFrame(frame)->GetDocument());
  }

  for (auto& document : documents) {
    ScopedFrameBlamer frame_blamer(document->GetFrame());
    TRACE_EVENT0("blink,rail", "PageAnimator::serviceScriptedAnimations");
    DocumentAnimations::UpdateAnimationTimingForAnimationFrame(*document);
    if (document->View()) {
      if (document->View()->ShouldThrottleRendering())
        continue;
      // Disallow throttling in case any script needs to do a synchronous
      // lifecycle update in other frames which are throttled.
      DocumentLifecycle::DisallowThrottlingScope no_throttling_scope(
          document->Lifecycle());
      if (ScrollableArea* scrollable_area =
              document->View()->GetScrollableArea()) {
        scrollable_area->ServiceScrollAnimations(
            monotonic_animation_start_time.since_origin().InSecondsF());
      }

      if (const LocalFrameView::ScrollableAreaSet* animating_scrollable_areas =
              document->View()->AnimatingScrollableAreas()) {
        // Iterate over a copy, since ScrollableAreas may deregister
        // themselves during the iteration.
        HeapVector<Member<ScrollableArea>> animating_scrollable_areas_copy;
        CopyToVector(*animating_scrollable_areas,
                     animating_scrollable_areas_copy);
        for (ScrollableArea* scrollable_area :
             animating_scrollable_areas_copy) {
          scrollable_area->ServiceScrollAnimations(
              monotonic_animation_start_time.since_origin().InSecondsF());
        }
      }
      SVGDocumentExtensions::ServiceOnAnimationFrame(*document);
    }
    // TODO(skyostil): This function should not run for documents without views.
    DocumentLifecycle::DisallowThrottlingScope no_throttling_scope(
        document->Lifecycle());
    document->ServiceScriptedAnimations(monotonic_animation_start_time);
  }
}

void PageAnimator::SetSuppressFrameRequestsWorkaroundFor704763Only(
    bool suppress_frame_requests) {
  // If we are enabling the suppression and it was already enabled then we must
  // have missed disabling it at the end of a previous frame.
  DCHECK(!suppress_frame_requests_workaround_for704763_only_ ||
         !suppress_frame_requests);
  suppress_frame_requests_workaround_for704763_only_ = suppress_frame_requests;
}

DISABLE_CFI_PERF
void PageAnimator::ScheduleVisualUpdate(LocalFrame* frame) {
  if (servicing_animations_ || updating_layout_and_style_for_painting_ ||
      suppress_frame_requests_workaround_for704763_only_) {
    return;
  }
  page_->GetChromeClient().ScheduleAnimation(frame->View());
}

void PageAnimator::UpdateAllLifecyclePhases(LocalFrame& root_frame) {
  LocalFrameView* view = root_frame.View();
  base::AutoReset<bool> servicing(&updating_layout_and_style_for_painting_,
                                  true);
  view->UpdateAllLifecyclePhases();
}

void PageAnimator::UpdateLifecycleToPrePaintClean(LocalFrame& root_frame) {
  LocalFrameView* view = root_frame.View();
  base::AutoReset<bool> servicing(&updating_layout_and_style_for_painting_,
                                  true);
  view->UpdateLifecycleToPrePaintClean();
}

}  // namespace blink
