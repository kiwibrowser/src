// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_LONG_TASK_DETECTOR_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_LONG_TASK_DETECTOR_H_

#include "third_party/blink/public/platform/web_thread.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/scheduler/base/task_time_observer.h"
#include "third_party/blink/renderer/platform/wtf/noncopyable.h"
#include "third_party/blink/renderer/platform/wtf/time.h"

namespace blink {

class PLATFORM_EXPORT LongTaskObserver : public GarbageCollectedMixin {
 public:
  virtual ~LongTaskObserver() = default;

  virtual void OnLongTaskDetected(TimeTicks start_time, TimeTicks end_time) = 0;
};

// LongTaskDetector detects tasks longer than kLongTaskThreshold and notifies
// observers. When it has non-zero LongTaskObserver, it adds itself as a
// TaskTimeObserver on the main thread and observes every task. When the number
// of LongTaskObservers drop to zero it automatically removes itself as a
// TaskTimeObserver.
class PLATFORM_EXPORT LongTaskDetector final
    : public GarbageCollectedFinalized<LongTaskDetector>,
      public base::sequence_manager::TaskTimeObserver {
  WTF_MAKE_NONCOPYABLE(LongTaskDetector);

 public:
  static LongTaskDetector& Instance();

  void RegisterObserver(LongTaskObserver*);
  void UnregisterObserver(LongTaskObserver*);

  void Trace(blink::Visitor*);

  static constexpr double kLongTaskThresholdSeconds = 0.05;

 private:
  LongTaskDetector();

  // scheduler::TaskTimeObserver implementation
  void WillProcessTask(double start_time) override {}
  void DidProcessTask(double start_time, double end_time) override;

  HeapHashSet<WeakMember<LongTaskObserver>> observers_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_LONG_TASK_DETECTOR_H_
