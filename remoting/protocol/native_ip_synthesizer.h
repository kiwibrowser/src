// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_PROTOCOL_NATIVE_IP_SYNTHESIZER_H_
#define REMOTING_PROTOCOL_NATIVE_IP_SYNTHESIZER_H_

namespace rtc {
class SocketAddress;
}  // namespace rtc

namespace remoting {
namespace protocol {

// Helper functions for synthesizing native IP address that is acceptabled by
// the OS from an IP literal.
//
// We hardcode IPv4 literals in stanza and other places. Some mobile ISP have an
// IPv6-only network with an IPv6->IPv4 gateway, so connecting with IPv4 literal
// may not work. Android and other OSes have a 464XLAT CLAT converter built into
// their network stack so IPv4 APIs are available to the app. However, iOS
// doesn't have this logic built into its network stack and instead requires
// developer to resolve an IPv6 address from IPv4 literal. This class helps
// working with this.

// Translate socket address into the one acceptable by the OS.
// If native IP synthesis is not needed by the OS, |original_socket| will be
// returned.
rtc::SocketAddress ToNativeSocket(const rtc::SocketAddress& original_socket);

}  // namespace protocol
}  // namespace remoting

#endif  // REMOTING_PROTOCOL_NATIVE_IP_SYNTHESIZER_H_
