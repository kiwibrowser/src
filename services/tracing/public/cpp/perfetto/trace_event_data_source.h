// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_TRACING_PUBLIC_CPP_PERFETTO_TRACE_EVENT_DATA_SOURCE_H_
#define SERVICES_TRACING_PUBLIC_CPP_PERFETTO_TRACE_EVENT_DATA_SOURCE_H_

#include <memory>
#include <string>

#include "base/component_export.h"
#include "base/macros.h"
#include "base/threading/thread_local.h"
#include "services/tracing/public/cpp/perfetto/producer_client.h"

namespace tracing {

class ProducerClient;

// This class acts as a bridge between the TraceLog and
// the Perfetto ProducerClient. It converts incoming
// trace events to ChromeTraceEvent protos and writes
// them into the Perfetto shared memory.
class COMPONENT_EXPORT(TRACING_CPP) TraceEventDataSource {
 public:
  class ThreadLocalEventSink;

  static TraceEventDataSource* GetInstance();

  // Deletes the TraceWriter for the current thread, if any.
  static void ResetCurrentThreadForTesting();

  // The ProducerClient is responsible for calling RequestStop
  // which will clear the stored pointer to it, before it
  // gets destroyed. ProducerClient::CreateTraceWriter can be
  // called by the TraceEventDataSource on any thread.
  void StartTracing(ProducerClient* producer_client,
                    const mojom::DataSourceConfig& data_source_config);

  // Called from the ProducerClient.
  void StopTracing(
      base::RepeatingClosure stop_complete_callback = base::RepeatingClosure());

 private:
  friend class base::NoDestructor<TraceEventDataSource>;

  TraceEventDataSource();
  ~TraceEventDataSource();

  ThreadLocalEventSink* CreateThreadLocalEventSink();

  // Callback from TraceLog on any added trace events, can be called from
  // any thread.
  static void OnAddTraceEvent(const base::trace_event::TraceEvent& trace_event);

  base::Lock lock_;
  uint32_t target_buffer_ = 0;
  ProducerClient* producer_client_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(TraceEventDataSource);
};

}  // namespace tracing

#endif  // SERVICES_TRACING_PUBLIC_CPP_PERFETTO_TRACE_EVENT_DATA_SOURCE_H_
