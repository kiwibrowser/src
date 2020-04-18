// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_NETWORK_NETWORK_UI_DATA_H_
#define CHROMEOS_NETWORK_NETWORK_UI_DATA_H_

#include <memory>
#include <string>

#include "chromeos/chromeos_export.h"
#include "components/onc/onc_constants.h"

namespace base {
class DictionaryValue;
}

namespace chromeos {

// Helper for accessing and setting values in the network's UI data dictionary.
// Accessing values is done via static members that take the network as an
// argument. In order to fill a UI data dictionary, construct an instance, set
// up your data members, and call FillDictionary(). For example, if you have a
// |network|:
//
//      NetworkUIData ui_data;
//      ui_data.FillDictionary(network->ui_data());
class CHROMEOS_EXPORT NetworkUIData {
 public:
  NetworkUIData();
  NetworkUIData(const NetworkUIData& other);
  NetworkUIData& operator=(const NetworkUIData& other);
  explicit NetworkUIData(const base::DictionaryValue& dict);
  ~NetworkUIData();

  ::onc::ONCSource onc_source() const { return onc_source_; }

  const base::DictionaryValue* user_settings() const {
    return user_settings_.get();
  }
  void set_user_settings(std::unique_ptr<base::DictionaryValue> dict);

  // Returns |onc_source_| as a string, one of kONCSource*.
  std::string GetONCSourceAsString() const;

  // Fills in |dict| with the currently configured values. This will write the
  // keys appropriate for Network::ui_data() as defined below (kKeyXXX).
  void FillDictionary(base::DictionaryValue* dict) const;

  // Creates a NetworkUIData object from |onc_source|. This function is used to
  // create the "UIData" property of the Shill configuration.
  static std::unique_ptr<NetworkUIData> CreateFromONC(
      ::onc::ONCSource onc_source);

  // Key for storing source of the ONC network.
  static const char kKeyONCSource[];

  // Key for storing the user settings.
  static const char kKeyUserSettings[];

  // Values for kKeyONCSource
  static const char kONCSourceUserImport[];
  static const char kONCSourceDevicePolicy[];
  static const char kONCSourceUserPolicy[];

 private:
  ::onc::ONCSource onc_source_;
  std::unique_ptr<base::DictionaryValue> user_settings_;
};

}  // namespace chromeos

#endif  // CHROMEOS_NETWORK_NETWORK_UI_DATA_H_
