// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/updater/extension_cache_fake.h"

#include "base/bind.h"
#include "base/stl_util.h"
#include "content/public/browser/browser_thread.h"

namespace extensions {

ExtensionCacheFake::ExtensionCacheFake() {
}

ExtensionCacheFake::~ExtensionCacheFake() {
}

void ExtensionCacheFake::Start(const base::Closure& callback) {
  content::BrowserThread::PostTask(
      content::BrowserThread::UI,
      FROM_HERE,
      callback);
}

void ExtensionCacheFake::Shutdown(const base::Closure& callback) {
  content::BrowserThread::PostTask(
      content::BrowserThread::UI,
      FROM_HERE,
      callback);
}

void ExtensionCacheFake::AllowCaching(const std::string& id) {
  allowed_extensions_.insert(id);
}

bool ExtensionCacheFake::GetExtension(const std::string& id,
                                      const std::string& expected_hash,
                                      base::FilePath* file_path,
                                      std::string* version) {
  Map::iterator it = cache_.find(id);
  if (it == cache_.end()) {
    return false;
  } else {
    if (version)
      *version = it->second.first;
    if (file_path)
      *file_path = it->second.second;
    return true;
  }
}

void ExtensionCacheFake::PutExtension(const std::string& id,
                                      const std::string& expected_hash,
                                      const base::FilePath& file_path,
                                      const std::string& version,
                                      const PutExtensionCallback& callback) {
  if (base::ContainsKey(allowed_extensions_, id)) {
    cache_[id].first = version;
    cache_[id].second = file_path;
    content::BrowserThread::PostTask(
        content::BrowserThread::UI, FROM_HERE,
        base::BindOnce(callback, file_path, false));
  } else {
    callback.Run(file_path, true);
  }
}

}  // namespace extensions
