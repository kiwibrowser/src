// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef OSP_IMPL_RECEIVER_LIST_H_
#define OSP_IMPL_RECEIVER_LIST_H_

#include <vector>

#include "osp/public/service_info.h"
#include "osp_base/error.h"

namespace openscreen {

class ReceiverList {
 public:
  ReceiverList();
  ~ReceiverList();
  ReceiverList(ReceiverList&&) = delete;
  ReceiverList& operator=(ReceiverList&&) = delete;

  void OnReceiverAdded(const ServiceInfo& info);

  Error OnReceiverChanged(const ServiceInfo& info);
  Error OnReceiverRemoved(const ServiceInfo& info);
  Error OnAllReceiversRemoved();

  const std::vector<ServiceInfo>& receivers() const { return receivers_; }

 private:
  std::vector<ServiceInfo> receivers_;
};

}  // namespace openscreen

#endif  // OSP_IMPL_RECEIVER_LIST_H_
