// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PREFS_PREF_REGISTRY_SIMPLE_H_
#define COMPONENTS_PREFS_PREF_REGISTRY_SIMPLE_H_

#include <stdint.h>

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/time/time.h"
#include "components/prefs/pref_registry.h"
#include "components/prefs/prefs_export.h"

namespace base {
class DictionaryValue;
class FilePath;
class ListValue;
}

// A simple implementation of PrefRegistry.
class COMPONENTS_PREFS_EXPORT PrefRegistrySimple : public PrefRegistry {
 public:
  PrefRegistrySimple();

  // For each of these registration methods, |flags| is an optional bitmask of
  // PrefRegistrationFlags.
  void RegisterBooleanPref(const std::string& path,
                           bool default_value,
                           uint32_t flags = NO_REGISTRATION_FLAGS);

  void RegisterIntegerPref(const std::string& path,
                           int default_value,
                           uint32_t flags = NO_REGISTRATION_FLAGS);

  void RegisterDoublePref(const std::string& path,
                          double default_value,
                          uint32_t flags = NO_REGISTRATION_FLAGS);

  void RegisterStringPref(const std::string& path,
                          const std::string& default_value,
                          uint32_t flags = NO_REGISTRATION_FLAGS);

  void RegisterFilePathPref(const std::string& path,
                            const base::FilePath& default_value,
                            uint32_t flags = NO_REGISTRATION_FLAGS);

  void RegisterListPref(const std::string& path,
                        uint32_t flags = NO_REGISTRATION_FLAGS);

  void RegisterListPref(const std::string& path,
                        std::unique_ptr<base::ListValue> default_value,
                        uint32_t flags = NO_REGISTRATION_FLAGS);

  void RegisterDictionaryPref(const std::string& path,
                              uint32_t flags = NO_REGISTRATION_FLAGS);

  void RegisterDictionaryPref(
      const std::string& path,
      std::unique_ptr<base::DictionaryValue> default_value,
      uint32_t flags = NO_REGISTRATION_FLAGS);

  void RegisterInt64Pref(const std::string& path,
                         int64_t default_value,
                         uint32_t flags = NO_REGISTRATION_FLAGS);

  void RegisterUint64Pref(const std::string& path,
                          uint64_t default_value,
                          uint32_t flags = NO_REGISTRATION_FLAGS);

  void RegisterTimePref(const std::string& path,
                        base::Time default_value,
                        uint32_t flags = NO_REGISTRATION_FLAGS);

 protected:
  ~PrefRegistrySimple() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(PrefRegistrySimple);
};

#endif  // COMPONENTS_PREFS_PREF_REGISTRY_SIMPLE_H_
