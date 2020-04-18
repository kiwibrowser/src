// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/page_load_metrics/observers/android_page_load_metrics_observer.h"

#include <string>

#include "base/android/jni_android.h"
#include "base/android/jni_string.h"
#include "base/android/scoped_java_ref.h"
#include "base/time/time.h"
#include "chrome/browser/net/nqe/ui_network_quality_estimator_service.h"
#include "chrome/browser/net/nqe/ui_network_quality_estimator_service_factory.h"
#include "chrome/browser/page_load_metrics/page_load_metrics_util.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_contents.h"
#include "jni/PageLoadMetrics_jni.h"
#include "url/gurl.h"

AndroidPageLoadMetricsObserver::AndroidPageLoadMetricsObserver(
    content::WebContents* web_contents)
    : web_contents_(web_contents) {
  Profile* profile =
      Profile::FromBrowserContext(web_contents->GetBrowserContext());
  if (profile) {
    network_quality_provider_ =
        UINetworkQualityEstimatorServiceFactory::GetForProfile(profile);
  }
}

AndroidPageLoadMetricsObserver::ObservePolicy
AndroidPageLoadMetricsObserver::OnStart(
    content::NavigationHandle* navigation_handle,
    const GURL& currently_committed_url,
    bool started_in_foreground) {
  navigation_id_ = navigation_handle->GetNavigationId();
  ReportNewNavigation();
  if (network_quality_provider_) {
    int64_t http_rtt =
        network_quality_provider_->GetHttpRTT().has_value()
            ? network_quality_provider_->GetHttpRTT()->InMilliseconds()
            : 0;
    int64_t transport_rtt =
        network_quality_provider_->GetTransportRTT().has_value()
            ? network_quality_provider_->GetTransportRTT()->InMilliseconds()
            : 0;
    ReportNetworkQualityEstimate(
        network_quality_provider_->GetEffectiveConnectionType(), http_rtt,
        transport_rtt);
  } else {
    ReportNetworkQualityEstimate(net::EFFECTIVE_CONNECTION_TYPE_UNKNOWN, 0, 0);
  }
  return CONTINUE_OBSERVING;
}

void AndroidPageLoadMetricsObserver::OnFirstContentfulPaintInPage(
    const page_load_metrics::mojom::PageLoadTiming& timing,
    const page_load_metrics::PageLoadExtraInfo& extra_info) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  int64_t first_contentful_paint_ms =
      timing.paint_timing->first_contentful_paint->InMilliseconds();
  ReportFirstContentfulPaint(
      (extra_info.navigation_start - base::TimeTicks()).InMicroseconds(),
      first_contentful_paint_ms);
}

void AndroidPageLoadMetricsObserver::OnFirstMeaningfulPaintInMainFrameDocument(
    const page_load_metrics::mojom::PageLoadTiming& timing,
    const page_load_metrics::PageLoadExtraInfo& extra_info) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  int64_t first_meaningful_paint_ms =
      timing.paint_timing->first_meaningful_paint->InMilliseconds();
  ReportFirstMeaningfulPaint(
      (extra_info.navigation_start - base::TimeTicks()).InMicroseconds(),
      first_meaningful_paint_ms);
}

void AndroidPageLoadMetricsObserver::OnLoadEventStart(
    const page_load_metrics::mojom::PageLoadTiming& timing,
    const page_load_metrics::PageLoadExtraInfo& info) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  int64_t load_event_start_ms =
      timing.document_timing->load_event_start->InMilliseconds();
  ReportLoadEventStart(
      (info.navigation_start - base::TimeTicks()).InMicroseconds(),
      load_event_start_ms);
}

void AndroidPageLoadMetricsObserver::OnLoadedResource(
    const page_load_metrics::ExtraRequestCompleteInfo&
        extra_request_complete_info) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  if (extra_request_complete_info.resource_type ==
      content::RESOURCE_TYPE_MAIN_FRAME) {
    DCHECK(!did_dispatch_on_main_resource_);
    if (did_dispatch_on_main_resource_) {
      // We are defensive for the case of something strange happening and return
      // in order not to post multiple times.
      return;
    }
    did_dispatch_on_main_resource_ = true;

    const net::LoadTimingInfo& timing =
        *extra_request_complete_info.load_timing_info;
    int64_t dns_start =
        timing.connect_timing.dns_start.since_origin().InMilliseconds();
    int64_t dns_end =
        timing.connect_timing.dns_end.since_origin().InMilliseconds();
    int64_t connect_start =
        timing.connect_timing.connect_start.since_origin().InMilliseconds();
    int64_t connect_end =
        timing.connect_timing.connect_end.since_origin().InMilliseconds();
    int64_t request_start =
        timing.request_start.since_origin().InMilliseconds();
    int64_t send_start = timing.send_start.since_origin().InMilliseconds();
    int64_t send_end = timing.send_end.since_origin().InMilliseconds();
    ReportLoadedMainResource(dns_start, dns_end, connect_start, connect_end,
                             request_start, send_start, send_end);
  }
}

void AndroidPageLoadMetricsObserver::ReportNewNavigation() {
  DCHECK_GE(navigation_id_, 0);
  base::android::ScopedJavaLocalRef<jobject> java_web_contents =
      web_contents_->GetJavaWebContents();
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_PageLoadMetrics_onNewNavigation(env, java_web_contents,
                                       static_cast<jlong>(navigation_id_));
}

void AndroidPageLoadMetricsObserver::ReportNetworkQualityEstimate(
    net::EffectiveConnectionType connection_type,
    int64_t http_rtt_ms,
    int64_t transport_rtt_ms) {
  base::android::ScopedJavaLocalRef<jobject> java_web_contents =
      web_contents_->GetJavaWebContents();
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_PageLoadMetrics_onNetworkQualityEstimate(
      env, java_web_contents, static_cast<jlong>(navigation_id_),
      static_cast<jint>(connection_type), static_cast<jlong>(http_rtt_ms),
      static_cast<jlong>(transport_rtt_ms));
}

void AndroidPageLoadMetricsObserver::ReportFirstContentfulPaint(
    int64_t navigation_start_tick,
    int64_t first_contentful_paint_ms) {
  base::android::ScopedJavaLocalRef<jobject> java_web_contents =
      web_contents_->GetJavaWebContents();
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_PageLoadMetrics_onFirstContentfulPaint(
      env, java_web_contents, static_cast<jlong>(navigation_id_),
      static_cast<jlong>(navigation_start_tick),
      static_cast<jlong>(first_contentful_paint_ms));
}

void AndroidPageLoadMetricsObserver::ReportFirstMeaningfulPaint(
    int64_t navigation_start_tick,
    int64_t first_meaningful_paint_ms) {
  base::android::ScopedJavaLocalRef<jobject> java_web_contents =
      web_contents_->GetJavaWebContents();
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_PageLoadMetrics_onFirstMeaningfulPaint(
      env, java_web_contents, static_cast<jlong>(navigation_id_),
      static_cast<jlong>(navigation_start_tick),
      static_cast<jlong>(first_meaningful_paint_ms));
}

void AndroidPageLoadMetricsObserver::ReportLoadEventStart(
    int64_t navigation_start_tick,
    int64_t load_event_start_ms) {
  base::android::ScopedJavaLocalRef<jobject> java_web_contents =
      web_contents_->GetJavaWebContents();
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_PageLoadMetrics_onLoadEventStart(
      env, java_web_contents, static_cast<jlong>(navigation_id_),
      static_cast<jlong>(navigation_start_tick),
      static_cast<jlong>(load_event_start_ms));
}

void AndroidPageLoadMetricsObserver::ReportLoadedMainResource(
    int64_t dns_start_ms,
    int64_t dns_end_ms,
    int64_t connect_start_ms,
    int64_t connect_end_ms,
    int64_t request_start_ms,
    int64_t send_start_ms,
    int64_t send_end_ms) {
  base::android::ScopedJavaLocalRef<jobject> java_web_contents =
      web_contents_->GetJavaWebContents();
  JNIEnv* env = base::android::AttachCurrentThread();
  Java_PageLoadMetrics_onLoadedMainResource(
      env, java_web_contents, static_cast<jlong>(navigation_id_),
      static_cast<jlong>(dns_start_ms), static_cast<jlong>(dns_end_ms),
      static_cast<jlong>(connect_start_ms), static_cast<jlong>(connect_end_ms),
      static_cast<jlong>(request_start_ms), static_cast<jlong>(send_start_ms),
      static_cast<jlong>(send_end_ms));
}
