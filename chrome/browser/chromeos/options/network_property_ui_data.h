// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_OPTIONS_NETWORK_PROPERTY_UI_DATA_H_
#define CHROME_BROWSER_CHROMEOS_OPTIONS_NETWORK_PROPERTY_UI_DATA_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "components/onc/onc_constants.h"

namespace base {
class DictionaryValue;
class Value;
}

namespace chromeos {

// Holds meta information for a network property: Whether the property is under
// policy control, if it is user-editable, and policy-provided default value, if
// available.
class NetworkPropertyUIData {
 public:
  // Initializes with ONC_SOURCE_NONE and no default value.
  NetworkPropertyUIData();

  // Initializes with the given |onc_source| and no default value.
  explicit NetworkPropertyUIData(::onc::ONCSource onc_source);

  ~NetworkPropertyUIData();

  // Update the property object from dictionary, reading the key given by
  // |property_key|.
  void ParseOncProperty(::onc::ONCSource onc_source,
                        const base::DictionaryValue* onc,
                        const std::string& property_key);

  const base::Value* default_value() const { return default_value_.get(); }
  bool IsManaged() const {
    return (onc_source_ == ::onc::ONC_SOURCE_DEVICE_POLICY ||
            onc_source_ == ::onc::ONC_SOURCE_USER_POLICY);
  }
  bool IsEditable() const { return !IsManaged(); }

 private:
  ::onc::ONCSource onc_source_;
  std::unique_ptr<base::Value> default_value_;

  DISALLOW_COPY_AND_ASSIGN(NetworkPropertyUIData);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_OPTIONS_NETWORK_PROPERTY_UI_DATA_H_
