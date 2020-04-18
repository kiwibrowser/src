// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/paint/paint_timing.h"

#include <memory>
#include <utility>

#include "third_party/blink/public/platform/web_layer_tree_view.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/local_frame_view.h"
#include "third_party/blink/renderer/core/loader/document_loader.h"
#include "third_party/blink/renderer/core/loader/interactive_detector.h"
#include "third_party/blink/renderer/core/loader/progress_tracker.h"
#include "third_party/blink/renderer/core/page/chrome_client.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/core/probe/core_probes.h"
#include "third_party/blink/renderer/core/timing/dom_window_performance.h"
#include "third_party/blink/renderer/core/timing/window_performance.h"
#include "third_party/blink/renderer/platform/cross_thread_functional.h"
#include "third_party/blink/renderer/platform/histogram.h"
#include "third_party/blink/renderer/platform/instrumentation/tracing/trace_event.h"
#include "third_party/blink/renderer/platform/scheduler/public/frame_scheduler.h"

namespace blink {

namespace {

WindowPerformance* GetPerformanceInstance(LocalFrame* frame) {
  WindowPerformance* performance = nullptr;
  if (frame && frame->DomWindow()) {
    performance = DOMWindowPerformance::performance(*frame->DomWindow());
  }
  return performance;
}

}  // namespace

// static
const char PaintTiming::kSupplementName[] = "PaintTiming";

// static
PaintTiming& PaintTiming::From(Document& document) {
  PaintTiming* timing = Supplement<Document>::From<PaintTiming>(document);
  if (!timing) {
    timing = new PaintTiming(document);
    ProvideTo(document, timing);
  }
  return *timing;
}

void PaintTiming::MarkFirstPaint() {
  // Test that m_firstPaint is non-zero here, as well as in setFirstPaint, so
  // we avoid invoking monotonicallyIncreasingTime() on every call to
  // markFirstPaint().
  if (!first_paint_.is_null())
    return;
  SetFirstPaint(CurrentTimeTicks());
}

void PaintTiming::MarkFirstContentfulPaint() {
  // Test that m_firstContentfulPaint is non-zero here, as well as in
  // setFirstContentfulPaint, so we avoid invoking
  // monotonicallyIncreasingTime() on every call to
  // markFirstContentfulPaint().
  if (!first_contentful_paint_.is_null())
    return;
  SetFirstContentfulPaint(CurrentTimeTicks());
}

void PaintTiming::MarkFirstTextPaint() {
  if (!first_text_paint_.is_null())
    return;
  first_text_paint_ = CurrentTimeTicks();
  SetFirstContentfulPaint(first_text_paint_);
  RegisterNotifySwapTime(PaintEvent::kFirstTextPaint);
}

void PaintTiming::MarkFirstImagePaint() {
  if (!first_image_paint_.is_null())
    return;
  first_image_paint_ = CurrentTimeTicks();
  SetFirstContentfulPaint(first_image_paint_);
  RegisterNotifySwapTime(PaintEvent::kFirstImagePaint);
}

void PaintTiming::SetFirstMeaningfulPaintCandidate(TimeTicks timestamp) {
  if (!first_meaningful_paint_candidate_.is_null())
    return;
  first_meaningful_paint_candidate_ = timestamp;
  if (GetFrame() && GetFrame()->View() && !GetFrame()->View()->IsAttached()) {
    GetFrame()->GetFrameScheduler()->OnFirstMeaningfulPaint();
  }
}

void PaintTiming::SetFirstMeaningfulPaint(
    TimeTicks stamp,
    TimeTicks swap_stamp,
    FirstMeaningfulPaintDetector::HadUserInput had_input) {
  DCHECK(first_meaningful_paint_.is_null());
  DCHECK(first_meaningful_paint_swap_.is_null());
  DCHECK(!stamp.is_null());
  DCHECK(!swap_stamp.is_null());

  TRACE_EVENT_MARK_WITH_TIMESTAMP2(
      "loading,rail,devtools.timeline", "firstMeaningfulPaint", swap_stamp,
      "frame", ToTraceValue(GetFrame()), "afterUserInput", had_input);

  InteractiveDetector* interactive_detector(
      InteractiveDetector::From(*GetSupplementable()));
  if (interactive_detector) {
    interactive_detector->OnFirstMeaningfulPaintDetected(swap_stamp, had_input);
  }

  // Notify FMP for UMA only if there's no user input before FMP, so that layout
  // changes caused by user interactions wouldn't be considered as FMP.
  if (had_input == FirstMeaningfulPaintDetector::kNoUserInput) {
    first_meaningful_paint_ = stamp;
    first_meaningful_paint_swap_ = swap_stamp;
    NotifyPaintTimingChanged();
  }

  ReportUserInputHistogram(had_input);
}

void PaintTiming::ReportUserInputHistogram(
    FirstMeaningfulPaintDetector::HadUserInput had_input) {
  DEFINE_STATIC_LOCAL(EnumerationHistogram, had_user_input_histogram,
                      ("PageLoad.Internal.PaintTiming."
                       "HadUserInputBeforeFirstMeaningfulPaint",
                       FirstMeaningfulPaintDetector::kHadUserInputEnumMax));

  if (GetFrame() && GetFrame()->IsMainFrame())
    had_user_input_histogram.Count(had_input);
}

void PaintTiming::NotifyPaint(bool is_first_paint,
                              bool text_painted,
                              bool image_painted) {
  if (is_first_paint)
    MarkFirstPaint();
  if (text_painted)
    MarkFirstTextPaint();
  if (image_painted)
    MarkFirstImagePaint();
  fmp_detector_->NotifyPaint();
}

void PaintTiming::Trace(blink::Visitor* visitor) {
  visitor->Trace(fmp_detector_);
  Supplement<Document>::Trace(visitor);
}

PaintTiming::PaintTiming(Document& document)
    : Supplement<Document>(document),
      fmp_detector_(new FirstMeaningfulPaintDetector(this, document)) {}

LocalFrame* PaintTiming::GetFrame() const {
  return GetSupplementable()->GetFrame();
}

void PaintTiming::NotifyPaintTimingChanged() {
  if (GetSupplementable()->Loader())
    GetSupplementable()->Loader()->DidChangePerformanceTiming();
}

void PaintTiming::SetFirstPaint(TimeTicks stamp) {
  if (!first_paint_.is_null())
    return;
  first_paint_ = stamp;
  RegisterNotifySwapTime(PaintEvent::kFirstPaint);
}

void PaintTiming::SetFirstContentfulPaint(TimeTicks stamp) {
  if (!first_contentful_paint_.is_null())
    return;
  SetFirstPaint(stamp);
  first_contentful_paint_ = stamp;
  RegisterNotifySwapTime(PaintEvent::kFirstContentfulPaint);
}

void PaintTiming::RegisterNotifySwapTime(PaintEvent event) {
  RegisterNotifySwapTime(
      event, CrossThreadBind(&PaintTiming::ReportSwapTime,
                             WrapCrossThreadWeakPersistent(this), event));
}

void PaintTiming::RegisterNotifySwapTime(PaintEvent event,
                                         ReportTimeCallback callback) {
  // ReportSwapTime on layerTreeView will queue a swap-promise, the callback is
  // called when the swap for current render frame completes or fails to happen.
  if (!GetFrame() || !GetFrame()->GetPage())
    return;
  if (WebLayerTreeView* layerTreeView =
          GetFrame()->GetPage()->GetChromeClient().GetWebLayerTreeView(
              GetFrame())) {
    layerTreeView->NotifySwapTime(ConvertToBaseCallback(std::move(callback)));
  }
}

void PaintTiming::ReportSwapTime(PaintEvent event,
                                 WebLayerTreeView::SwapResult result,
                                 base::TimeTicks timestamp) {
  // If the swap fails for any reason, we use the timestamp when the SwapPromise
  // was broken. |result| == WebLayerTreeView::SwapResult::kDidNotSwapSwapFails
  // usually means the compositor decided not swap because there was no actual
  // damage, which can happen when what's being painted isn't visible. In this
  // case, the timestamp will be consistent with the case where the swap
  // succeeds, as they both capture the time up to swap. In other failure cases
  // (aborts during commit), this timestamp is an improvement over the blink
  // paint time, but does not capture some time we're interested in, e.g.  image
  // decoding.
  //
  // TODO(crbug.com/738235): Consider not reporting any timestamp when failing
  // for reasons other than kDidNotSwapSwapFails.
  ReportSwapResultHistogram(result);
  switch (event) {
    case PaintEvent::kFirstPaint:
      SetFirstPaintSwap(timestamp);
      return;
    case PaintEvent::kFirstContentfulPaint:
      SetFirstContentfulPaintSwap(timestamp);
      return;
    case PaintEvent::kFirstTextPaint:
      SetFirstTextPaintSwap(timestamp);
      return;
    case PaintEvent::kFirstImagePaint:
      SetFirstImagePaintSwap(timestamp);
      return;
    default:
      NOTREACHED();
  }
}

void PaintTiming::SetFirstPaintSwap(TimeTicks stamp) {
  DCHECK(first_paint_swap_.is_null());
  first_paint_swap_ = stamp;
  probe::paintTiming(GetSupplementable(), "firstPaint",
                     TimeTicksInSeconds(first_paint_swap_));
  WindowPerformance* performance = GetPerformanceInstance(GetFrame());
  if (performance)
    performance->AddFirstPaintTiming(first_paint_swap_);
  NotifyPaintTimingChanged();
}

void PaintTiming::SetFirstContentfulPaintSwap(TimeTicks stamp) {
  DCHECK(first_contentful_paint_swap_.is_null());
  first_contentful_paint_swap_ = stamp;
  probe::paintTiming(GetSupplementable(), "firstContentfulPaint",
                     TimeTicksInSeconds(first_contentful_paint_swap_));
  WindowPerformance* performance = GetPerformanceInstance(GetFrame());
  if (performance)
    performance->AddFirstContentfulPaintTiming(first_contentful_paint_swap_);
  if (GetFrame())
    GetFrame()->Loader().Progress().DidFirstContentfulPaint();
  NotifyPaintTimingChanged();
  fmp_detector_->NotifyFirstContentfulPaint(first_contentful_paint_swap_);
}

void PaintTiming::SetFirstTextPaintSwap(TimeTicks stamp) {
  DCHECK(first_text_paint_swap_.is_null());
  first_text_paint_swap_ = stamp;
  probe::paintTiming(GetSupplementable(), "firstTextPaint",
                     TimeTicksInSeconds(first_text_paint_swap_));
  NotifyPaintTimingChanged();
}

void PaintTiming::SetFirstImagePaintSwap(TimeTicks stamp) {
  DCHECK(first_image_paint_swap_.is_null());
  first_image_paint_swap_ = stamp;
  probe::paintTiming(GetSupplementable(), "firstImagePaint",
                     TimeTicksInSeconds(first_image_paint_swap_));
  NotifyPaintTimingChanged();
}

void PaintTiming::ReportSwapResultHistogram(
    const WebLayerTreeView::SwapResult result) {
  DEFINE_STATIC_LOCAL(EnumerationHistogram, did_swap_histogram,
                      ("PageLoad.Internal.Renderer.PaintTiming.SwapResult",
                       WebLayerTreeView::SwapResult::kSwapResultMax));
  did_swap_histogram.Count(result);
}

}  // namespace blink
