// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/updater/null_extension_cache.h"

#include "base/callback.h"

namespace extensions {

NullExtensionCache::NullExtensionCache() {
}

NullExtensionCache::~NullExtensionCache() {
}

void NullExtensionCache::Start(const base::Closure& callback) {
  callback.Run();
}

void NullExtensionCache::Shutdown(const base::Closure& callback) {
  callback.Run();
}

void NullExtensionCache::AllowCaching(const std::string& id) {
}

bool NullExtensionCache::GetExtension(const std::string& id,
                                      const std::string& expected_hash,
                                      base::FilePath* file_path,
                                      std::string* version) {
  return false;
}

void NullExtensionCache::PutExtension(const std::string& id,
                                      const std::string& expected_hash,
                                      const base::FilePath& file_path,
                                      const std::string& version,
                                      const PutExtensionCallback& callback) {
  callback.Run(file_path, true);
}

}  // namespace extensions
