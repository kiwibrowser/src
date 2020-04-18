// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_COMMON_API_DECLARATIVE_NET_REQUEST_UTILS_H_
#define EXTENSIONS_COMMON_API_DECLARATIVE_NET_REQUEST_UTILS_H_

namespace extensions {
namespace declarative_net_request {

// Returns true if the Declarative Net Request API is available on the current
// channel.
bool IsAPIAvailable();

}  // namespace declarative_net_request
}  // namespace extensions

#endif  // EXTENSIONS_COMMON_API_DECLARATIVE_NET_REQUEST_UTILS_H_
