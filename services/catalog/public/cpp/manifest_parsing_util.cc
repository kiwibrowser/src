// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/catalog/public/cpp/manifest_parsing_util.h"

#include "base/values.h"
#include "build/build_config.h"
#include "services/catalog/store.h"

namespace catalog {

namespace {

bool IsValidPlatformName(const std::string& name) {
  return name == Store::kRequiredFilesKey_PlatformValue_Windows ||
         name == Store::kRequiredFilesKey_PlatformValue_Linux ||
         name == Store::kRequiredFilesKey_PlatformValue_MacOSX ||
         name == Store::kRequiredFilesKey_PlatformValue_Android ||
         name == Store::kRequiredFilesKey_PlatformValue_Fuchsia;
}

bool IsCurrentPlatform(const std::string& name) {
#if defined(OS_WIN)
  return name == Store::kRequiredFilesKey_PlatformValue_Windows;
#elif defined(OS_LINUX)
  return name == Store::kRequiredFilesKey_PlatformValue_Linux;
#elif defined(OS_MACOSX)
  return name == Store::kRequiredFilesKey_PlatformValue_MacOSX;
#elif defined(OS_ANDROID)
  return name == Store::kRequiredFilesKey_PlatformValue_Android;
#elif defined(OS_FUCHSIA)
  return name == Store::kRequiredFilesKey_PlatformValue_Fuchsia;
#else
#error This architecture is not supported.
#endif
}

}  // namespace

base::Optional<RequiredFileMap> RetrieveRequiredFiles(
    const base::Value& manifest_value) {
  const base::DictionaryValue* manifest_dictionary = nullptr;
  if (!manifest_value.GetAsDictionary(&manifest_dictionary)) {
    DLOG(ERROR) << "Entry::Deserialize: manifest node is not a dictionary.";
    return base::nullopt;
  }

  RequiredFileMap required_files;
  if (!manifest_dictionary->HasKey(Store::kRequiredFilesKey))
    return {required_files};

  const base::DictionaryValue* required_files_value = nullptr;
  if (!manifest_dictionary->GetDictionary(Store::kRequiredFilesKey,
                                          &required_files_value)) {
    DLOG(ERROR) << "Entry::Deserialize: RequiredFiles not a dictionary.";
    return base::nullopt;
  }

  base::DictionaryValue::Iterator it(*required_files_value);
  for (; !it.IsAtEnd(); it.Advance()) {
    const std::string& entry_name = it.key();
    const base::ListValue* all_platform_values = nullptr;
    if (!it.value().GetAsList(&all_platform_values)) {
      DLOG(ERROR) << "Entry::Deserialize: value of RequiredFiles for key: "
                  << entry_name << " not a list.";
      return base::nullopt;
    }

    for (size_t i = 0; i < all_platform_values->GetSize(); i++) {
      const base::DictionaryValue* file_descriptor_value = nullptr;
      if (!all_platform_values->GetDictionary(i, &file_descriptor_value)) {
        DLOG(ERROR) << "Entry::Deserialize: value of entry at index " << i
                    << " of RequiredFiles for key: " << entry_name
                    << " not a dictionary.";
        return base::nullopt;
      }
      std::string platform;
      if (file_descriptor_value->GetString(Store::kRequiredFilesKey_PlatformKey,
                                           &platform)) {
        if (!IsValidPlatformName(platform)) {
          DLOG(ERROR) << "Entry::Deserialize: value of platform for "
                      << "required file entry entry is invalid " << platform;
          return base::nullopt;
        }
      }
      if (!IsCurrentPlatform(platform)) {
        continue;
      }
      base::FilePath::StringType path;
      if (!file_descriptor_value->GetString(Store::kRequiredFilesKey_PathKey,
                                            &path)) {
        DLOG(ERROR) << "Entry::Deserialize: value of RequiredFiles entry for "
                    << "key: " << entry_name
                    << " missing: " << Store::kRequiredFilesKey_PathKey
                    << " value.";
        return base::nullopt;
      }
      if (required_files.count(entry_name) > 0) {
        DLOG(ERROR) << "Entry::Deserialize: value of RequiredFiles entry for "
                    << "key: " << entry_name << " has more than one value for "
                    << "platform: " << platform;
        return base::nullopt;
      }
      required_files[entry_name] = base::FilePath(path);
    }
  }
  return base::make_optional(std::move(required_files));
}

}  // namespace content
