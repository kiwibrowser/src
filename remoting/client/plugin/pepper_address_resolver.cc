// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "remoting/client/plugin/pepper_address_resolver.h"

#include "base/logging.h"
#include "ppapi/cpp/net_address.h"
#include "remoting/client/plugin/pepper_util.h"

namespace remoting {

PepperAddressResolver::PepperAddressResolver(const pp::InstanceHandle& instance)
    : resolver_(instance),
      error_(0),
      callback_factory_(this) {
}

PepperAddressResolver::~PepperAddressResolver() {
}

void PepperAddressResolver::Start(const rtc::SocketAddress& address) {
  DCHECK(!address.hostname().empty());

  PP_HostResolver_Hint hint;
  hint.flags = 0;
  hint.family = PP_NETADDRESS_FAMILY_UNSPECIFIED;
  pp::CompletionCallback callback =
      callback_factory_.NewCallback(&PepperAddressResolver::OnResolved);
  int result = resolver_.Resolve(
      address.hostname().c_str(), address.port(), hint, callback);
  DCHECK_EQ(result, PP_OK_COMPLETIONPENDING);
}

bool PepperAddressResolver::GetResolvedAddress(int family,
                                               rtc::SocketAddress* addr) const {
  rtc::SocketAddress result;
  switch (family) {
    case AF_INET:
      result = ipv4_address_;
      break;
    case AF_INET6:
      result = ipv6_address_;
      break;
  }

  if (result.IsNil())
    return false;

  *addr = result;
  return true;
}

int PepperAddressResolver::GetError() const {
  return error_;
}

void PepperAddressResolver::Destroy(bool wait) {
  delete this;
}

void PepperAddressResolver::OnResolved(int32_t result) {
  if (result < 0) {
    error_ = EAI_FAIL;
    SignalDone(this);
    return;
  }

  int count = resolver_.GetNetAddressCount();
  for (int i = 0; i < count; ++i) {
    pp::NetAddress address = resolver_.GetNetAddress(i);
    if (address.GetFamily() == PP_NETADDRESS_FAMILY_IPV4 &&
        ipv4_address_.IsNil()) {
      PpNetAddressToSocketAddress(address, &ipv4_address_);
    } else if (address.GetFamily() == PP_NETADDRESS_FAMILY_IPV6 &&
               ipv6_address_.IsNil()) {
      PpNetAddressToSocketAddress(address, &ipv6_address_);
    }
  }

  SignalDone(this);
}

}  // namespace remoting
