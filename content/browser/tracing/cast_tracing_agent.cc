// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/tracing/cast_tracing_agent.h"

#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/logging.h"
#include "base/task_scheduler/task_scheduler.h"
#include "base/trace_event/trace_config.h"
#include "chromecast/tracing/system_tracing_common.h"
#include "content/public/browser/browser_thread.h"
#include "services/service_manager/public/cpp/connector.h"
#include "services/tracing/public/mojom/constants.mojom.h"

namespace content {
namespace {

std::string GetTracingCategories(
    const base::trace_event::TraceConfig& trace_config) {
  std::vector<base::StringPiece> categories;
  for (size_t i = 0; i < chromecast::tracing::kCategoryCount; ++i) {
    base::StringPiece category(chromecast::tracing::kCategories[i]);
    if (trace_config.category_filter().IsCategoryGroupEnabled(category))
      categories.push_back(category);
  }
  return base::JoinString(categories, ",");
}

std::string GetAllTracingCategories() {
  std::vector<base::StringPiece> categories;
  for (size_t i = 0; i < chromecast::tracing::kCategoryCount; ++i) {
    base::StringPiece category(chromecast::tracing::kCategories[i]);
    categories.push_back(category);
  }
  return base::JoinString(categories, ",");
}

}  // namespace

CastTracingAgent::CastTracingAgent(service_manager::Connector* connector)
    : BaseAgent(connector,
                "systemTraceEvents",
                tracing::mojom::TraceDataType::STRING,
                false /* supports_explicit_clock_sync */) {
  task_runner_ =
      base::TaskScheduler::GetInstance()->CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::BACKGROUND,
           base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN});
}

CastTracingAgent::~CastTracingAgent() = default;

// tracing::mojom::Agent. Called by Mojo internals on the UI thread.
void CastTracingAgent::StartTracing(const std::string& config,
                                    base::TimeTicks coordinator_time,
                                    Agent::StartTracingCallback callback) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  base::trace_event::TraceConfig trace_config(config);

  if (!trace_config.IsSystraceEnabled()) {
    std::move(callback).Run(false /* success */);
    return;
  }

  start_tracing_callback_ = std::move(callback);

  task_runner_->PostTask(FROM_HERE,
                         base::BindOnce(&CastTracingAgent::StartTracingOnIO,
                                        base::Unretained(this),
                                        base::ThreadTaskRunnerHandle::Get(),
                                        GetTracingCategories(trace_config)));
}

void CastTracingAgent::StopAndFlush(tracing::mojom::RecorderPtr recorder) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  recorder_ = std::move(recorder);

  task_runner_->PostTask(FROM_HERE,
                         base::BindOnce(&CastTracingAgent::StopAndFlushOnIO,
                                        base::Unretained(this),
                                        base::ThreadTaskRunnerHandle::Get()));
}

void CastTracingAgent::GetCategories(Agent::GetCategoriesCallback callback) {
  std::move(callback).Run(GetAllTracingCategories());
}

void CastTracingAgent::StartTracingOnIO(
    scoped_refptr<base::TaskRunner> reply_task_runner,
    const std::string& categories) {
  system_tracer_ = std::make_unique<chromecast::SystemTracer>();

  system_tracer_->StartTracing(
      categories, base::BindOnce(&CastTracingAgent::FinishStartOnIO,
                                 base::Unretained(this), reply_task_runner));
}

void CastTracingAgent::FinishStartOnIO(
    scoped_refptr<base::TaskRunner> reply_task_runner,
    chromecast::SystemTracer::Status status) {
  reply_task_runner->PostTask(FROM_HERE,
                              base::BindOnce(&CastTracingAgent::FinishStart,
                                             base::Unretained(this), status));
  if (status != chromecast::SystemTracer::Status::OK) {
    base::SequencedTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::BindOnce(&CastTracingAgent::CleanupOnIO, base::Unretained(this)));
  }
}

void CastTracingAgent::FinishStart(chromecast::SystemTracer::Status status) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  std::move(start_tracing_callback_)
      .Run(status == chromecast::SystemTracer::Status::OK);
}

void CastTracingAgent::StopAndFlushOnIO(
    scoped_refptr<base::TaskRunner> reply_task_runner) {
  system_tracer_->StopTracing(
      base::BindRepeating(&CastTracingAgent::HandleTraceDataOnIO,
                          base::Unretained(this), reply_task_runner));
}

void CastTracingAgent::HandleTraceDataOnIO(
    scoped_refptr<base::TaskRunner> reply_task_runner,
    chromecast::SystemTracer::Status status,
    std::string trace_data) {
  reply_task_runner->PostTask(
      FROM_HERE,
      base::BindOnce(&CastTracingAgent::HandleTraceData, base::Unretained(this),
                     status, std::move(trace_data)));
  if (status != chromecast::SystemTracer::Status::KEEP_GOING) {
    base::SequencedTaskRunnerHandle::Get()->PostTask(
        FROM_HERE,
        base::BindOnce(&CastTracingAgent::CleanupOnIO, base::Unretained(this)));
  }
}

void CastTracingAgent::HandleTraceData(chromecast::SystemTracer::Status status,
                                       std::string trace_data) {
  if (recorder_ && status != chromecast::SystemTracer::Status::FAIL)
    recorder_->AddChunk(std::move(trace_data));
  if (status != chromecast::SystemTracer::Status::KEEP_GOING)
    recorder_.reset();
}

void CastTracingAgent::CleanupOnIO() {
  system_tracer_.reset();
}

}  // namespace content
