// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/tracing/public/cpp/trace_event_agent.h"

#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/memory/ref_counted.h"
#include "base/memory/ref_counted_memory.h"
#include "base/strings/string_util.h"
#include "base/threading/thread_checker.h"
#include "base/time/time.h"
#include "base/trace_event/trace_log.h"
#include "base/values.h"
#include "build/build_config.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/tracing/public/cpp/tracing_features.h"
#include "services/tracing/public/mojom/constants.mojom.h"

#if defined(OS_ANDROID) || defined(OS_LINUX) || defined(OS_MACOSX)
#include "services/tracing/public/cpp/perfetto/producer_client.h"
#endif

namespace {

const char kTraceEventLabel[] = "traceEvents";

}  // namespace

namespace tracing {

#if defined(OS_ANDROID) || defined(OS_LINUX) || defined(OS_MACOSX)
class PerfettoTraceEventAgent : public TraceEventAgent {
 public:
  explicit PerfettoTraceEventAgent(service_manager::Connector* connector) {
    mojom::PerfettoServicePtr perfetto_service;
    connector->BindInterface(mojom::kServiceName, &perfetto_service);

    producer_client_ = std::make_unique<ProducerClient>();
    producer_client_->CreateMojoMessagepipes(base::BindOnce(
        [](mojom::PerfettoServicePtr perfetto_service,
           mojom::ProducerClientPtr producer_client_pipe,
           mojom::ProducerHostRequest producer_host_pipe) {
          perfetto_service->ConnectToProducerHost(
              std::move(producer_client_pipe), std::move(producer_host_pipe));
        },
        std::move(perfetto_service)));
  }

  ~PerfettoTraceEventAgent() override {
    ProducerClient::DeleteSoon(std::move(producer_client_));
  }

 private:
  std::unique_ptr<ProducerClient> producer_client_;
};
#endif

// static
std::unique_ptr<TraceEventAgent> TraceEventAgent::Create(
    service_manager::Connector* connector,
    bool request_clock_sync_marker_on_android) {
  if (TracingUsesPerfettoBackend()) {
#if defined(OS_ANDROID) || defined(OS_LINUX) || defined(OS_MACOSX)
    return std::make_unique<PerfettoTraceEventAgent>(connector);
#else
    LOG(FATAL) << "Perfetto is not yet available for this platform.";
    return nullptr;
#endif
  } else {
    return std::make_unique<TraceEventAgentImpl>(
        connector, request_clock_sync_marker_on_android);
  }
}

TraceEventAgent::TraceEventAgent() = default;

TraceEventAgent::~TraceEventAgent() = default;

TraceEventAgentImpl::TraceEventAgentImpl(
    service_manager::Connector* connector,
    bool request_clock_sync_marker_on_android)
    : BaseAgent(connector,
                kTraceEventLabel,
                mojom::TraceDataType::ARRAY,
#if defined(OS_ANDROID)
                request_clock_sync_marker_on_android),
#else
                false),
#endif
      enabled_tracing_modes_(0) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
}

TraceEventAgentImpl::~TraceEventAgentImpl() {
  DCHECK(!trace_log_needs_me_);
}

void TraceEventAgentImpl::AddMetadataGeneratorFunction(
    MetadataGeneratorFunction generator) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  metadata_generator_functions_.push_back(generator);
}

void TraceEventAgentImpl::StartTracing(const std::string& config,
                                       base::TimeTicks coordinator_time,
                                       StartTracingCallback callback) {
  DCHECK(!recorder_);
#if defined(__native_client__)
  // NaCl and system times are offset by a bit, so subtract some time from
  // the captured timestamps. The value might be off by a bit due to messaging
  // latency.
  base::TimeDelta time_offset = TRACE_TIME_TICKS_NOW() - coordinator_time;
  TraceLog::GetInstance()->SetTimeOffset(time_offset);
#endif
  enabled_tracing_modes_ = base::trace_event::TraceLog::RECORDING_MODE;
  const base::trace_event::TraceConfig trace_config(config);
  if (!trace_config.event_filters().empty())
    enabled_tracing_modes_ |= base::trace_event::TraceLog::FILTERING_MODE;
  base::trace_event::TraceLog::GetInstance()->SetEnabled(
      trace_config, enabled_tracing_modes_);
  std::move(callback).Run(true);
}

void TraceEventAgentImpl::StopAndFlush(mojom::RecorderPtr recorder) {
  DCHECK(!recorder_);
  recorder_ = std::move(recorder);
  base::trace_event::TraceLog::GetInstance()->SetDisabled(
      enabled_tracing_modes_);
  enabled_tracing_modes_ = 0;
  for (const auto& generator : metadata_generator_functions_) {
    auto metadata = generator.Run();
    if (metadata)
      recorder_->AddMetadata(std::move(*metadata));
  }
  trace_log_needs_me_ = true;
  base::trace_event::TraceLog::GetInstance()->Flush(base::Bind(
      &TraceEventAgentImpl::OnTraceLogFlush, base::Unretained(this)));
}

void TraceEventAgentImpl::RequestClockSyncMarker(
    const std::string& sync_id,
    Agent::RequestClockSyncMarkerCallback callback) {
#if defined(OS_ANDROID)
  base::trace_event::TraceLog::GetInstance()->AddClockSyncMetadataEvent();
  std::move(callback).Run(base::TimeTicks(), base::TimeTicks());
#else
  NOTREACHED();
#endif
}

void TraceEventAgentImpl::RequestBufferStatus(
    RequestBufferStatusCallback callback) {
  base::trace_event::TraceLogStatus status =
      base::trace_event::TraceLog::GetInstance()->GetStatus();
  std::move(callback).Run(status.event_capacity, status.event_count);
}

void TraceEventAgentImpl::GetCategories(GetCategoriesCallback callback) {
  std::vector<std::string> category_vector;
  base::trace_event::TraceLog::GetInstance()->GetKnownCategoryGroups(
      &category_vector);
  std::move(callback).Run(base::JoinString(category_vector, ","));
}

void TraceEventAgentImpl::OnTraceLogFlush(
    const scoped_refptr<base::RefCountedString>& events_str,
    bool has_more_events) {
  if (!events_str->data().empty())
    recorder_->AddChunk(events_str->data());
  if (!has_more_events) {
    trace_log_needs_me_ = false;
    recorder_.reset();
  }
}

}  // namespace tracing
