// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_SERVICE_MANAGER_SERVICE_OVERRIDES_H_
#define SERVICES_SERVICE_MANAGER_SERVICE_OVERRIDES_H_

#include <map>
#include <memory>
#include <string>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/values.h"

namespace service_manager {

class ServiceOverrides {
 public:
  struct Entry {
    Entry();
    ~Entry();

    base::FilePath executable_path;
    std::string package_name;
  };

  explicit ServiceOverrides(std::unique_ptr<base::Value> overrides);
  ~ServiceOverrides();

  bool GetExecutablePathOverride(const std::string& service_name,
                                 base::FilePath* path) const;

  const std::map<std::string, Entry>& entries() const { return entries_; }

 private:
  std::map<std::string, Entry> entries_;

  DISALLOW_COPY_AND_ASSIGN(ServiceOverrides);
};

}  // namespace service_manager

#endif  // SERVICES_SERVICE_MANAGER_SERVICE_OVERRIDES_H_
