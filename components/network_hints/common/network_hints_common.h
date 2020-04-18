// Copyright (c) 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_NETWORK_HINTS_COMMON_NETWORK_HINTS_COMMON_H_
#define COMPONENTS_NETWORK_HINTS_COMMON_NETWORK_HINTS_COMMON_H_

#include <stddef.h>

#include <string>
#include <vector>

#include "url/gurl.h"

namespace network_hints {

// IPC messages are passed from the renderer to the browser in the form of
// Namelist instances.
// Each element of this vector is a hostname that needs to be looked up.
// The hostnames should never be empty strings.
typedef std::vector<std::string> NameList;
// TODO(jar): We still need to switch to passing scheme/host/port in UrlList,
// instead of NameList, from renderer (where content of pages are scanned for
// links) to browser (where we perform predictive actions).
typedef std::vector<GURL> UrlList;

struct LookupRequest {
  LookupRequest();
  ~LookupRequest();

  NameList hostname_list;
};

// The maximum number of hostnames submitted to the browser DNS resolver per
// IPC call.
extern const size_t kMaxDnsHostnamesPerRequest;

// The maximum length for a given DNS hostname to resolve.
extern const size_t kMaxDnsHostnameLength;

}  // namespace network_hints

#endif  // COMPONENTS_NETWORK_HINTS_COMMON_NETWORK_HINTS_COMMON_H_
