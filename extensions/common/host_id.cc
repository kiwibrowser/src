// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/common/host_id.h"

HostID::HostID()
    : type_(HostType::EXTENSIONS) {
}

HostID::HostID(HostType type, const std::string& id)
    : type_(type), id_(id) {
}

HostID::HostID(const HostID& host_id)
    : type_(host_id.type()),
      id_(host_id.id()) {
}

HostID::~HostID() {
}

bool HostID::operator<(const HostID& host_id) const {
  if (type_ != host_id.type())
    return type_ < host_id.type();
  else if (id_ != host_id.id())
    return id_ < host_id.id();
  return false;
}

bool HostID::operator==(const HostID& host_id) const {
  return type_ == host_id.type() && id_ == host_id.id();
}
