// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMECAST_BROWSER_CAST_RESOURCE_DISPATCHER_HOST_DELEGATE_H_
#define CHROMECAST_BROWSER_CAST_RESOURCE_DISPATCHER_HOST_DELEGATE_H_

#include "base/macros.h"
#include "content/public/browser/resource_dispatcher_host_delegate.h"

namespace chromecast {
namespace shell {

class CastResourceDispatcherHostDelegate
    : public content::ResourceDispatcherHostDelegate {
 public:
  CastResourceDispatcherHostDelegate() {}

  // content::ResourceDispatcherHostDelegate implementation:
  void RequestComplete(net::URLRequest* url_request, int net_error) override;
  // TODO(seantopping): Deprecate this once the above method becomes used.
  void RequestComplete(net::URLRequest* url_request) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(CastResourceDispatcherHostDelegate);
};

}  // namespace shell
}  // namespace chromecast

#endif  // CHROMECAST_BROWSER_CAST_RESOURCE_DISPATCHER_HOST_DELEGATE_H_
