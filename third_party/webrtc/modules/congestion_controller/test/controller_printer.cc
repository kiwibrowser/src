/*
 *  Copyright 2018 The WebRTC project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#include "modules/congestion_controller/test/controller_printer.h"

#include <limits>
#include <utility>

#include "absl/types/optional.h"
#include "api/units/data_rate.h"
#include "api/units/data_size.h"
#include "api/units/time_delta.h"

namespace webrtc {

ControlStatePrinter::ControlStatePrinter(
    std::unique_ptr<RtcEventLogOutput> output,
    std::unique_ptr<DebugStatePrinter> debug_printer)
    : output_(std::move(output)), debug_printer_(std::move(debug_printer)) {}

ControlStatePrinter::~ControlStatePrinter() = default;

void ControlStatePrinter::PrintHeaders() {
  output_->Write("time bandwidth rtt target pacing padding window");
  if (debug_printer_) {
    output_->Write(" ");
    debug_printer_->PrintHeaders(output_.get());
  }
  output_->Write("\n");
  output_->Flush();
}

void ControlStatePrinter::PrintState(const Timestamp time,
                                     const NetworkControlUpdate state) {
  double timestamp = time.seconds<double>();
  auto estimate = state.target_rate->network_estimate;
  double bandwidth = estimate.bandwidth.bps() / 8.0;
  double rtt = estimate.round_trip_time.seconds<double>();
  double target_rate = state.target_rate->target_rate.bps() / 8.0;
  double pacing_rate = state.pacer_config->data_rate().bps() / 8.0;
  double padding_rate = state.pacer_config->pad_rate().bps() / 8.0;
  double congestion_window = state.congestion_window
                                 ? state.congestion_window->bytes<double>()
                                 : std::numeric_limits<double>::infinity();
  LogWriteFormat(output_.get(), "%f %f %f %f %f %f %f", timestamp, bandwidth,
                 rtt, target_rate, pacing_rate, padding_rate,
                 congestion_window);

  if (debug_printer_) {
    output_->Write(" ");
    debug_printer_->PrintValues(output_.get());
  }
  output_->Write("\n");
  output_->Flush();
}

void ControlStatePrinter::PrintState(const Timestamp time) {
  if (debug_printer_ && debug_printer_->Attached()) {
    PrintState(time, debug_printer_->GetState(time));
  }
}
}  // namespace webrtc
