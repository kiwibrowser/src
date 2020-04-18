// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/diagnostics/diagnostics_api.h"

namespace {

const char kErrorPingNotImplemented[] = "Not implemented";
const char kErrorPingFailed[] = "Failed to send ping packet";
}

namespace extensions {

namespace SendPacket = api::diagnostics::SendPacket;

DiagnosticsSendPacketFunction::DiagnosticsSendPacketFunction() {
}

DiagnosticsSendPacketFunction::~DiagnosticsSendPacketFunction() {
}

bool DiagnosticsSendPacketFunction::Prepare() {
  parameters_ = SendPacket::Params::Create(*args_);
  EXTENSION_FUNCTION_VALIDATE(parameters_.get());
  return true;
}

bool DiagnosticsSendPacketFunction::Respond() {
  return error_.empty();
}

void DiagnosticsSendPacketFunction::OnCompleted(
    SendPacketResultCode result_code,
    const std::string& ip,
    double latency) {
  switch (result_code) {
    case SEND_PACKET_OK: {
      api::diagnostics::SendPacketResult result;
      result.ip = ip;
      result.latency = latency;
      results_ = SendPacket::Results::Create(result);
      break;
    }
    case SEND_PACKET_NOT_IMPLEMENTED:
      SetError(kErrorPingNotImplemented);
      break;
    case SEND_PACKET_FAILED:
      SetError(kErrorPingFailed);
      break;
  }
  AsyncWorkCompleted();
}

}  // namespace extensions
