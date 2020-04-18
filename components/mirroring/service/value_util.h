// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_MIRRORING_SERVICE_VALUE_UTIL_H_
#define COMPONENTS_MIRRORING_SERVICE_VALUE_UTIL_H_

#include <string>

#include "base/values.h"

namespace mirroring {

// Read certain type of data from dictionary |value| if |key| exits. Return
// false if |key| exists and the type of the data mismatches. Return true
// otherwise.

bool GetInt(const base::Value& value, const std::string& key, int32_t* result);

bool GetDouble(const base::Value& value,
               const std::string& key,
               double* result);

bool GetString(const base::Value& value,
               const std::string& key,
               std::string* result);

bool GetBool(const base::Value& value, const std::string& key, bool* result);

bool GetIntArray(const base::Value& value,
                 const std::string& key,
                 std::vector<int32_t>* result);

bool GetStringArray(const base::Value& value,
                    const std::string& key,
                    std::vector<std::string>* result);
}  // namespace mirroring

#endif  // COMPONENTS_MIRRORING_SERVICE_VALUE_UTIL_H_
