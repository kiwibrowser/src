// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_TRACING_CAST_TRACING_AGENT_H_
#define CONTENT_BROWSER_TRACING_CAST_TRACING_AGENT_H_

#include <memory>
#include <string>

#include "chromecast/tracing/system_tracer.h"
#include "services/tracing/public/cpp/base_agent.h"
#include "services/tracing/public/mojom/tracing.mojom.h"

namespace service_manager {
class Connector;
}  // namespace service_manager

namespace chromecast {
class SystemTracer;
}

namespace content {

class CastTracingAgent : public tracing::BaseAgent {
 public:
  explicit CastTracingAgent(service_manager::Connector* connector);
  ~CastTracingAgent() override;

 private:
  // tracing::mojom::Agent. Called by Mojo internals on the UI thread.
  void StartTracing(const std::string& config,
                    base::TimeTicks coordinator_time,
                    Agent::StartTracingCallback callback) override;
  void StopAndFlush(tracing::mojom::RecorderPtr recorder) override;
  void GetCategories(Agent::GetCategoriesCallback callback) override;

  void StartTracingOnIO(scoped_refptr<base::TaskRunner> reply_task_runner,
                        const std::string& categories);
  void FinishStartOnIO(scoped_refptr<base::TaskRunner> reply_task_runner,
                       chromecast::SystemTracer::Status status);
  void FinishStart(chromecast::SystemTracer::Status status);
  void StopAndFlushOnIO(scoped_refptr<base::TaskRunner> reply_task_runner);
  void HandleTraceDataOnIO(scoped_refptr<base::TaskRunner> reply_task_runner,
                           chromecast::SystemTracer::Status,
                           std::string trace_data);
  void HandleTraceData(chromecast::SystemTracer::Status status,
                       std::string trace_data);
  void CleanupOnIO();

  Agent::StartTracingCallback start_tracing_callback_;
  tracing::mojom::RecorderPtr recorder_;

  // Task runner for collecting traces in a worker thread.
  scoped_refptr<base::SequencedTaskRunner> task_runner_;

  std::unique_ptr<chromecast::SystemTracer> system_tracer_;

  DISALLOW_COPY_AND_ASSIGN(CastTracingAgent);
};

}  // namespace content

#endif  // CONTENT_BROWSER_TRACING_CAST_TRACING_AGENT_H_
