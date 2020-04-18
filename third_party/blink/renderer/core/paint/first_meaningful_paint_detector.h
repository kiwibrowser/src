// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_FIRST_MEANINGFUL_PAINT_DETECTOR_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PAINT_FIRST_MEANINGFUL_PAINT_DETECTOR_H_

#include "base/macros.h"
#include "third_party/blink/public/platform/web_layer_tree_view.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/paint/paint_event.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/timer.h"
#include "third_party/blink/renderer/platform/wtf/time.h"

namespace blink {

class Document;
class PaintTiming;

// FirstMeaningfulPaintDetector observes layout operations during page load
// until network stable (2 seconds of no network activity), and computes the
// layout-based First Meaningful Paint.
// See https://goo.gl/vpaxv6 and http://goo.gl/TEiMi4 for more details.
class CORE_EXPORT FirstMeaningfulPaintDetector
    : public GarbageCollectedFinalized<FirstMeaningfulPaintDetector> {

 public:
  // Used by FrameView to keep track of the number of layout objects created
  // in the frame.
  class LayoutObjectCounter {
   public:
    void Reset() { count_ = 0; }
    void Increment() { count_++; }
    unsigned Count() const { return count_; }

   private:
    unsigned count_ = 0;
  };

  static FirstMeaningfulPaintDetector& From(Document&);

  FirstMeaningfulPaintDetector(PaintTiming*, Document&);
  virtual ~FirstMeaningfulPaintDetector() = default;

  void MarkNextPaintAsMeaningfulIfNeeded(const LayoutObjectCounter&,
                                         int contents_height_before_layout,
                                         int contents_height_after_layout,
                                         int visible_height);
  void NotifyInputEvent();
  void NotifyPaint();
  void CheckNetworkStable();
  void ReportSwapTime(PaintEvent,
                      WebLayerTreeView::SwapResult,
                      base::TimeTicks);
  void NotifyFirstContentfulPaint(TimeTicks swap_stamp);

  void Trace(blink::Visitor*);

  enum HadUserInput { kNoUserInput, kHadUserInput, kHadUserInputEnumMax };

 private:
  friend class FirstMeaningfulPaintDetectorTest;

  enum DeferFirstMeaningfulPaint {
    kDoNotDefer,
    kDeferOutstandingSwapPromises,
    kDeferFirstContentfulPaintNotSet
  };

  // The page is n-quiet if there are no more than n active network requests for
  // this duration of time.
  static constexpr double kNetwork2QuietWindowSeconds = 0.5;
  static constexpr double kNetwork0QuietWindowSeconds = 0.5;

  Document* GetDocument();
  int ActiveConnections();
  void SetNetworkQuietTimers(int active_connections);
  void Network0QuietTimerFired(TimerBase*);
  void Network2QuietTimerFired(TimerBase*);
  void ReportHistograms();
  void RegisterNotifySwapTime(PaintEvent);
  void SetFirstMeaningfulPaint(TimeTicks stamp, TimeTicks swap_stamp);

  bool next_paint_is_meaningful_ = false;
  HadUserInput had_user_input_ = kNoUserInput;
  HadUserInput had_user_input_before_provisional_first_meaningful_paint_ =
      kNoUserInput;

  Member<PaintTiming> paint_timing_;
  TimeTicks provisional_first_meaningful_paint_;
  TimeTicks provisional_first_meaningful_paint_swap_;
  double max_significance_so_far_ = 0.0;
  double accumulated_significance_while_having_blank_text_ = 0.0;
  unsigned prev_layout_object_count_ = 0;
  bool seen_first_meaningful_paint_candidate_ = false;
  bool network0_quiet_reached_ = false;
  bool network2_quiet_reached_ = false;
  TimeTicks first_meaningful_paint0_quiet_;
  TimeTicks first_meaningful_paint2_quiet_;
  unsigned outstanding_swap_promise_count_ = 0;
  DeferFirstMeaningfulPaint defer_first_meaningful_paint_ = kDoNotDefer;
  TaskRunnerTimer<FirstMeaningfulPaintDetector> network0_quiet_timer_;
  TaskRunnerTimer<FirstMeaningfulPaintDetector> network2_quiet_timer_;
  DISALLOW_COPY_AND_ASSIGN(FirstMeaningfulPaintDetector);
};

}  // namespace blink

#endif
