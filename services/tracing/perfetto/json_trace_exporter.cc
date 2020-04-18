// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/tracing/perfetto/json_trace_exporter.h"

#include "base/json/string_escape.h"
#include "base/logging.h"

#include "base/format_macros.h"
#include "base/strings/stringprintf.h"
#include "base/trace_event/trace_event.h"
#include "services/tracing/public/mojom/perfetto_service.mojom.h"
#include "third_party/perfetto/include/perfetto/tracing/core/trace_config.h"
#include "third_party/perfetto/include/perfetto/tracing/core/trace_packet.h"
#include "third_party/perfetto/protos/perfetto/trace/chrome/chrome_trace_event.pbzero.h"
#include "third_party/perfetto/protos/perfetto/trace/chrome/chrome_trace_packet.pb.h"

using TraceEvent = base::trace_event::TraceEvent;

namespace {

void OutputJSONFromTraceEventProto(
    const perfetto::protos::ChromeTraceEvent& event,
    std::string* out) {
  char phase = static_cast<char>(event.phase());
  base::StringAppendF(out,
                      "{\"pid\":%i,\"tid\":%i,\"ts\":%" PRId64
                      ",\"ph\":\"%c\",\"cat\":\"%s\",\"name\":",
                      event.process_id(), event.thread_id(), event.timestamp(),
                      phase, event.category_group_name().c_str());
  base::EscapeJSONString(event.name(), true, out);

  if (event.has_duration()) {
    base::StringAppendF(out, ",\"dur\":%" PRId64, event.duration());
  }

  if (event.has_thread_duration()) {
    base::StringAppendF(out, ",\"tdur\":%" PRId64, event.thread_duration());
  }

  if (event.has_thread_timestamp()) {
    base::StringAppendF(out, ",\"tts\":%" PRId64, event.thread_timestamp());
  }

  // Output async tts marker field if flag is set.
  if (event.flags() & TRACE_EVENT_FLAG_ASYNC_TTS) {
    base::StringAppendF(out, ", \"use_async_tts\":1");
  }

  // If id_ is set, print it out as a hex string so we don't loose any
  // bits (it might be a 64-bit pointer).
  unsigned int id_flags =
      event.flags() & (TRACE_EVENT_FLAG_HAS_ID | TRACE_EVENT_FLAG_HAS_LOCAL_ID |
                       TRACE_EVENT_FLAG_HAS_GLOBAL_ID);
  if (id_flags) {
    if (event.has_scope()) {
      base::StringAppendF(out, ",\"scope\":\"%s\"", event.scope().c_str());
    }

    DCHECK(event.has_id());
    switch (id_flags) {
      case TRACE_EVENT_FLAG_HAS_ID:
        base::StringAppendF(out, ",\"id\":\"0x%" PRIx64 "\"",
                            static_cast<uint64_t>(event.id()));
        break;

      case TRACE_EVENT_FLAG_HAS_LOCAL_ID:
        base::StringAppendF(out, ",\"id2\":{\"local\":\"0x%" PRIx64 "\"}",
                            static_cast<uint64_t>(event.id()));
        break;

      case TRACE_EVENT_FLAG_HAS_GLOBAL_ID:
        base::StringAppendF(out, ",\"id2\":{\"global\":\"0x%" PRIx64 "\"}",
                            static_cast<uint64_t>(event.id()));
        break;

      default:
        NOTREACHED() << "More than one of the ID flags are set";
        break;
    }
  }

  if (event.flags() & TRACE_EVENT_FLAG_BIND_TO_ENCLOSING)
    base::StringAppendF(out, ",\"bp\":\"e\"");

  if (event.has_bind_id()) {
    base::StringAppendF(out, ",\"bind_id\":\"0x%" PRIx64 "\"",
                        static_cast<uint64_t>(event.bind_id()));
  }

  if (event.flags() & TRACE_EVENT_FLAG_FLOW_IN)
    base::StringAppendF(out, ",\"flow_in\":true");
  if (event.flags() & TRACE_EVENT_FLAG_FLOW_OUT)
    base::StringAppendF(out, ",\"flow_out\":true");

  // Instant events also output their scope.
  if (phase == TRACE_EVENT_PHASE_INSTANT) {
    char scope = '?';
    switch (event.flags() & TRACE_EVENT_FLAG_SCOPE_MASK) {
      case TRACE_EVENT_SCOPE_GLOBAL:
        scope = TRACE_EVENT_SCOPE_NAME_GLOBAL;
        break;

      case TRACE_EVENT_SCOPE_PROCESS:
        scope = TRACE_EVENT_SCOPE_NAME_PROCESS;
        break;

      case TRACE_EVENT_SCOPE_THREAD:
        scope = TRACE_EVENT_SCOPE_NAME_THREAD;
        break;
    }
    base::StringAppendF(out, ",\"s\":\"%c\"", scope);
  }

  *out += ",\"args\":{";
  for (int i = 0; i < event.args_size(); ++i) {
    auto& arg = event.args(i);

    if (i > 0) {
      *out += ",";
    }

    *out += "\"";
    *out += arg.name();
    *out += "\":";

    TraceEvent::TraceValue value;
    if (arg.has_bool_value()) {
      value.as_bool = arg.bool_value();
      TraceEvent::AppendValueAsJSON(TRACE_VALUE_TYPE_BOOL, value, out);
      continue;
    }

    if (arg.has_uint_value()) {
      value.as_uint = arg.uint_value();
      TraceEvent::AppendValueAsJSON(TRACE_VALUE_TYPE_UINT, value, out);
      continue;
    }

    if (arg.has_int_value()) {
      value.as_int = arg.int_value();
      TraceEvent::AppendValueAsJSON(TRACE_VALUE_TYPE_INT, value, out);
      continue;
    }

    if (arg.has_double_value()) {
      value.as_double = arg.double_value();
      TraceEvent::AppendValueAsJSON(TRACE_VALUE_TYPE_DOUBLE, value, out);
      continue;
    }

    if (arg.has_pointer_value()) {
      value.as_pointer = reinterpret_cast<void*>(arg.pointer_value());
      TraceEvent::AppendValueAsJSON(TRACE_VALUE_TYPE_POINTER, value, out);
      continue;
    }

    if (arg.has_string_value()) {
      std::string str = arg.string_value();
      value.as_string = &str[0];
      TraceEvent::AppendValueAsJSON(TRACE_VALUE_TYPE_STRING, value, out);
      continue;
    }
  }

  *out += "}}";
}

}  // namespace

namespace tracing {

JSONTraceExporter::JSONTraceExporter(const std::string& config,
                                     perfetto::Service* service)
    : config_(config) {
  consumer_endpoint_ = service->ConnectConsumer(this);
}

JSONTraceExporter::~JSONTraceExporter() = default;

void JSONTraceExporter::OnConnect() {
  // Start tracing.
  perfetto::TraceConfig trace_config;
  trace_config.add_buffers()->set_size_kb(4096 * 100);
  auto* ds_config = trace_config.add_data_sources()->mutable_config();
  ds_config->set_name(mojom::kTraceEventDataSourceName);
  ds_config->set_target_buffer(0);
  auto* chrome_config = ds_config->mutable_chrome_config();
  chrome_config->set_trace_config(config_);

  consumer_endpoint_->EnableTracing(trace_config);
}

void JSONTraceExporter::OnDisconnect() {}

void JSONTraceExporter::StopAndFlush(OnTraceEventJSONCallback callback) {
  DCHECK(!json_callback_ && callback);
  json_callback_ = callback;

  consumer_endpoint_->DisableTracing();
  consumer_endpoint_->ReadBuffers();
}

void JSONTraceExporter::OnTraceData(std::vector<perfetto::TracePacket> packets,
                                    bool has_more) {
  DCHECK(json_callback_);
  DCHECK(!packets.empty() || !has_more);

  std::string out;

  if (!has_output_json_preamble_) {
    out = "{\"traceEvents\":[";
    has_output_json_preamble_ = true;
  }

  for (auto& encoded_packet : packets) {
    perfetto::protos::ChromeTracePacket packet;
    bool decoded = encoded_packet.Decode(&packet);
    DCHECK(decoded);

    if (!packet.has_chrome_events()) {
      continue;
    }

    const perfetto::protos::ChromeEventBundle& bundle = packet.chrome_events();
    for (const perfetto::protos::ChromeTraceEvent& event :
         bundle.trace_events()) {
      if (has_output_first_event_) {
        out += ",";
      } else {
        has_output_first_event_ = true;
      }

      OutputJSONFromTraceEventProto(event, &out);
    }
  }

  if (!has_more) {
    out += "]}";
  }

  json_callback_.Run(out, has_more);
}

}  // namespace tracing
