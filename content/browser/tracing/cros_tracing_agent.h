// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_TRACING_CROS_TRACING_AGENT_H_
#define CONTENT_BROWSER_TRACING_CROS_TRACING_AGENT_H_

#include <memory>
#include <string>

#include "services/tracing/public/cpp/base_agent.h"
#include "services/tracing/public/mojom/tracing.mojom.h"

namespace base {
class RefCountedString;
}  // namespace base

namespace chromeos {
class DebugDaemonClient;
}  // namespace chromeos

namespace service_manager {
class Connector;
}  // namespace service_manager

namespace content {

class CrOSTracingAgent : public tracing::BaseAgent {
 public:
  explicit CrOSTracingAgent(service_manager::Connector* connector);

 private:
  friend std::default_delete<CrOSTracingAgent>;

  ~CrOSTracingAgent() override;

  // tracing::mojom::Agent. Called by Mojo internals on the UI thread.
  void StartTracing(const std::string& config,
                    base::TimeTicks coordinator_time,
                    Agent::StartTracingCallback callback) override;
  void StopAndFlush(tracing::mojom::RecorderPtr recorder) override;

  void StartTracingCallbackProxy(Agent::StartTracingCallback callback,
                                 const std::string& agent_name,
                                 bool success);
  void RecorderProxy(const std::string& event_name,
                     const std::string& events_label,
                     const scoped_refptr<base::RefCountedString>& events);

  chromeos::DebugDaemonClient* debug_daemon_ = nullptr;
  tracing::mojom::RecorderPtr recorder_;

  DISALLOW_COPY_AND_ASSIGN(CrOSTracingAgent);
};

}  // namespace content

#endif  // CONTENT_BROWSER_TRACING_CROS_TRACING_AGENT_H_
