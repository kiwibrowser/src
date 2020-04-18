// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CLOUD_DEVICES_COMMON_CLOUD_DEVICE_DESCRIPTION_H_
#define COMPONENTS_CLOUD_DEVICES_COMMON_CLOUD_DEVICE_DESCRIPTION_H_

#include <memory>
#include <string>

#include "base/macros.h"

namespace base {
class DictionaryValue;
class ListValue;
}

namespace cloud_devices {

// Provides parsing, serialization and validation Cloud Device Description or
// Cloud Job Ticket.
// https://developers.google.com/cloud-print/docs/cdd
class CloudDeviceDescription {
 public:
  CloudDeviceDescription();
  ~CloudDeviceDescription();

  bool InitFromString(const std::string& json);

  std::string ToString() const;

  const base::DictionaryValue& root() const { return *root_; }

  // Returns dictionary with capability/option.
  // Returns NULL if missing.
  const base::DictionaryValue* GetItem(const std::string& path) const;

  // Create dictionary for capability/option.
  // Never returns NULL.
  base::DictionaryValue* CreateItem(const std::string& path);

  // Returns list with capability/option.
  // Returns NULL if missing.
  const base::ListValue* GetListItem(const std::string& path) const;

  // Create list for capability/option.
  // Never returns NULL.
  base::ListValue* CreateListItem(const std::string& path);

 private:
  std::unique_ptr<base::DictionaryValue> root_;

  DISALLOW_COPY_AND_ASSIGN(CloudDeviceDescription);
};

}  // namespace cloud_devices

#endif  // COMPONENTS_CLOUD_DEVICES_COMMON_CLOUD_DEVICE_DESCRIPTION_H_
