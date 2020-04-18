// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_API_SETTINGS_PRIVATE_GENERATED_PREFS_H_
#define CHROME_BROWSER_EXTENSIONS_API_SETTINGS_PRIVATE_GENERATED_PREFS_H_

#include <memory>
#include <string>
#include <unordered_map>

#include "base/macros.h"
#include "chrome/browser/extensions/api/settings_private/generated_pref.h"
#include "chrome/browser/extensions/api/settings_private/prefs_util_enums.h"
#include "components/keyed_service/core/keyed_service.h"

class Profile;

namespace base {
class Value;
}

namespace extensions {
namespace api {
namespace settings_private {
struct PrefObject;
}  // namespace settings_private
}  // namespace api

namespace settings_private {

// This is a "store" for virtual preferences that exist only for
// api::settings_private. These are used to control Chrome Settings UI elements
// not directly attached to user preferences.
class GeneratedPrefs : public KeyedService {
 public:
  // Preference name to implementation map.
  using PrefsMap =
      std::unordered_map<std::string, std::unique_ptr<GeneratedPref>>;

  explicit GeneratedPrefs(Profile* profile);
  ~GeneratedPrefs() override;

  // Returns true if preference is supported.
  bool HasPref(const std::string& pref_name) const;

  // Returns fully populated PrefObject or nullptr if not supported.
  std::unique_ptr<api::settings_private::PrefObject> GetPref(
      const std::string& pref_name) const;

  // Updates preference value.
  SetPrefResult SetPref(const std::string& pref_name, const base::Value* value);

  // Modify list of observers for the given preference.
  void AddObserver(const std::string& pref_name,
                   GeneratedPref::Observer* observer);
  void RemoveObserver(const std::string& pref_name,
                      GeneratedPref::Observer* observer);

 private:
  // Returns preference implementation or nullptr if not found.
  GeneratedPref* FindPrefImpl(const std::string& pref_name) const;

  // Known preference map.
  PrefsMap prefs_;

  DISALLOW_COPY_AND_ASSIGN(GeneratedPrefs);
};

}  // namespace settings_private
}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_API_SETTINGS_PRIVATE_GENERATED_PREFS_H_
