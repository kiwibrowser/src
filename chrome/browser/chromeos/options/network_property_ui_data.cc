// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/options/network_property_ui_data.h"

#include "base/logging.h"
#include "base/values.h"
#include "chromeos/network/onc/onc_utils.h"

namespace chromeos {

NetworkPropertyUIData::NetworkPropertyUIData()
    : onc_source_(::onc::ONC_SOURCE_NONE) {
}

NetworkPropertyUIData::NetworkPropertyUIData(::onc::ONCSource onc_source)
    : onc_source_(onc_source) {
}

NetworkPropertyUIData::~NetworkPropertyUIData() {
}

void NetworkPropertyUIData::ParseOncProperty(::onc::ONCSource onc_source,
                                             const base::DictionaryValue* onc,
                                             const std::string& property_key) {
  default_value_.reset();
  onc_source_ = onc_source;

  if (!onc || !IsManaged())
    return;

  if (!onc::IsRecommendedValue(onc, property_key))
    return;

  // Set onc_source_ to NONE to indicate that the value is not enforced.
  onc_source_ = ::onc::ONC_SOURCE_NONE;

  const base::Value* default_value = NULL;
  if (!onc->Get(property_key, &default_value)) {
    // No default entry indicates that the property can be modified by the user,
    // but has no default value, e.g. Username or Passphrase. (The default
    // behavior for a property with no entry is non user modifiable).
    return;
  }

  // Set the recommended (default) value.
  default_value_.reset(default_value->DeepCopy());
}

}  // namespace chromeos
