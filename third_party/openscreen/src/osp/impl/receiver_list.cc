// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "osp/impl/receiver_list.h"

#include <algorithm>

namespace openscreen {

ReceiverList::ReceiverList() = default;
ReceiverList::~ReceiverList() = default;

void ReceiverList::OnReceiverAdded(const ServiceInfo& info) {
  receivers_.emplace_back(info);
}

Error ReceiverList::OnReceiverChanged(const ServiceInfo& info) {
  auto existing_info = std::find_if(receivers_.begin(), receivers_.end(),
                                    [&info](const ServiceInfo& x) {
                                      return x.service_id == info.service_id;
                                    });
  if (existing_info == receivers_.end())
    return Error::Code::kNoItemFound;

  *existing_info = info;
  return Error::None();
}

Error ReceiverList::OnReceiverRemoved(const ServiceInfo& info) {
  const auto it = std::remove(receivers_.begin(), receivers_.end(), info);
  if (it == receivers_.end())
    return Error::Code::kNoItemFound;

  receivers_.erase(it, receivers_.end());
  return Error::None();
}

Error ReceiverList::OnAllReceiversRemoved() {
  const auto empty = receivers_.empty();
  receivers_.clear();
  return empty ? Error::Code::kNoItemFound : Error::None();
}

}  // namespace openscreen
