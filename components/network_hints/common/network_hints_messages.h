// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Multiply-included file, no traditional include guard.
#include <string>
#include <vector>

#include "components/network_hints/common/network_hints_common.h"
#include "ipc/ipc_message_macros.h"
#include "ipc/ipc_message_utils.h"
#include "url/ipc/url_param_traits.h"

// Singly-included section for custom IPC traits.
#ifndef COMPONENTS_NETWORK_HINTS_COMMON_NETWORK_HINTS_MESSAGES_H_
#define COMPONENTS_NETWORK_HINTS_COMMON_NETWORK_HINTS_MESSAGES_H_

namespace IPC {

template <>
struct ParamTraits<network_hints::LookupRequest> {
  typedef network_hints::LookupRequest param_type;
  static void Write(base::Pickle* m, const param_type& p);
  static bool Read(const base::Pickle* m,
                   base::PickleIterator* iter,
                   param_type* r);
  static void Log(const param_type& p, std::string* l);
};

}  // namespace IPC

#endif  // COMPONENTS_NETWORK_HINTS_COMMON_NETWORK_HINTS_MESSAGES_H_

#define IPC_MESSAGE_START NetworkHintsMsgStart

//-----------------------------------------------------------------------------
// Host messages
// These are messages sent from the renderer process to the browser process.

// Request for a DNS prefetch of the names in the array.
// NameList is typedef'ed std::vector<std::string>
IPC_MESSAGE_CONTROL1(NetworkHintsMsg_DNSPrefetch,
                     network_hints::LookupRequest)


// Request for preconnect to host providing resource specified by URL
IPC_MESSAGE_CONTROL3(NetworkHintsMsg_Preconnect,
                     GURL /* preconnect target url */,
                     bool /* Does connection have its credentials flag set */,
                     int /* number of connections */)
