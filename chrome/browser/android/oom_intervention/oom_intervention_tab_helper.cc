// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/android/oom_intervention/oom_intervention_tab_helper.h"

#include "base/metrics/field_trial_params.h"
#include "base/metrics/histogram_macros.h"
#include "chrome/browser/android/oom_intervention/oom_intervention_decider.h"
#include "chrome/browser/ui/android/infobars/near_oom_infobar.h"
#include "chrome/common/chrome_features.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/web_contents.h"
#include "third_party/blink/common/oom_intervention/oom_intervention_types.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(OomInterventionTabHelper);

namespace {

constexpr base::TimeDelta kRendererHighMemoryUsageDetectionWindow =
    base::TimeDelta::FromSeconds(60);

content::WebContents* g_last_visible_web_contents = nullptr;

bool IsLastVisibleWebContents(content::WebContents* web_contents) {
  return web_contents == g_last_visible_web_contents;
}

void SetLastVisibleWebContents(content::WebContents* web_contents) {
  g_last_visible_web_contents = web_contents;
}

// These enums are associated with UMA. Values must be kept in sync with
// enums.xml and must not be renumbered/reused.
enum class NearOomDetectionEndReason {
  OOM_PROTECTED_CRASH = 0,
  RENDERER_GONE = 1,
  NAVIGATION = 2,
  COUNT,
};

void RecordNearOomDetectionEndReason(NearOomDetectionEndReason reason) {
  UMA_HISTOGRAM_ENUMERATION(
      "Memory.Experimental.OomIntervention.NearOomDetectionEndReason", reason,
      NearOomDetectionEndReason::COUNT);
}

void RecordInterventionUserDecision(bool accepted) {
  UMA_HISTOGRAM_BOOLEAN("Memory.Experimental.OomIntervention.UserDecision",
                        accepted);
}

void RecordInterventionStateOnCrash(bool accepted) {
  UMA_HISTOGRAM_BOOLEAN(
      "Memory.Experimental.OomIntervention.InterventionStateOnCrash", accepted);
}

// Field trial parameter names.
const char kRendererPauseParamName[] = "pause_renderer";
const char kShouldDetectInRenderer[] = "detect_in_renderer";
const char kRendererWorkloadThreshold[] = "renderer_workload_threshold";

bool RendererPauseIsEnabled() {
  static bool enabled = base::GetFieldTrialParamByFeatureAsBool(
      features::kOomIntervention, kRendererPauseParamName, false);
  return enabled;
}

bool ShouldDetectInRenderer() {
  static bool enabled = base::GetFieldTrialParamByFeatureAsBool(
      features::kOomIntervention, kShouldDetectInRenderer, true);
  return enabled;
}

uint64_t GetRendererMemoryWorkloadThreshold() {
  const uint64_t kDefaultMemoryWorkloadThreshold = 80 * 1024 * 1024;

  std::string threshold_str = base::GetFieldTrialParamValueByFeature(
      features::kOomIntervention, kRendererWorkloadThreshold);
  uint64_t threshold = 0;
  if (!base::StringToUint64(threshold_str, &threshold)) {
    return kDefaultMemoryWorkloadThreshold;
  }
  return threshold;
}

}  // namespace

// static
bool OomInterventionTabHelper::IsEnabled() {
  return NearOomMonitor::GetInstance() != nullptr;
}

OomInterventionTabHelper::OomInterventionTabHelper(
    content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      decider_(OomInterventionDecider::GetForBrowserContext(
          web_contents->GetBrowserContext())),
      binding_(this),
      renderer_memory_workload_threshold_(GetRendererMemoryWorkloadThreshold()),
      weak_ptr_factory_(this) {
  OutOfMemoryReporter::FromWebContents(web_contents)->AddObserver(this);
  shared_metrics_buffer_ = base::UnsafeSharedMemoryRegion::Create(
      sizeof(blink::OomInterventionMetrics));
  metrics_mapping_ = shared_metrics_buffer_.Map();
}

OomInterventionTabHelper::~OomInterventionTabHelper() = default;

void OomInterventionTabHelper::OnHighMemoryUsage(bool intervention_triggered) {
  if (intervention_triggered) {
    NearOomInfoBar::Show(web_contents(), this);
    intervention_state_ = InterventionState::UI_SHOWN;
  }
  near_oom_detected_time_ = base::TimeTicks::Now();
  renderer_detection_timer_.AbandonAndStop();
}

void OomInterventionTabHelper::AcceptIntervention() {
  RecordInterventionUserDecision(true);
  intervention_state_ = InterventionState::ACCEPTED;
}

void OomInterventionTabHelper::DeclineIntervention() {
  RecordInterventionUserDecision(false);
  ResetInterfaces();
  intervention_state_ = InterventionState::DECLINED;

  if (decider_) {
    DCHECK(!web_contents()->GetBrowserContext()->IsOffTheRecord());
    const std::string& host = web_contents()->GetVisibleURL().host();
    decider_->OnInterventionDeclined(host);
  }
}

void OomInterventionTabHelper::DeclineInterventionSticky() {
  NOTREACHED();
}

void OomInterventionTabHelper::WebContentsDestroyed() {
  OutOfMemoryReporter::FromWebContents(web_contents())->RemoveObserver(this);
  StopMonitoring();
}

void OomInterventionTabHelper::RenderProcessGone(
    base::TerminationStatus status) {
  ResetInterfaces();

  // Skip background process termination.
  if (!IsLastVisibleWebContents(web_contents())) {
    ResetInterventionState();
    return;
  }

  // OOM crash is handled in OnForegroundOOMDetected().
  if (status == base::TERMINATION_STATUS_OOM_PROTECTED)
    return;

  if (near_oom_detected_time_) {
    base::TimeDelta elapsed_time =
        base::TimeTicks::Now() - near_oom_detected_time_.value();
    UMA_HISTOGRAM_MEDIUM_TIMES(
        "Memory.Experimental.OomIntervention."
        "RendererGoneAfterDetectionTime",
        elapsed_time);
    ResetInterventionState();
  } else {
    RecordNearOomDetectionEndReason(NearOomDetectionEndReason::RENDERER_GONE);
  }
}

void OomInterventionTabHelper::DidStartNavigation(
    content::NavigationHandle* navigation_handle) {
  // Filter out sub-frame's navigation.
  if (!navigation_handle->IsInMainFrame())
    return;

  // Filter out the first navigation.
  if (!navigation_started_) {
    navigation_started_ = true;
    return;
  }

  // Filter out a navigation if the navigation happens without changing
  // document.
  if (navigation_handle->IsSameDocument())
    return;

  ResetInterfaces();

  // Filter out background navigation.
  if (!IsLastVisibleWebContents(navigation_handle->GetWebContents())) {
    ResetInterventionState();
    return;
  }

  if (near_oom_detected_time_) {
    // near-OOM was detected.
    base::TimeDelta elapsed_time =
        base::TimeTicks::Now() - near_oom_detected_time_.value();
    UMA_HISTOGRAM_MEDIUM_TIMES(
        "Memory.Experimental.OomIntervention."
        "NavigationAfterDetectionTime",
        elapsed_time);
    ResetInterventionState();
  } else {
    // Monitoring but near-OOM hasn't been detected.
    RecordNearOomDetectionEndReason(NearOomDetectionEndReason::NAVIGATION);
  }
}

void OomInterventionTabHelper::DocumentAvailableInMainFrame() {
  if (IsLastVisibleWebContents(web_contents()))
    StartMonitoringIfNeeded();
}

void OomInterventionTabHelper::OnVisibilityChanged(
    content::Visibility visibility) {
  if (visibility == content::Visibility::VISIBLE) {
    StartMonitoringIfNeeded();
    SetLastVisibleWebContents(web_contents());
  } else {
    StopMonitoring();
  }
}

void OomInterventionTabHelper::OnForegroundOOMDetected(
    const GURL& url,
    ukm::SourceId source_id) {
  DCHECK(IsLastVisibleWebContents(web_contents()));
  if (near_oom_detected_time_) {
    base::TimeDelta elapsed_time =
        base::TimeTicks::Now() - near_oom_detected_time_.value();
    UMA_HISTOGRAM_MEDIUM_TIMES(
        "Memory.Experimental.OomIntervention."
        "OomProtectedCrashAfterDetectionTime",
        elapsed_time);

    if (intervention_state_ != InterventionState::NOT_TRIGGERED) {
      // Consider UI_SHOWN as ACCEPTED because we already triggered the
      // intervention and the user didn't decline.
      bool accepted = intervention_state_ != InterventionState::DECLINED;
      RecordInterventionStateOnCrash(accepted);
    }
    ResetInterventionState();
  } else {
    RecordNearOomDetectionEndReason(
        NearOomDetectionEndReason::OOM_PROTECTED_CRASH);
  }

  blink::OomInterventionMetrics* metrics =
      static_cast<blink::OomInterventionMetrics*>(metrics_mapping_.memory());

  UMA_HISTOGRAM_MEMORY_LARGE_MB(
      "Memory.Experimental.OomIntervention.RendererPrivateMemoryFootprintAtOOM",
      metrics->current_private_footprint_kb / 1024);
  UMA_HISTOGRAM_MEMORY_MB(
      "Memory.Experimental.OomIntervention.RendererSwapFootprintAtOOM",
      metrics->current_swap_kb / 1024);
  UMA_HISTOGRAM_MEMORY_MB(
      "Memory.Experimental.OomIntervention.RendererBlinkUsageAtOOM",
      metrics->current_blink_usage_kb / 1024);

  if (decider_) {
    DCHECK(!web_contents()->GetBrowserContext()->IsOffTheRecord());
    const std::string& host = web_contents()->GetVisibleURL().host();
    decider_->OnOomDetected(host);
  }
}

void OomInterventionTabHelper::StartMonitoringIfNeeded() {
  if (subscription_)
    return;

  if (intervention_)
    return;

  if (near_oom_detected_time_)
    return;

  if (ShouldDetectInRenderer()) {
    if (binding_.is_bound())
      return;
    StartDetectionInRenderer();
  } else {
    subscription_ = NearOomMonitor::GetInstance()->RegisterCallback(
        base::BindRepeating(&OomInterventionTabHelper::OnNearOomDetected,
                            base::Unretained(this)));
  }
}

void OomInterventionTabHelper::StopMonitoring() {
  if (ShouldDetectInRenderer()) {
    ResetInterfaces();
  } else {
    subscription_.reset();
  }
}

void OomInterventionTabHelper::StartDetectionInRenderer() {
  bool trigger_intervention = RendererPauseIsEnabled();
  if (trigger_intervention && decider_) {
    DCHECK(!web_contents()->GetBrowserContext()->IsOffTheRecord());
    const std::string& host = web_contents()->GetVisibleURL().host();
    trigger_intervention = decider_->CanTriggerIntervention(host);
  }

  content::RenderFrameHost* main_frame = web_contents()->GetMainFrame();
  DCHECK(main_frame);
  content::RenderProcessHost* render_process_host = main_frame->GetProcess();
  DCHECK(render_process_host);
  content::BindInterface(render_process_host,
                         mojo::MakeRequest(&intervention_));
  DCHECK(!binding_.is_bound());
  blink::mojom::OomInterventionHostPtr host;
  binding_.Bind(mojo::MakeRequest(&host));
  intervention_->StartDetection(
      std::move(host), shared_metrics_buffer_.Duplicate(),
      renderer_memory_workload_threshold_, trigger_intervention);
}

void OomInterventionTabHelper::OnNearOomDetected() {
  DCHECK(!ShouldDetectInRenderer());
  DCHECK_EQ(web_contents()->GetVisibility(), content::Visibility::VISIBLE);
  DCHECK(!near_oom_detected_time_);
  subscription_.reset();

  StartDetectionInRenderer();
  DCHECK(!renderer_detection_timer_.IsRunning());
  renderer_detection_timer_.Start(
      FROM_HERE, kRendererHighMemoryUsageDetectionWindow,
      base::BindRepeating(&OomInterventionTabHelper::
                              OnDetectionWindowElapsedWithoutHighMemoryUsage,
                          weak_ptr_factory_.GetWeakPtr()));
}

void OomInterventionTabHelper::
    OnDetectionWindowElapsedWithoutHighMemoryUsage() {
  ResetInterventionState();
  ResetInterfaces();
  StartMonitoringIfNeeded();
}

void OomInterventionTabHelper::ResetInterventionState() {
  near_oom_detected_time_.reset();
  intervention_state_ = InterventionState::NOT_TRIGGERED;
  renderer_detection_timer_.AbandonAndStop();
}

void OomInterventionTabHelper::ResetInterfaces() {
  intervention_.reset();
  if (binding_.is_bound())
    binding_.Close();
}
