// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_SERVICE_MANAGER_EMBEDDER_SHARED_FILE_UTIL_H_
#define SERVICES_SERVICE_MANAGER_EMBEDDER_SHARED_FILE_UTIL_H_

#include <map>
#include <string>

#include "base/command_line.h"
#include "base/optional.h"
#include "services/service_manager/embedder/service_manager_embedder_export.h"

namespace service_manager {

class SERVICE_MANAGER_EMBEDDER_EXPORT SharedFileSwitchValueBuilder final {
 public:
  void AddEntry(const std::string& key_str, int key_id);
  const std::string& switch_value() const { return switch_value_; }

 private:
  std::string switch_value_;
};

SERVICE_MANAGER_EMBEDDER_EXPORT base::Optional<std::map<int, std::string>>
ParseSharedFileSwitchValue(const std::string& value);

}  // namespace service_manager

#endif  // SERVICES_SERVICE_MANAGER_EMBEDDER_SHARED_FILE_UTIL_H_
