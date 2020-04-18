// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_CLOUD_PRINT_CLOUD_PRINT_PROXY_INFO_H_
#define CHROME_COMMON_CLOUD_PRINT_CLOUD_PRINT_PROXY_INFO_H_

#include <string>

namespace cloud_print {

// This struct is used for ServiceHostMsg_CloudPrint_Info IPC message.
struct CloudPrintProxyInfo {
  CloudPrintProxyInfo();
  ~CloudPrintProxyInfo();

  bool enabled;
  std::string email;
  std::string proxy_id;
};

}  // namespace cloud_print

#endif  // CHROME_COMMON_CLOUD_PRINT_CLOUD_PRINT_PROXY_INFO_H_
