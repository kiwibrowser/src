// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/service_manager/service_overrides.h"

#include "base/base_paths.h"
#include "base/path_service.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"

namespace service_manager {

namespace {

const char kExecutablePathKey[] = "executable_path";
const char kPackageNameKey[] = "package_name";

}  // namespace

ServiceOverrides::Entry::Entry() {}

ServiceOverrides::Entry::~Entry() {}

ServiceOverrides::ServiceOverrides(std::unique_ptr<base::Value> overrides) {
  const base::DictionaryValue* services;
  if (!overrides->GetAsDictionary(&services)) {
    LOG(ERROR) << "Expected top-level dictionary.";
    return;
  }

  base::DictionaryValue::Iterator service_iter(*services);
  for (; !service_iter.IsAtEnd(); service_iter.Advance()) {
    Entry& new_entry = entries_[service_iter.key()];

    const base::DictionaryValue* value;
    if (!service_iter.value().GetAsDictionary(&value)) {
      LOG(ERROR) << "Expected service entry to be a dictionary.";
      return;
    }

    std::string executable_path_value;
    if (value->GetString(kExecutablePathKey, &executable_path_value)) {
      base::FilePath exe_dir;
      CHECK(base::PathService::Get(base::DIR_EXE, &exe_dir));
#if defined(OS_WIN)
      executable_path_value += ".exe";
      base::ReplaceFirstSubstringAfterOffset(
          &executable_path_value, 0, "@EXE_DIR",
          base::UTF16ToUTF8(exe_dir.value()));
      new_entry.executable_path =
          base::FilePath(base::UTF8ToUTF16(executable_path_value));
#else
      base::ReplaceFirstSubstringAfterOffset(
          &executable_path_value, 0, "@EXE_DIR",
          exe_dir.value());
      new_entry.executable_path = base::FilePath(executable_path_value);
#endif
    }

    value->GetString(kPackageNameKey, &new_entry.package_name);
  }
}

ServiceOverrides::~ServiceOverrides() {}

bool ServiceOverrides::GetExecutablePathOverride(
    const std::string& service_name,
    base::FilePath* path) const {
  auto iter = entries_.find(service_name);
  if (iter == entries_.end() || iter->second.executable_path.empty())
    return false;

  *path = iter->second.executable_path;
  return true;
}

}  // namespace service_manager
