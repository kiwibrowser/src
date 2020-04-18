// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/tracing/perfetto/perfetto_tracing_coordinator.h"

#include <utility>

#include "mojo/public/cpp/system/data_pipe_utils.h"
#include "services/tracing/perfetto/json_trace_exporter.h"
#include "services/tracing/perfetto/perfetto_service.h"

namespace tracing {

// A TracingSession is used to wrap all the associated
// state of an on-going tracing session, for easy setup
// and cleanup.
class PerfettoTracingCoordinator::TracingSession {
 public:
  TracingSession(const std::string& config,
                 base::OnceClosure tracing_over_callback)
      : tracing_over_callback_(std::move(tracing_over_callback)) {
    json_trace_exporter_ = std::make_unique<JSONTraceExporter>(
        config, PerfettoService::GetInstance()->GetService());
  }

  ~TracingSession() {
    if (!stop_and_flush_callback_.is_null()) {
      base::ResetAndReturn(&stop_and_flush_callback_)
          .Run(base::Value(base::Value::Type::DICTIONARY));
    }

    stream_.reset();
  }

  void StopAndFlush(mojo::ScopedDataPipeProducerHandle stream,
                    StopAndFlushCallback callback) {
    stream_ = std::move(stream);
    stop_and_flush_callback_ = std::move(callback);

    json_trace_exporter_->StopAndFlush(base::BindRepeating(
        &TracingSession::OnJSONTraceEventCallback, base::Unretained(this)));
  }

  void OnJSONTraceEventCallback(const std::string& json, bool has_more) {
    if (stream_.is_valid()) {
      mojo::BlockingCopyFromString(json, stream_);
    }

    if (!has_more) {
      DCHECK(!stop_and_flush_callback_.is_null());
      base::ResetAndReturn(&stop_and_flush_callback_)
          .Run(/*metadata=*/base::Value(base::Value::Type::DICTIONARY));

      base::SequencedTaskRunnerHandle::Get()->PostTask(
          FROM_HERE, std::move(tracing_over_callback_));
    }
  }

 private:
  mojo::ScopedDataPipeProducerHandle stream_;
  std::unique_ptr<JSONTraceExporter> json_trace_exporter_;
  StopAndFlushCallback stop_and_flush_callback_;
  base::OnceClosure tracing_over_callback_;

  DISALLOW_COPY_AND_ASSIGN(TracingSession);
};

// static
void PerfettoTracingCoordinator::DestroyOnSequence(
    std::unique_ptr<PerfettoTracingCoordinator> coordinator) {
  PerfettoService::GetInstance()->task_runner()->DeleteSoon(
      FROM_HERE, std::move(coordinator));
}

PerfettoTracingCoordinator::PerfettoTracingCoordinator()
    : binding_(this), weak_factory_(this) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

PerfettoTracingCoordinator::~PerfettoTracingCoordinator() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

void PerfettoTracingCoordinator::OnClientConnectionError() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  tracing_session_.reset();
}

void PerfettoTracingCoordinator::BindCoordinatorRequest(
    mojom::CoordinatorRequest request,
    const service_manager::BindSourceInfo& source_info) {
  PerfettoService::GetInstance()->task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&PerfettoTracingCoordinator::BindOnSequence,
                                base::Unretained(this), std::move(request)));
}

void PerfettoTracingCoordinator::BindOnSequence(
    mojom::CoordinatorRequest request) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  binding_.Bind(std::move(request));
  binding_.set_connection_error_handler(
      base::BindOnce(&PerfettoTracingCoordinator::OnClientConnectionError,
                     base::Unretained(this)));
}

void PerfettoTracingCoordinator::StartTracing(const std::string& config,
                                              StartTracingCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  tracing_session_ = std::make_unique<TracingSession>(
      config, base::BindOnce(&PerfettoTracingCoordinator::OnTracingOverCallback,
                             weak_factory_.GetWeakPtr()));
  std::move(callback).Run(true);
}

void PerfettoTracingCoordinator::OnTracingOverCallback() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(tracing_session_);
  tracing_session_.reset();
}

void PerfettoTracingCoordinator::StopAndFlush(
    mojo::ScopedDataPipeProducerHandle stream,
    StopAndFlushCallback callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK(tracing_session_);
  tracing_session_->StopAndFlush(std::move(stream), std::move(callback));
}

void PerfettoTracingCoordinator::StopAndFlushAgent(
    mojo::ScopedDataPipeProducerHandle stream,
    const std::string& agent_label,
    StopAndFlushCallback callback) {
  NOTREACHED();
}

void PerfettoTracingCoordinator::IsTracing(IsTracingCallback callback) {
  std::move(callback).Run(tracing_session_ != nullptr);
}

void PerfettoTracingCoordinator::RequestBufferUsage(
    RequestBufferUsageCallback callback) {
  std::move(callback).Run(false /* success */, 0.0f /* percent_full */,
                          0 /* approximate_count */);
}

// TODO(oysteine): Old tracing and Perfetto need to both be active for
// about://tracing to enumerate categories.
void PerfettoTracingCoordinator::GetCategories(GetCategoriesCallback callback) {
  std::move(callback).Run(false /* success */, "" /* categories_list */);
}

}  // namespace tracing
