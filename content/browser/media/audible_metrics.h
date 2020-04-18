// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_MEDIA_AUDIBLE_METRICS_H_
#define CONTENT_BROWSER_MEDIA_AUDIBLE_METRICS_H_

#include <memory>
#include <set>

#include "base/time/tick_clock.h"
#include "content/common/content_export.h"

namespace content {

class WebContents;

// This class handles metrics regarding audible WebContents.
// It does register three different information:
// - how many WebContents are audible when a WebContents become audible.
// - how long multiple WebContents are audible at the same time.
// - for a browsing session, how often and how many WebContents get audible at
//   the same time.
class CONTENT_EXPORT AudibleMetrics {
 public:
  AudibleMetrics();
  ~AudibleMetrics();

  void UpdateAudibleWebContentsState(const WebContents* web_contents,
                                     bool audible);

  void SetClockForTest(const base::TickClock* test_clock);

 private:
  void AddAudibleWebContents(const WebContents* web_contents);
  void RemoveAudibleWebContents(const WebContents* web_contents);

  base::TimeTicks concurrent_web_contents_start_time_;
  size_t max_concurrent_audible_web_contents_in_session_;
  const base::TickClock* clock_;

  std::set<const WebContents*> audible_web_contents_;

  DISALLOW_COPY_AND_ASSIGN(AudibleMetrics);
};

}  // namespace content

#endif // CONTENT_BROWSER_MEDIA_AUDIBLE_METRICS_H_
