// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_ANDROID_OOM_INTERVENTION_OOM_INTERVENTION_TAB_HELPER_H_
#define CHROME_BROWSER_ANDROID_OOM_INTERVENTION_OOM_INTERVENTION_TAB_HELPER_H_

#include "base/memory/unsafe_shared_memory_region.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "base/time/time.h"
#include "chrome/browser/android/oom_intervention/near_oom_monitor.h"
#include "chrome/browser/metrics/oom/out_of_memory_reporter.h"
#include "chrome/browser/ui/interventions/intervention_delegate.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "third_party/blink/public/platform/oom_intervention.mojom.h"

namespace content {
class WebContents;
}

class OomInterventionDecider;

// A tab helper for near-OOM intervention. This class depends on
// OutOfMemoryReporter. OutOfMemoryReporter must be created on TabHelpers
// before creating OomInterventionTabHelper.
class OomInterventionTabHelper
    : public content::WebContentsObserver,
      public content::WebContentsUserData<OomInterventionTabHelper>,
      public OutOfMemoryReporter::Observer,
      public blink::mojom::OomInterventionHost,
      public InterventionDelegate {
 public:
  static bool IsEnabled();

  ~OomInterventionTabHelper() override;

  // blink::mojom::OomInterventionHost:
  void OnHighMemoryUsage(bool intervention_triggered) override;

  // InterventionDelegate:
  void AcceptIntervention() override;
  void DeclineIntervention() override;
  void DeclineInterventionSticky() override;

 private:
  explicit OomInterventionTabHelper(content::WebContents* web_contents);

  friend class content::WebContentsUserData<OomInterventionTabHelper>;

  // content::WebContentsObserver:
  void WebContentsDestroyed() override;
  void RenderProcessGone(base::TerminationStatus status) override;
  void DidStartNavigation(
      content::NavigationHandle* navigation_handle) override;
  void DocumentAvailableInMainFrame() override;
  void OnVisibilityChanged(content::Visibility visibility) override;

  // OutOfMemoryReporter::Observer:
  void OnForegroundOOMDetected(const GURL& url,
                               ukm::SourceId source_id) override;

  // Starts observing near-OOM situation if it's not started.
  void StartMonitoringIfNeeded();
  // Stops observing near-OOM situation.
  void StopMonitoring();
  // Starts detecting near-OOM situation in renderer.
  void StartDetectionInRenderer();

  // Called when NearOomMonitor detects near-OOM situation.
  void OnNearOomDetected();

  // Called when we stop monitoring high memory usage in the foreground
  // renderer.
  void OnDetectionWindowElapsedWithoutHighMemoryUsage();

  void ResetInterventionState();

  void ResetInterfaces();

  bool navigation_started_ = false;
  base::Optional<base::TimeTicks> near_oom_detected_time_;
  std::unique_ptr<NearOomMonitor::Subscription> subscription_;
  base::OneShotTimer renderer_detection_timer_;

  // Not owned. This will be nullptr in incognito mode.
  OomInterventionDecider* decider_;

  blink::mojom::OomInterventionPtr intervention_;

  enum class InterventionState {
    // Intervention isn't triggered yet.
    NOT_TRIGGERED,
    // Intervention is triggered but the user doesn't respond yet.
    UI_SHOWN,
    // Intervention is triggered and the user declined it.
    DECLINED,
    // Intervention is triggered and the user accepted it.
    ACCEPTED,
  };

  InterventionState intervention_state_ = InterventionState::NOT_TRIGGERED;

  mojo::Binding<blink::mojom::OomInterventionHost> binding_;

  // The shared memory region that stores metrics written by the renderer
  // process. The memory is updated frequently and the browser should touch the
  // memory only after renderer process is dead.
  base::UnsafeSharedMemoryRegion shared_metrics_buffer_;
  base::WritableSharedMemoryMapping metrics_mapping_;

  // If memory workload in renderer is above this threshold, we assume that we
  // are in a near-OOM situation.
  uint64_t renderer_memory_workload_threshold_;

  base::WeakPtrFactory<OomInterventionTabHelper> weak_ptr_factory_;
};

#endif  // CHROME_BROWSER_ANDROID_OOM_INTERVENTION_OOM_INTERVENTION_TAB_HELPER_H_
