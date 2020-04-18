// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_METRICS_RENDERER_UPTIME_TRACKER_H_
#define CHROME_BROWSER_METRICS_RENDERER_UPTIME_TRACKER_H_

#include <map>

#include "base/time/time.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"

namespace metrics {

// Class for tracking and renderer uptime info.
class RendererUptimeTracker : public content::NotificationObserver {
 public:
  // Creates the |RendererUptimeTracker| instance and initializes the
  // observers that notify to it.
  static void Initialize();

  // Returns the |RendererUptimeTracker| instance.
  static RendererUptimeTracker* Get();

  // Called when a load happens in a main frame.
  void OnLoadInMainFrame(int pid);

  // content::NotificationObserver:
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  // Retrieve the uptime for the given process |pid|.
  virtual base::TimeDelta GetProcessUptime(int pid);

 protected:
  // Exposed for unittest purpose.
  static RendererUptimeTracker* SetMockRendererUptimeTracker(
      RendererUptimeTracker* tracker);

  RendererUptimeTracker();
  ~RendererUptimeTracker() override;

 private:
  void OnRendererStarted(int pid);
  void OnRendererTerminated(int pid);

  // Object for registering notification requests.
  content::NotificationRegistrar registrar_;

  struct RendererInfo {
    base::TimeTicks launched_at_;
    int num_loads_in_main_frame_;
  };

  // Maps RenderProcessHost ID to its info on uptime.
  std::map<int, RendererInfo> info_map_;

  DISALLOW_COPY_AND_ASSIGN(RendererUptimeTracker);
};

}  // namespace metrics

#endif  // CHROME_BROWSER_METRICS_RENDERER_UPTIME_TRACKER_H_
