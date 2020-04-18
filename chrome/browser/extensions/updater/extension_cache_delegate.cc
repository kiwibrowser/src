// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/updater/extension_cache_delegate.h"

namespace extensions {
namespace {

// Default maximum size of local cache on disk, in bytes.
const size_t kDefaultCacheSizeLimit = 256 * 1024 * 1024;

// Default minimum size of local cache on disk, in bytes.
const int kDefaultMinimumCacheSize = 1024 * 1024;

// Maximum age of unused extensions in the cache.
const int kMaxCacheAgeInDays = 30;

}  // namespace

ExtensionCacheDelegate::~ExtensionCacheDelegate() {
}

size_t ExtensionCacheDelegate::GetMinimumCacheSize() const {
  return kDefaultMinimumCacheSize;
}

size_t ExtensionCacheDelegate::GetMaximumCacheSize() const {
  return kDefaultCacheSizeLimit;
}

base::TimeDelta ExtensionCacheDelegate::GetMaximumCacheAge() const {
  return base::TimeDelta::FromDays(kMaxCacheAgeInDays);
}

}  // namespace extensions
