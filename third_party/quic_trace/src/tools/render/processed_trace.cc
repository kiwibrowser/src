// Copyright 2018 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "tools/render/processed_trace.h"

#include "absl/algorithm/container.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "tools/render/layout_constants.h"

namespace quic_trace {
namespace render {

namespace {

std::string FormatBandwidth(size_t total_bytes, uint64_t time_delta_us) {
  double rate = 8e6 * total_bytes / time_delta_us;

  const char* unit = "";
  for (const char* current_unit : {"k", "M", "G"}) {
    if (rate < 1e3) {
      break;
    }
    rate /= 1e3;
    unit = current_unit;
  }

  char buffer[16];
  snprintf(buffer, sizeof(buffer), "%.2f %sbps", rate, unit);
  return buffer;
}

std::string FormatPercentage(size_t numerator, size_t denominator) {
  double percentage = 100. * numerator / denominator;
  char buffer[8];
  snprintf(buffer, sizeof(buffer), "%.2f%%", percentage);
  return buffer;
}

std::string FormatTime(uint64_t time_us) {
  char buffer[16];
  snprintf(buffer, sizeof(buffer), "%.3fs", time_us / 1e6);
  return buffer;
}

std::string FormatRtt(uint64_t rtt_us) {
  if (rtt_us < 10 * 1000) {
    return absl::StrCat(rtt_us, "µs");
  }
  if (rtt_us < 1000 * 1000) {
    return absl::StrCat(rtt_us / 1000, "ms");
  }
  return FormatTime(rtt_us);
}

}  // namespace

void ProcessedTrace::AddPacket(TraceRenderer* renderer,
                               const Event& packet,
                               Interval interval,
                               PacketType type) {
  renderer->AddPacket(packet.time_us(), interval.offset, interval.size, type);
  rendered_packets_.push_back(
      RenderedPacket{Box{vec2(packet.time_us(), interval.offset),
                         vec2(kSentPacketDurationMs, interval.size)},
                     &packet});
}

ProcessedTrace::ProcessedTrace(std::unique_ptr<Trace> trace,
                               TraceRenderer* renderer) {
  renderer->PacketCountHint(trace->events_size());

  quic_trace::NumberingWithoutRetransmissions numbering;
  size_t largest_sent = 0;
  uint64_t largest_time = 0;
  for (const Event& event : trace->events()) {
    if (!event.has_time_us() || event.time_us() < largest_time) {
      LOG(FATAL)
          << "All events in the trace have to be sorted in chronological order";
    }
    largest_time = event.time_us();

    if (event.event_type() == PACKET_SENT) {
      Interval mapped = numbering.AssignTraceNumbering(event);
      AddPacket(renderer, event, mapped, PacketType::SENT);
      largest_sent =
          std::max<size_t>(mapped.offset + mapped.size, largest_sent);
    }
    if (event.event_type() == PACKET_RECEIVED) {
      for (const Frame& frame : event.frames()) {
        for (const AckBlock& range : frame.ack_info().acked_packets()) {
          for (size_t i = range.first_packet(); i <= range.last_packet(); i++) {
            if (!packets_acked_.insert(i).second) {
              continue;
            }
            Interval mapped = numbering.GetTraceNumbering(i);
            AddPacket(renderer, event, mapped, PacketType::ACKED);
            acks_.emplace(vec2(event.time_us(), mapped.offset), i);
            // Don't count spurious retransmissions as losses.
            packets_lost_.erase(i);
          }
        }
      }
    }
    if (event.event_type() == PACKET_LOST) {
      Interval mapped = numbering.GetTraceNumbering(event.packet_number());
      AddPacket(renderer, event, mapped, PacketType::LOST);
      packets_lost_.insert(event.packet_number());
    }
    if (event.event_type() == APPLICATION_LIMITED) {
      // Normally, we would use the size of the packet as height, but
      // app-limited events have no size, so we pick an arbitrary number (in
      // bytes).
      const size_t kAppLimitedHeigth = 800;
      AddPacket(renderer, event, Interval{largest_sent, kAppLimitedHeigth},
                PacketType::APP_LIMITED);
    }
  }

  renderer->FinishPackets();
  trace_ = std::move(trace);
}

bool ProcessedTrace::SummaryTable(Table* table,
                                  float start_time,
                                  float end_time) {
  auto compare = [](Event a, Event b) { return a.time_us() < b.time_us(); };
  Event key_event;

  key_event.set_time_us(start_time);
  auto start_it = absl::c_lower_bound(trace_->events(), key_event, compare);

  key_event.set_time_us(end_time);
  auto end_it = absl::c_upper_bound(trace_->events(), key_event, compare);

  if (start_it > end_it) {
    return false;
  }

  // TODO(vasilvv): add actually useful information.
  size_t count_sent = 0;
  size_t bytes_sent = 0;
  size_t bytes_sent_acked = 0;
  size_t bytes_sent_lost = 0;
  for (auto it = start_it; it != end_it; it++) {
    switch (it->event_type()) {
      case PACKET_SENT:
        count_sent++;
        bytes_sent += it->packet_size();
        if (packets_acked_.count(it->packet_number()) > 0) {
          bytes_sent_acked += it->packet_size();
        }
        if (packets_lost_.count(it->packet_number()) > 0) {
          bytes_sent_lost += it->packet_size();
        }
        break;
      default:
        break;
    };
  }

  table->AddHeader("Selection summary");
  table->AddRow("#sent", absl::StrCat(count_sent));
  table->AddRow("Send rate",
                FormatBandwidth(bytes_sent, end_time - start_time));
  table->AddRow("Goodput",
                FormatBandwidth(bytes_sent_acked, end_time - start_time));
  table->AddRow("Loss rate", FormatPercentage(bytes_sent_lost, bytes_sent));
  return true;
}

ProcessedTrace::PacketSearchResult ProcessedTrace::FindPacketContainingPoint(
    vec2 point,
    vec2 margin) {
  RenderedPacket key{Box(), nullptr};

  key.box.origin.x = point.x - kSentPacketDurationMs - margin.x;
  auto start_it = absl::c_lower_bound(rendered_packets_, key);

  key.box.origin.x = point.x + kSentPacketDurationMs + margin.x;
  auto end_it = absl::c_upper_bound(rendered_packets_, key);

  if (start_it > end_it) {
    return PacketSearchResult();
  }

  auto closest_box = end_it;
  float closest_distance;
  for (auto it = start_it; it != end_it; it++) {
    Box target_box{it->box.origin - margin, it->box.size + 2 * margin};
    if (IsInside(point, target_box)) {
      float distance = DistanceSquared(point, BoxCenter(target_box));
      if (closest_box == end_it || distance < closest_distance) {
        closest_box = it;
        closest_distance = distance;
      }
    }
  }
  if (closest_box != end_it) {
    PacketSearchResult result;
    result.index = closest_box - rendered_packets_.cbegin();
    result.as_rendered = &closest_box->box;
    result.event = closest_box->packet;
    return result;
  }

  return PacketSearchResult();
}

namespace {
const char* EncryptionLevelToString(EncryptionLevel level) {
  switch (level) {
    case ENCRYPTION_INITIAL:
      return "Initial";
    case ENCRYPTION_HANDSHAKE:
      return "Handshake";
    case ENCRYPTION_0RTT:
      return "0-RTT";
    case ENCRYPTION_1RTT:
      return "1-RTT";

    case ENCRYPTION_UNKNOWN:
    default:
      return "???";
  }
}

constexpr int kMaxAckBlocksShown = 3;
std::string AckSummary(const AckInfo& ack) {
  if (ack.acked_packets_size() == 0) {
    return "";
  }

  bool overflow = false;
  int blocks_to_show = ack.acked_packets_size();
  if (blocks_to_show > kMaxAckBlocksShown) {
    blocks_to_show = kMaxAckBlocksShown;
    overflow = true;
  }

  std::vector<std::string> ack_ranges;
  for (int i = 0; i < blocks_to_show; i++) {
    const AckBlock& block = ack.acked_packets(i);
    if (block.first_packet() == block.last_packet()) {
      ack_ranges.push_back(absl::StrCat(block.first_packet()));
    } else {
      ack_ranges.push_back(
          absl::StrCat(block.first_packet(), ":", block.last_packet()));
    }
  }

  std::string result = absl::StrJoin(ack_ranges, ", ");
  if (overflow) {
    absl::StrAppend(&result, "...");
  }
  return result;
}
}  // namespace

void ProcessedTrace::FillTableForPacket(Table* table,
                                        const Box* as_rendered,
                                        const Event* packet) {
  switch (packet->event_type()) {
    case PACKET_SENT:
      table->AddHeader(absl::StrCat("Sent packet #", packet->packet_number()));
      break;
    case PACKET_RECEIVED: {
      std::string packet_acked = "???";
      auto it = acks_.find(as_rendered->origin);
      if (it != acks_.end()) {
        packet_acked = absl::StrCat(it->second);
      }
      table->AddHeader(absl::StrCat("Ack for packet #", packet_acked));
      break;
    }
    case PACKET_LOST:
      table->AddHeader(absl::StrCat("Lost packet #", packet->packet_number()));
      break;
    case APPLICATION_LIMITED:
      table->AddHeader("Application limited");
      break;
    default:
      return;
  }

  table->AddRow("Time", FormatTime(packet->time_us()));

  if (packet->event_type() == PACKET_SENT) {
    table->AddRow("Size", absl::StrCat(packet->packet_size(), " bytes"));
    table->AddRow("Encryption",
                  EncryptionLevelToString(packet->encryption_level()));

    table->AddHeader("Frame list");
    for (const Frame& frame : packet->frames()) {
      switch (frame.frame_type()) {
        case STREAM:
          table->AddRow(
              "Stream",
              absl::StrCat("#", frame.stream_frame_info().stream_id(), ": ",
                           frame.stream_frame_info().offset(), "-",
                           frame.stream_frame_info().offset() +
                               frame.stream_frame_info().length(),
                           " (", frame.stream_frame_info().length(), ")"));
          break;
        case RESET_STREAM:
          table->AddRow(
              "Reset stream",
              absl::StrCat("#", frame.reset_stream_info().stream_id()));
          break;
        case CONNECTION_CLOSE:
          table->AddRow(
              "Connection close",
              absl::StrCat(absl::Hex(frame.close_info().error_code())));
          break;
        case PING:
          table->AddRow("Ping", "");
          break;
        case BLOCKED:
          table->AddRow("Blocked", "");
          break;
        case STREAM_BLOCKED:
          table->AddRow("Stream blocked",
                        absl::StrCat(frame.flow_control_info().stream_id()));
          break;
        case ACK:
          table->AddRow("Ack", AckSummary(frame.ack_info()));
          break;
        default:
          table->AddRow("Unknown", "");
          break;
      }
    }
  }

  if (packet->has_transport_state()) {
    const TransportState& state = packet->transport_state();
    table->AddHeader("Transport state");
    if (state.has_last_rtt_us()) {
      table->AddRow("RTT", FormatRtt(state.last_rtt_us()));
    }
    if (state.has_smoothed_rtt_us()) {
      table->AddRow("SRTT", FormatRtt(state.smoothed_rtt_us()));
    }
    if (state.has_min_rtt_us()) {
      table->AddRow("Min RTT", FormatRtt(state.min_rtt_us()));
    }
    if (state.has_in_flight_bytes()) {
      table->AddRow("In flight",
                    absl::StrCat(state.in_flight_bytes(), " bytes"));
    }
    if (state.has_cwnd_bytes()) {
      table->AddRow("CWND", absl::StrCat(state.cwnd_bytes(), " bytes"));
    }
    if (state.has_pacing_rate_bps()) {
      table->AddRow("Pacing rate",
                    FormatBandwidth(state.pacing_rate_bps(), 8000 * 1000));
    }
    if (state.has_congestion_control_state()) {
      // Truncate CC state strings longer than 80 characters.
      const int kMaxLen = 80;
      std::string ccstate = state.congestion_control_state();
      if (ccstate.length() > kMaxLen) {
        ccstate = ccstate.substr(0, kMaxLen) + "...";
      }
      table->AddRow("CC State", ccstate);
    }
  }
}

absl::optional<Box> ProcessedTrace::BoundContainedPackets(Box boundary) {
  RenderedPacket key{Box(), nullptr};

  key.box.origin.x = boundary.origin.x;
  auto range_start = absl::c_lower_bound(rendered_packets_, key);

  key.box.origin.x =
      boundary.origin.x + boundary.size.x + kSentPacketDurationMs;
  auto range_end = absl::c_upper_bound(rendered_packets_, key);

  if (range_start == rendered_packets_.end() ||
      range_end == rendered_packets_.end() || range_start > range_end) {
    return absl::nullopt;
  }

  absl::optional<Box> result;
  for (auto it = range_start; it <= range_end; it++) {
    if (IsInside(it->box, boundary)) {
      result = result.has_value() ? BoundingBox(*result, it->box) : it->box;
    }
  }
  return result;
}

}  // namespace render
}  // namespace quic_trace
