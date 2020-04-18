// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// This file defines dummy implementation of several functions from the
// master_preferences namespace for Google Chrome. These functions allow 64-bit
// Windows Chrome binary to build successfully. Since this binary is only used
// for Native Client support which uses the 32 bit installer, most of the
// master preferences functionality is not actually needed.

#include "chrome/installer/util/master_preferences.h"

#include "base/logging.h"
#include "base/values.h"

namespace installer {

MasterPreferences::MasterPreferences(const base::CommandLine& cmd_line)
    : distribution_(NULL), preferences_read_from_file_(false) {
}

MasterPreferences::MasterPreferences(const base::FilePath& prefs_path)
    : distribution_(NULL), preferences_read_from_file_(false) {
}

MasterPreferences::~MasterPreferences() {
}

bool MasterPreferences::GetBool(const std::string& name, bool* value) const {
  NOTREACHED();
  return false;
}

bool MasterPreferences::GetInt(const std::string& name, int* value) const {
  NOTREACHED();
  return false;
}

bool MasterPreferences::GetString(const std::string& name,
                                  std::string* value) const {
  NOTREACHED();
  return false;
}

std::vector<std::string> MasterPreferences::GetFirstRunTabs() const {
  NOTREACHED();
  return std::vector<std::string>();
}

// static
const MasterPreferences& MasterPreferences::ForCurrentProcess() {
  static MasterPreferences prefs(*base::CommandLine::ForCurrentProcess());
  return prefs;
}

}  // namespace installer
