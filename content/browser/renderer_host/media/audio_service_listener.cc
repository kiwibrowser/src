// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/media/audio_service_listener.h"

#include <utility>

#include "base/metrics/histogram_macros.h"
#include "base/time/default_tick_clock.h"
#include "content/public/browser/child_process_data.h"
#include "content/public/browser/child_process_termination_info.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "services/audio/public/mojom/constants.mojom.h"

namespace content {

AudioServiceListener::Metrics::Metrics(const base::TickClock* clock)
    : clock_(clock), initial_downtime_start_(clock_->NowTicks()) {}

AudioServiceListener::Metrics::~Metrics() = default;

void AudioServiceListener::Metrics::ServiceAlreadyRunning() {
  LogServiceStartStatus(ServiceStartStatus::kAlreadyStarted);
  started_ = clock_->NowTicks();
  initial_downtime_start_ = base::TimeTicks();
}

void AudioServiceListener::Metrics::ServiceCreated() {
  DCHECK(created_.is_null());
  created_ = clock_->NowTicks();
}

void AudioServiceListener::Metrics::ServiceStarted() {
  started_ = clock_->NowTicks();

  // |created_| is uninitialized if OnServiceCreated() was called before the
  // listener is initialized with OnInit() call.
  if (!created_.is_null()) {
    LogServiceStartStatus(ServiceStartStatus::kSuccess);
    UMA_HISTOGRAM_TIMES("Media.AudioService.ObservedStartupTime",
                        started_ - created_);
    created_ = base::TimeTicks();
  }

  if (!initial_downtime_start_.is_null()) {
    UMA_HISTOGRAM_CUSTOM_TIMES("Media.AudioService.ObservedInitialDowntime",
                               started_ - initial_downtime_start_,
                               base::TimeDelta(), base::TimeDelta::FromDays(7),
                               50);
    initial_downtime_start_ = base::TimeTicks();
  }

  if (!stopped_.is_null()) {
    UMA_HISTOGRAM_CUSTOM_TIMES("Media.AudioService.ObservedDowntime2",
                               started_ - stopped_, base::TimeDelta(),
                               base::TimeDelta::FromDays(7), 50);
    stopped_ = base::TimeTicks();
  }
}

void AudioServiceListener::Metrics::ServiceFailedToStart() {
  LogServiceStartStatus(ServiceStartStatus::kFailure);
  created_ = base::TimeTicks();
}

void AudioServiceListener::Metrics::ServiceStopped() {
  stopped_ = clock_->NowTicks();

  DCHECK(!started_.is_null());
  UMA_HISTOGRAM_CUSTOM_TIMES("Media.AudioService.ObservedUptime",
                             stopped_ - started_, base::TimeDelta(),
                             base::TimeDelta::FromDays(7), 50);
  started_ = base::TimeTicks();
}

void AudioServiceListener::Metrics::ServiceProcessTerminated(
    Metrics::ServiceProcessTerminationStatus status) {
  UMA_HISTOGRAM_ENUMERATION(
      "Media.AudioService.ObservedProcessTerminationStatus", status);
}

void AudioServiceListener::Metrics::LogServiceStartStatus(
    Metrics::ServiceStartStatus status) {
  UMA_HISTOGRAM_ENUMERATION("Media.AudioService.ObservedStartStatus", status);
}

AudioServiceListener::AudioServiceListener(
    std::unique_ptr<service_manager::Connector> connector)
    : binding_(this),
      metrics_(base::DefaultTickClock::GetInstance()) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(owning_sequence_);
  if (!connector)
    return;  // Happens in unittests.

  service_manager::mojom::ServiceManagerPtr service_manager;
  connector->BindInterface(service_manager::mojom::kServiceName,
                           &service_manager);
  service_manager::mojom::ServiceManagerListenerPtr listener;
  service_manager::mojom::ServiceManagerListenerRequest request(
      mojo::MakeRequest(&listener));
  service_manager->AddListener(std::move(listener));
  binding_.Bind(std::move(request));
  BrowserChildProcessObserver::Add(this);
}

AudioServiceListener::~AudioServiceListener() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(owning_sequence_);
  BrowserChildProcessObserver::Remove(this);
}

base::ProcessId AudioServiceListener::GetProcessId() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(owning_sequence_);
  return process_id_;
}

void AudioServiceListener::OnInit(
    std::vector<service_manager::mojom::RunningServiceInfoPtr>
        running_services) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(owning_sequence_);
  for (const service_manager::mojom::RunningServiceInfoPtr& instance :
       running_services) {
    if (instance->identity.name() == audio::mojom::kServiceName) {
      process_id_ = instance->pid;
      metrics_.ServiceAlreadyRunning();
      break;
    }
  }
}

void AudioServiceListener::OnServiceCreated(
    service_manager::mojom::RunningServiceInfoPtr service) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(owning_sequence_);
  if (service->identity.name() != audio::mojom::kServiceName)
    return;
  metrics_.ServiceCreated();
}

void AudioServiceListener::OnServiceStarted(
    const ::service_manager::Identity& identity,
    uint32_t pid) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(owning_sequence_);
  if (identity.name() != audio::mojom::kServiceName)
    return;
  process_id_ = pid;
  metrics_.ServiceStarted();
}

void AudioServiceListener::OnServicePIDReceived(
    const ::service_manager::Identity& identity,
    uint32_t pid) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(owning_sequence_);
  if (identity.name() != audio::mojom::kServiceName)
    return;
  process_id_ = pid;
}

void AudioServiceListener::OnServiceFailedToStart(
    const ::service_manager::Identity& identity) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(owning_sequence_);
  if (identity.name() != audio::mojom::kServiceName)
    return;
  metrics_.ServiceFailedToStart();
}

void AudioServiceListener::OnServiceStopped(
    const ::service_manager::Identity& identity) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(owning_sequence_);
  if (identity.name() != audio::mojom::kServiceName)
    return;
  metrics_.ServiceStopped();
}

void AudioServiceListener::BrowserChildProcessHostDisconnected(
    const ChildProcessData& data) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(owning_sequence_);
  if (base::GetProcId(data.handle) != process_id_)
    return;
  process_id_ = base::kNullProcessId;
  metrics_.ServiceProcessTerminated(
      Metrics::ServiceProcessTerminationStatus::kDisconnect);
}

void AudioServiceListener::BrowserChildProcessCrashed(
    const ChildProcessData& data,
    const ChildProcessTerminationInfo& info) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(owning_sequence_);
  if (base::GetProcId(data.handle) != process_id_)
    return;
  process_id_ = base::kNullProcessId;
  metrics_.ServiceProcessTerminated(
      Metrics::ServiceProcessTerminationStatus::kCrash);
}

void AudioServiceListener::BrowserChildProcessKilled(
    const ChildProcessData& data,
    const ChildProcessTerminationInfo& info) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(owning_sequence_);
  if (base::GetProcId(data.handle) != process_id_)
    return;
  process_id_ = base::kNullProcessId;
  metrics_.ServiceProcessTerminated(
      Metrics::ServiceProcessTerminationStatus::kKill);
}

}  // namespace content
