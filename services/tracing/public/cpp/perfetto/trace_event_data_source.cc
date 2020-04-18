// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/tracing/public/cpp/perfetto/trace_event_data_source.h"

#include <utility>

#include "base/no_destructor.h"
#include "base/process/process_handle.h"
#include "base/trace_event/trace_event.h"
#include "services/tracing/public/mojom/constants.mojom.h"
#include "third_party/perfetto/include/perfetto/tracing/core/shared_memory_arbiter.h"
#include "third_party/perfetto/include/perfetto/tracing/core/trace_writer.h"
#include "third_party/perfetto/protos/perfetto/trace/chrome/chrome_trace_event.pbzero.h"
#include "third_party/perfetto/protos/perfetto/trace/trace_packet.pbzero.h"

using TraceLog = base::trace_event::TraceLog;
using TraceEvent = base::trace_event::TraceEvent;
using TraceConfig = base::trace_event::TraceConfig;

namespace tracing {

class TraceEventDataSource::ThreadLocalEventSink {
 public:
  explicit ThreadLocalEventSink(
      std::unique_ptr<perfetto::TraceWriter> trace_writer)
      : trace_writer_(std::move(trace_writer)) {}

  ~ThreadLocalEventSink() {
    // Delete the TraceWriter on the sequence that Perfetto runs on, needed
    // as the ThreadLocalEventSink gets deleted on thread
    // shutdown and we can't safely call TaskRunnerHandle::Get() at that point
    // (which can happen as the TraceWriter destructor might make a Mojo call
    // and trigger it).
    ProducerClient::GetTaskRunner()->DeleteSoon(FROM_HERE,
                                                std::move(trace_writer_));
  }

  void AddTraceEvent(const TraceEvent& trace_event) {
    // TODO(oysteine): Adding trace events to Perfetto will
    // stall in some situations, specifically when we overflow
    // the buffer and need to make a sync call to flush it, and we're
    // running on the same thread as the service. The short-term fix (while
    // we're behind a flag) is to run the service on its own thread, the longer
    // term fix is most likely to not go via Mojo in that specific case.

    // TODO(oysteine): Temporary workaround for a specific trace event
    // which is added while a scheduler lock is held, and will deadlock
    // if Perfetto does a PostTask to commit a finished chunk.
    if (strcmp(trace_event.name(), "RealTimeDomain::DelayTillNextTask") == 0) {
      return;
    }

    // TODO(oysteine): Consider batching several trace events per trace packet,
    // and only add repeated data once per batch.
    auto trace_packet = trace_writer_->NewTracePacket();
    protozero::MessageHandle<perfetto::protos::pbzero::ChromeEventBundle>
        event_bundle(trace_packet->set_chrome_events());

    auto* new_trace_event = event_bundle->add_trace_events();
    new_trace_event->set_name(trace_event.name());

    new_trace_event->set_timestamp(
        trace_event.timestamp().since_origin().InMicroseconds());

    uint32_t flags = trace_event.flags();
    new_trace_event->set_flags(flags);

    int process_id;
    int thread_id;
    if ((flags & TRACE_EVENT_FLAG_HAS_PROCESS_ID) &&
        trace_event.thread_id() != base::kNullProcessId) {
      process_id = trace_event.thread_id();
      thread_id = -1;
    } else {
      process_id = TraceLog::GetInstance()->process_id();
      thread_id = trace_event.thread_id();
    }

    new_trace_event->set_process_id(process_id);
    new_trace_event->set_thread_id(thread_id);

    new_trace_event->set_category_group_name(
        TraceLog::GetCategoryGroupName(trace_event.category_group_enabled()));

    char phase = trace_event.phase();
    new_trace_event->set_phase(phase);

    for (int i = 0;
         i < base::trace_event::kTraceMaxNumArgs && trace_event.arg_name(i);
         ++i) {
      // TODO(oysteine): Support ConvertableToTraceFormat serialized into a JSON
      // string.
      auto type = trace_event.arg_type(i);
      if (type == TRACE_VALUE_TYPE_CONVERTABLE) {
        continue;
      }

      auto* new_arg = new_trace_event->add_args();
      new_arg->set_name(trace_event.arg_name(i));

      auto& value = trace_event.arg_value(i);
      switch (type) {
        case TRACE_VALUE_TYPE_BOOL:
          new_arg->set_bool_value(value.as_bool);
          break;
        case TRACE_VALUE_TYPE_UINT:
          new_arg->set_uint_value(value.as_uint);
          break;
        case TRACE_VALUE_TYPE_INT:
          new_arg->set_int_value(value.as_int);
          break;
        case TRACE_VALUE_TYPE_DOUBLE:
          new_arg->set_double_value(value.as_double);
          break;
        case TRACE_VALUE_TYPE_POINTER:
          new_arg->set_pointer_value(static_cast<uint64_t>(
              reinterpret_cast<uintptr_t>(value.as_pointer)));
          break;
        case TRACE_VALUE_TYPE_STRING:
        case TRACE_VALUE_TYPE_COPY_STRING:
          new_arg->set_string_value(value.as_string ? value.as_string : "NULL");
          break;
        default:
          NOTREACHED() << "Don't know how to print this value";
          break;
      }
    }

    if (phase == TRACE_EVENT_PHASE_COMPLETE) {
      int64_t duration = trace_event.duration().InMicroseconds();
      if (duration != -1) {
        new_trace_event->set_duration(duration);
      } else {
        // TODO(oysteine): Workaround until TRACE_EVENT_PHASE_COMPLETE can be
        // split into begin/end pairs. If the duration is -1 and the
        // trace-viewer will spend forever generating a warning for each event.
        new_trace_event->set_duration(0);
      }

      if (!trace_event.thread_timestamp().is_null()) {
        int64_t thread_duration =
            trace_event.thread_duration().InMicroseconds();
        if (thread_duration != -1) {
          new_trace_event->set_thread_duration(thread_duration);
        }
      }
    }

    if (!trace_event.thread_timestamp().is_null()) {
      int64_t thread_time_int64 =
          trace_event.thread_timestamp().since_origin().InMicroseconds();
      new_trace_event->set_thread_timestamp(thread_time_int64);
    }

    if (trace_event.scope() != trace_event_internal::kGlobalScope) {
      new_trace_event->set_scope(trace_event.scope());
    }

    if (flags & (TRACE_EVENT_FLAG_HAS_ID | TRACE_EVENT_FLAG_HAS_LOCAL_ID |
                 TRACE_EVENT_FLAG_HAS_GLOBAL_ID)) {
      new_trace_event->set_id(trace_event.id());
    }

    if ((flags & TRACE_EVENT_FLAG_FLOW_OUT) ||
        (flags & TRACE_EVENT_FLAG_FLOW_IN)) {
      new_trace_event->set_bind_id(trace_event.bind_id());
    }
  }

 private:
  std::unique_ptr<perfetto::TraceWriter> trace_writer_;
};

namespace {

base::ThreadLocalStorage::Slot* ThreadLocalEventSinkSlot() {
  static base::NoDestructor<base::ThreadLocalStorage::Slot>
      thread_local_event_sink_tls([](void* event_sink) {
        delete static_cast<TraceEventDataSource::ThreadLocalEventSink*>(
            event_sink);
      });

  return thread_local_event_sink_tls.get();
}

}  // namespace

// static
TraceEventDataSource* TraceEventDataSource::GetInstance() {
  static base::NoDestructor<TraceEventDataSource> instance;
  return instance.get();
}

TraceEventDataSource::TraceEventDataSource() = default;

TraceEventDataSource::~TraceEventDataSource() = default;

void TraceEventDataSource::StartTracing(
    ProducerClient* producer_client,
    const mojom::DataSourceConfig& data_source_config) {
  base::AutoLock lock(lock_);

  DCHECK(!producer_client_);
  producer_client_ = producer_client;
  target_buffer_ = data_source_config.target_buffer;

  TraceLog::GetInstance()->SetAddTraceEventOverride(
      &TraceEventDataSource::OnAddTraceEvent);

  TraceLog::GetInstance()->SetEnabled(
      TraceConfig(data_source_config.trace_config), TraceLog::RECORDING_MODE);
}

void TraceEventDataSource::StopTracing(
    base::RepeatingClosure stop_complete_callback) {
  DCHECK(producer_client_);

  {
    base::AutoLock lock(lock_);

    producer_client_ = nullptr;
    target_buffer_ = 0;
  }

  TraceLog::GetInstance()->SetAddTraceEventOverride(nullptr);

  // We call CancelTracing because we don't want/need TraceLog to do any of
  // its own JSON serialization on its own
  TraceLog::GetInstance()->CancelTracing(base::BindRepeating(
      [](base::RepeatingClosure stop_complete_callback,
         const scoped_refptr<base::RefCountedString>&, bool has_more_events) {
        if (!has_more_events && stop_complete_callback) {
          stop_complete_callback.Run();
        }
      },
      std::move(stop_complete_callback)));
}

TraceEventDataSource::ThreadLocalEventSink*
TraceEventDataSource::CreateThreadLocalEventSink() {
  base::AutoLock lock(lock_);

  if (producer_client_) {
    return new ThreadLocalEventSink(
        producer_client_->CreateTraceWriter(target_buffer_));
  } else {
    return nullptr;
  }
}

// static
void TraceEventDataSource::OnAddTraceEvent(const TraceEvent& trace_event) {
  auto* thread_local_event_sink =
      static_cast<ThreadLocalEventSink*>(ThreadLocalEventSinkSlot()->Get());

  if (!thread_local_event_sink) {
    thread_local_event_sink = GetInstance()->CreateThreadLocalEventSink();
    ThreadLocalEventSinkSlot()->Set(thread_local_event_sink);
  }

  if (thread_local_event_sink) {
    thread_local_event_sink->AddTraceEvent(trace_event);
  }
}

// static
void TraceEventDataSource::ResetCurrentThreadForTesting() {
  ThreadLocalEventSink* thread_local_event_sink =
      static_cast<ThreadLocalEventSink*>(ThreadLocalEventSinkSlot()->Get());
  if (thread_local_event_sink) {
    delete thread_local_event_sink;
    ThreadLocalEventSinkSlot()->Set(nullptr);
  }
}

}  // namespace tracing
