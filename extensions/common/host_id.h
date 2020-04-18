// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_COMMON_HOST_ID_H_
#define EXTENSIONS_COMMON_HOST_ID_H_

#include <string>

// IDs of hosts who own user scripts.
// A HostID is immutable after creation.
struct HostID {
  enum HostType { EXTENSIONS, WEBUI, HOST_TYPE_LAST = WEBUI };

  HostID();
  HostID(HostType type, const std::string& id);
  HostID(const HostID& host_id);
  ~HostID();

  bool operator<(const HostID& host_id) const;
  bool operator==(const HostID& host_id) const;

  HostType type() const { return type_; }
  const std::string& id() const { return id_; }

 private:
  // The type of the host.
  HostType type_;

  // Similar to extension_id, host_id is a unique indentifier for a host,
  // e.g., an Extension or WebUI.
  std::string id_;
};

#endif  // EXTENSIONS_COMMON_HOST_ID_H_
