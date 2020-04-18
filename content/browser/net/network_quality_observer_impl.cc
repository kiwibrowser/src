// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/net/network_quality_observer_impl.h"

#include "base/metrics/histogram_macros.h"
#include "content/browser/renderer_host/render_process_host_impl.h"
#include "content/common/view_messages.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_types.h"
#include "content/public/browser/render_process_host.h"
#include "net/nqe/network_quality_estimator.h"

namespace {

// Returns true if the |current_value| is meaningfully different from the
// |past_value|. May be called with either RTT or throughput values to compare.
bool MetricChangedMeaningfully(int32_t past_value, int32_t current_value) {
  if ((past_value == net::nqe::internal::INVALID_RTT_THROUGHPUT) !=
      (current_value == net::nqe::internal::INVALID_RTT_THROUGHPUT)) {
    return true;
  }

  if (past_value == net::nqe::internal::INVALID_RTT_THROUGHPUT &&
      current_value == net::nqe::internal::INVALID_RTT_THROUGHPUT) {
    return false;
  }

  DCHECK_LE(0, past_value);
  DCHECK_LE(0, current_value);

  // Metric has changed meaningfully only if (i) the difference between the two
  // values exceed the threshold; and, (ii) the ratio of the values also exceeds
  // the threshold.
  static const int kMinDifferenceInMetrics = 100;
  static const float kMinRatio = 1.2f;

  if (std::abs(past_value - current_value) < kMinDifferenceInMetrics) {
    // The absolute change in the value is not sufficient enough.
    return false;
  }

  if (past_value < (kMinRatio * current_value) &&
      current_value < (kMinRatio * past_value)) {
    // The relative change in the value is not sufficient enough.
    return false;
  }

  return true;
}

}  // namespace

namespace content {

// UiThreadObserver observes the changes to the network quality on the UI
// thread, and notifies the renderers of the change in the network quality.
class NetworkQualityObserverImpl::UiThreadObserver
    : public content::NotificationObserver {
 public:
  UiThreadObserver()
      : last_notified_type_(net::EFFECTIVE_CONNECTION_TYPE_UNKNOWN) {}

  ~UiThreadObserver() override {
    if (!registrar_.IsRegistered(this, NOTIFICATION_RENDERER_PROCESS_CREATED,
                                 NotificationService::AllSources())) {
      return;
    }
    registrar_.Remove(this, NOTIFICATION_RENDERER_PROCESS_CREATED,
                      NotificationService::AllSources());
  }

  void InitOnUIThread() {
    DCHECK_CURRENTLY_ON(BrowserThread::UI);
    registrar_.Add(this, NOTIFICATION_RENDERER_PROCESS_CREATED,
                   NotificationService::AllSources());
  }

  void OnEffectiveConnectionTypeChanged(net::EffectiveConnectionType type) {
    DCHECK_CURRENTLY_ON(BrowserThread::UI);

    if (last_notified_type_ == type)
      return;

    last_notified_type_ = type;

    // Notify all the existing renderers of the change in the network quality.
    for (RenderProcessHost::iterator it(RenderProcessHost::AllHostsIterator());
         !it.IsAtEnd(); it.Advance()) {
      it.GetCurrentValue()->GetRendererInterface()->OnNetworkQualityChanged(
          last_notified_type_, last_notified_network_quality_.http_rtt(),
          last_notified_network_quality_.transport_rtt(),
          last_notified_network_quality_.downstream_throughput_kbps());
    }
  }

  void OnRTTOrThroughputEstimatesComputed(
      const net::nqe::internal::NetworkQuality& network_quality) {
    DCHECK_CURRENTLY_ON(BrowserThread::UI);

    last_notified_network_quality_ = network_quality;

    // Notify all the existing renderers of the change in the network quality.
    for (RenderProcessHost::iterator it(RenderProcessHost::AllHostsIterator());
         !it.IsAtEnd(); it.Advance()) {
      it.GetCurrentValue()->GetRendererInterface()->OnNetworkQualityChanged(
          last_notified_type_, last_notified_network_quality_.http_rtt(),
          last_notified_network_quality_.transport_rtt(),
          last_notified_network_quality_.downstream_throughput_kbps());
    }
  }

 private:
  // NotificationObserver implementation:
  void Observe(int type,
               const NotificationSource& source,
               const NotificationDetails& details) override {
    DCHECK_CURRENTLY_ON(BrowserThread::UI);
    DCHECK_EQ(NOTIFICATION_RENDERER_PROCESS_CREATED, type);

    RenderProcessHost* rph = Source<RenderProcessHost>(source).ptr();

    // Notify the newly created renderer of the current network quality.
    rph->GetRendererInterface()->OnNetworkQualityChanged(
        last_notified_type_, last_notified_network_quality_.http_rtt(),
        last_notified_network_quality_.transport_rtt(),
        last_notified_network_quality_.downstream_throughput_kbps());
  }

  content::NotificationRegistrar registrar_;

  // The network quality that was last sent to the renderers.
  net::EffectiveConnectionType last_notified_type_;
  net::nqe::internal::NetworkQuality last_notified_network_quality_;

  DISALLOW_COPY_AND_ASSIGN(UiThreadObserver);
};

NetworkQualityObserverImpl::NetworkQualityObserverImpl(
    net::NetworkQualityEstimator* network_quality_estimator)
    : network_quality_estimator_(network_quality_estimator),
      last_notified_type_(net::EFFECTIVE_CONNECTION_TYPE_UNKNOWN) {
  network_quality_estimator_->AddRTTAndThroughputEstimatesObserver(this);
  network_quality_estimator_->AddEffectiveConnectionTypeObserver(this);

  ui_thread_observer_ = std::make_unique<UiThreadObserver>();
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(&UiThreadObserver::InitOnUIThread,
                     base::Unretained(ui_thread_observer_.get())));
}

NetworkQualityObserverImpl::~NetworkQualityObserverImpl() {
  DCHECK(thread_checker_.CalledOnValidThread());
  network_quality_estimator_->RemoveRTTAndThroughputEstimatesObserver(this);
  network_quality_estimator_->RemoveEffectiveConnectionTypeObserver(this);

  DCHECK(ui_thread_observer_);

  // If possible, delete |ui_thread_observer_| on UI thread.
  UiThreadObserver* ui_thread_observer_ptr = ui_thread_observer_.release();
  bool posted = BrowserThread::DeleteSoon(BrowserThread::UI, FROM_HERE,
                                          ui_thread_observer_ptr);

  if (!posted)
    delete ui_thread_observer_ptr;
}

void NetworkQualityObserverImpl::OnEffectiveConnectionTypeChanged(
    net::EffectiveConnectionType type) {
  DCHECK(thread_checker_.CalledOnValidThread());

  if (last_notified_type_ == type)
    return;

  last_notified_type_ = type;

  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(&UiThreadObserver::OnEffectiveConnectionTypeChanged,
                     base::Unretained(ui_thread_observer_.get()),
                     last_notified_type_));
}

void NetworkQualityObserverImpl::OnRTTOrThroughputEstimatesComputed(
    base::TimeDelta http_rtt,
    base::TimeDelta transport_rtt,
    int32_t downstream_throughput_kbps) {
  DCHECK(thread_checker_.CalledOnValidThread());

  // Check if any of the network quality metrics changed meaningfully.
  bool http_rtt_changed = MetricChangedMeaningfully(
      last_notified_network_quality_.http_rtt().InMilliseconds(),
      http_rtt.InMilliseconds());

  bool transport_rtt_changed = MetricChangedMeaningfully(
      last_notified_network_quality_.transport_rtt().InMilliseconds(),
      transport_rtt.InMilliseconds());
  bool kbps_changed = MetricChangedMeaningfully(
      last_notified_network_quality_.downstream_throughput_kbps(),
      downstream_throughput_kbps);

  bool network_quality_meaningfully_changed =
      http_rtt_changed || transport_rtt_changed || kbps_changed;
  UMA_HISTOGRAM_BOOLEAN("NQE.ContentObserver.NetworkQualityMeaningfullyChanged",
                        network_quality_meaningfully_changed);

  if (!network_quality_meaningfully_changed) {
    // Return since none of the metrics changed meaningfully. This reduces
    // the number of notifications to the different renderers every time
    // the network quality is recomputed.
    return;
  }

  last_notified_network_quality_ = net::nqe::internal::NetworkQuality(
      http_rtt, transport_rtt, downstream_throughput_kbps);

  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(&UiThreadObserver::OnRTTOrThroughputEstimatesComputed,
                     base::Unretained(ui_thread_observer_.get()),
                     last_notified_network_quality_));
}

std::unique_ptr<net::RTTAndThroughputEstimatesObserver>
CreateNetworkQualityObserver(
    net::NetworkQualityEstimator* network_quality_estimator) {
  return std::make_unique<NetworkQualityObserverImpl>(
      network_quality_estimator);
}

}  // namespace content