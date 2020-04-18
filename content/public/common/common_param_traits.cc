// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/common/common_param_traits.h"

#include <string>

#include "base/containers/stack_container.h"
#include "content/public/common/content_constants.h"
#include "content/public/common/page_state.h"
#include "content/public/common/referrer.h"
#include "net/base/ip_address.h"
#include "net/base/ip_endpoint.h"
#include "net/http/http_util.h"

namespace IPC {

void ParamTraits<net::IPEndPoint>::Write(base::Pickle* m, const param_type& p) {
  WriteParam(m, p.address());
  WriteParam(m, p.port());
}

bool ParamTraits<net::IPEndPoint>::Read(const base::Pickle* m,
                                        base::PickleIterator* iter,
                                        param_type* p) {
  net::IPAddress address;
  uint16_t port;
  if (!ReadParam(m, iter, &address) || !ReadParam(m, iter, &port))
    return false;
  if (!address.empty() && !address.IsValid())
    return false;

  *p = net::IPEndPoint(address, port);
  return true;
}

void ParamTraits<net::IPEndPoint>::Log(const param_type& p, std::string* l) {
  LogParam("IPEndPoint:" + p.ToString(), l);
}

void ParamTraits<net::IPAddress>::Write(base::Pickle* m, const param_type& p) {
  base::StackVector<uint8_t, 16> bytes;
  for (uint8_t byte : p.bytes())
    bytes->push_back(byte);
  WriteParam(m, bytes);
}

bool ParamTraits<net::IPAddress>::Read(const base::Pickle* m,
                                       base::PickleIterator* iter,
                                       param_type* p) {
  base::StackVector<uint8_t, 16> bytes;
  if (!ReadParam(m, iter, &bytes))
    return false;
  if (bytes->size() && bytes->size() != net::IPAddress::kIPv4AddressSize &&
      bytes->size() != net::IPAddress::kIPv6AddressSize) {
    return false;
  }
  *p = net::IPAddress(bytes->data(), bytes->size());
  return true;
}

void ParamTraits<net::IPAddress>::Log(const param_type& p, std::string* l) {
    LogParam("IPAddress:" + (p.empty() ? "(empty)" : p.ToString()), l);
}

void ParamTraits<content::PageState>::Write(base::Pickle* m,
                                            const param_type& p) {
  WriteParam(m, p.ToEncodedData());
}

bool ParamTraits<content::PageState>::Read(const base::Pickle* m,
                                           base::PickleIterator* iter,
                                           param_type* r) {
  std::string data;
  if (!ReadParam(m, iter, &data))
    return false;
  *r = content::PageState::CreateFromEncodedData(data);
  return true;
}

void ParamTraits<content::PageState>::Log(
    const param_type& p, std::string* l) {
  l->append("(");
  LogParam(p.ToEncodedData(), l);
  l->append(")");
}

}  // namespace IPC

// Generate param traits write methods.
#include "ipc/param_traits_write_macros.h"
namespace IPC {
#undef CONTENT_PUBLIC_COMMON_COMMON_PARAM_TRAITS_MACROS_H_
#include "content/public/common/common_param_traits_macros.h"
}  // namespace IPC

// Generate param traits read methods.
#include "ipc/param_traits_read_macros.h"
namespace IPC {
#undef CONTENT_PUBLIC_COMMON_COMMON_PARAM_TRAITS_MACROS_H_
#include "content/public/common/common_param_traits_macros.h"
}  // namespace IPC

// Generate param traits log methods.
#include "ipc/param_traits_log_macros.h"
namespace IPC {
#undef CONTENT_PUBLIC_COMMON_COMMON_PARAM_TRAITS_MACROS_H_
#include "content/public/common/common_param_traits_macros.h"
}  // namespace IPC
