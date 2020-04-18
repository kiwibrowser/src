// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_UTIL_CHANNEL_INFO_H_
#define CHROME_INSTALLER_UTIL_CHANNEL_INFO_H_

#include "base/strings/string16.h"

namespace base {
namespace win {
class RegKey;
}
}

namespace installer {

// A helper class for parsing and modifying the Google Update additional
// parameter ("ap") client state value for a product.
class ChannelInfo {
 public:

  // Initialize an instance from the "ap" value in a given registry key.
  // Returns false if the value is present but could not be read from the
  // registry. Returns true if the value was not present or could be read.
  // Also returns true if the key is not valid.
  // An absent "ap" value is treated identically to an empty "ap" value.
  bool Initialize(const base::win::RegKey& key);

  // Writes the info to the "ap" value in a given registry key.
  // Returns false if the value could not be written to the registry.
  bool Write(base::win::RegKey* key) const;

  const base::string16& value() const { return value_; }
  void set_value(const base::string16& value) { value_ = value; }
  bool Equals(const ChannelInfo& other) const {
    return value_ == other.value_;
  }

  // Returns true if the -chrome modifier is present in the value.
  bool IsChrome() const;

  // Adds or removes the -chrome modifier, returning true if the value is
  // modified.
  bool SetChrome(bool value);

  // Returns true if the -chromeframe modifier is present in the value.
  bool IsChromeFrame() const;

  // Adds or removes the -chromeframe modifier, returning true if the value is
  // modified.
  bool SetChromeFrame(bool value);

  // (Deprecated) Returns true if the -applauncher modifier is present in the
  // value.
  bool IsAppLauncher() const;

  // (Deprecated) Adds or removes the -applauncher modifier, returning true if
  // the value is modified.
  bool SetAppLauncher(bool value);

  // Returns true if the -multi modifier is present in the value.
  bool IsMultiInstall() const;

  // Adds or removes the -multi modifier, returning true if the value is
  // modified.
  bool SetMultiInstall(bool value);

  // Returns true if the -readymode modifier is present in the value.
  bool IsReadyMode() const;

  // Adds or removes the -readymode modifier, returning true if the value is
  // modified.
  bool SetReadyMode(bool value);

  // Removes the -stage: modifier, returning true if the value is modified.
  bool ClearStage();

  // Returns the string identifying the stats default state (i.e., the starting
  // value of the "send usage stats" checkbox during install), or an empty
  // string if the -statsdef_ modifier is not present in the value.
  base::string16 GetStatsDefault() const;

  // Returns true if the -full suffix is present in the value.
  bool HasFullSuffix() const;

  // Adds or removes the -full suffix, returning true if the value is
  // modified.
  bool SetFullSuffix(bool value);

  // Returns true if the -multifail suffix is present in the value.
  bool HasMultiFailSuffix() const;

  // Adds or removes the -multifail suffix, returning true if the value is
  // modified.
  bool SetMultiFailSuffix(bool value);

  // Adds or removes the -migrating suffix, returning true if the value is
  // modified.
  bool SetMigratingSuffix(bool value);

  // Returns true if the -migrating suffix is present in the value.
  bool HasMigratingSuffix() const;

  // Removes all modifiers and suffixes. For example, 2.0-dev-multi-chrome-full
  // becomes 2.0-dev. Returns true if the value is modified.
  bool RemoveAllModifiersAndSuffixes();

 private:
  base::string16 value_;
};  // class ChannelInfo

}  // namespace installer

#endif  // CHROME_INSTALLER_UTIL_CHANNEL_INFO_H_
