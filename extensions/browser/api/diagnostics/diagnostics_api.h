// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_API_DIAGNOSTICS_DIAGNOSTICS_API_H_
#define EXTENSIONS_BROWSER_API_DIAGNOSTICS_DIAGNOSTICS_API_H_

#include <memory>
#include <string>

#include "extensions/browser/api/async_api_function.h"
#include "extensions/common/api/diagnostics.h"

namespace extensions {

class DiagnosticsSendPacketFunction : public AsyncApiFunction {
 public:
  // Result code for sending packet. Platform specific AsyncWorkStart() will
  // finish with this ResultCode so we can maximize shared code.
  enum SendPacketResultCode {
    // Ping packed is sent and ICMP reply is received before time out.
    SEND_PACKET_OK,

    // Not implemented on the platform.
    SEND_PACKET_NOT_IMPLEMENTED,

    // The ping operation failed because of timeout or network unreachable.
    SEND_PACKET_FAILED,
  };

  DECLARE_EXTENSION_FUNCTION("diagnostics.sendPacket", DIAGNOSTICS_SENDPACKET);

  DiagnosticsSendPacketFunction();

 protected:
  ~DiagnosticsSendPacketFunction() override;

  // AsyncApiFunction:
  bool Prepare() override;
  // This methods will be implemented differently on different platforms.
  void AsyncWorkStart() override;
  bool Respond() override;

 private:
  void SendPingPacket();
  void OnCompleted(SendPacketResultCode result_code,
                   const std::string& ip,
                   double latency);

  std::unique_ptr<api::diagnostics::SendPacket::Params> parameters_;
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_API_DIAGNOSTICS_DIAGNOSTICS_API_H_
