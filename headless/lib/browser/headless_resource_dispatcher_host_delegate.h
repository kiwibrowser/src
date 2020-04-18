// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef HEADLESS_LIB_BROWSER_HEADLESS_RESOURCE_DISPATCHER_HOST_DELEGATE_H_
#define HEADLESS_LIB_BROWSER_HEADLESS_RESOURCE_DISPATCHER_HOST_DELEGATE_H_

#include "content/public/browser/resource_dispatcher_host_delegate.h"

namespace headless {

class HeadlessResourceDispatcherHostDelegate
    : public content::ResourceDispatcherHostDelegate {
 public:
  HeadlessResourceDispatcherHostDelegate();
  ~HeadlessResourceDispatcherHostDelegate() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(HeadlessResourceDispatcherHostDelegate);
};

}  // namespace headless

#endif  // HEADLESS_LIB_BROWSER_HEADLESS_RESOURCE_DISPATCHER_HOST_DELEGATE_H_
