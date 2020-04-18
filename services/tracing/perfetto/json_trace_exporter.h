// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_TRACING_PERFETTO_JSON_TRACE_EXPORTER_H_
#define SERVICES_TRACING_PERFETTO_JSON_TRACE_EXPORTER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"

#include "third_party/perfetto/include/perfetto/tracing/core/consumer.h"
#include "third_party/perfetto/include/perfetto/tracing/core/service.h"

namespace tracing {

// This is a Perfetto consumer which will enable Perfetto tracing
// and subscribe to ChromeTraceEvent data sources. Any received
// protos will be converted to the legacy JSON Chrome Tracing
// format.
class JSONTraceExporter : public perfetto::Consumer {
 public:
  // The owner of JSONTraceExporter should make sure to destroy
  // |service| before destroying this.
  JSONTraceExporter(const std::string& config, perfetto::Service* service);

  ~JSONTraceExporter() override;

  using OnTraceEventJSONCallback =
      base::RepeatingCallback<void(const std::string& json, bool has_more)>;
  void StopAndFlush(OnTraceEventJSONCallback callback);

  // perfetto::Consumer implementation.
  // This gets called by the Perfetto service as control signals,
  // and to send finished protobufs over.
  void OnConnect() override;
  void OnDisconnect() override;
  void OnTracingDisabled() override{};
  void OnTraceData(std::vector<perfetto::TracePacket> packets,
                   bool has_more) override;

 private:
  OnTraceEventJSONCallback json_callback_;
  bool has_output_json_preamble_ = false;
  bool has_output_first_event_ = false;
  std::string config_;

  // Keep last to avoid edge-cases where its callbacks come in mid-destruction.
  std::unique_ptr<perfetto::Service::ConsumerEndpoint> consumer_endpoint_;
  DISALLOW_COPY_AND_ASSIGN(JSONTraceExporter);
};

}  // namespace tracing

#endif  // SERVICES_TRACING_PERFETTO_JSON_TRACE_EXPORTER_H_
