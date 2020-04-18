// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/network_hints/common/network_hints_messages.h"

#include "base/strings/string_number_conversions.h"
#include "components/network_hints/common/network_hints_common.h"

namespace IPC {

void ParamTraits<network_hints::LookupRequest>::Write(
    base::Pickle* m,
    const network_hints::LookupRequest& request) {
  IPC::WriteParam(m, request.hostname_list);
}

bool ParamTraits<network_hints::LookupRequest>::Read(
    const base::Pickle* m,
    base::PickleIterator* iter,
    network_hints::LookupRequest* request) {
  // Verify the hostname limits after deserialization success.
  if (IPC::ReadParam(m, iter, &request->hostname_list)) {
    network_hints::NameList& hostnames = request->hostname_list;
    if (hostnames.size() > network_hints::kMaxDnsHostnamesPerRequest)
      return false;

    for (const auto& hostname : hostnames) {
      if (hostname.length() > network_hints::kMaxDnsHostnameLength)
        return false;
    }
  }
  return true;
}

void ParamTraits<network_hints::LookupRequest>::Log(
    const network_hints::LookupRequest& p, std::string* l) {
  l->append("<network_hints::LookupRequest: ");
  l->append(base::NumberToString(p.hostname_list.size()));
  l->append(" hostnames>");
}

}  // namespace IPC
