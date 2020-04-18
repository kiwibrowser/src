// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_COMMON_FEATURES_FEATURE_UTIL_H_
#define EXTENSIONS_COMMON_FEATURES_FEATURE_UTIL_H_

#include "base/debug/alias.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/strings/string_util.h"

// Writes |message| to the stack so that it shows up in the minidump, then
// crashes the current process.
//
// The prefix "e::" is used so that the crash can be quickly located.
//
// This is provided in feature_util because for some reason features are prone
// to mysterious crashes in named map lookups. For example see crbug.com/365192
// and crbug.com/461915.
#define CRASH_WITH_MINIDUMP(message)                                           \
  {                                                                            \
    std::string message_copy(message);                                         \
    char minidump[BUFSIZ];                                                     \
    base::debug::Alias(&minidump);                                             \
    base::snprintf(minidump, arraysize(minidump), "e::%s:%d:\"%s\"", __FILE__, \
                   __LINE__, message_copy.c_str());                            \
    LOG(FATAL) << message_copy;                                                \
  }

namespace extensions {
namespace feature_util {

// Returns true if service workers are enabled for extension schemes.
// TODO(lazyboy): Remove this function once extension Service Workers
// are enabled by default for a while.
bool ExtensionServiceWorkersEnabled();

}  // namespace feature_util
}  // namespace extensions

#endif  // EXTENSIONS_COMMON_FEATURES_FEATURE_UTIL_H_
