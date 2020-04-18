// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DATA_REDUCTION_PROXY_CORE_BROWSER_LOFI_UI_SERVICE_H_
#define COMPONENTS_DATA_REDUCTION_PROXY_CORE_BROWSER_LOFI_UI_SERVICE_H_

#include "base/macros.h"

namespace net {
class URLRequest;
}

namespace data_reduction_proxy {

// Passes notifications to the UI thread that a Lo-Fi response has been
// received. These notifications may be used to show Lo-Fi UI.
class LoFiUIService {
 public:
  virtual ~LoFiUIService() {}

  // Notifies the UI thread that |request| has a Lo-Fi response.
  virtual void OnLoFiReponseReceived(const net::URLRequest& request) = 0;
};

}  // namespace data_reduction_proxy

#endif  // COMPONENTS_DATA_REDUCTION_PROXY_CORE_BROWSER_LOFI_UI_SERVICE_H_
