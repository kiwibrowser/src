// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/indexed_db/leveldb/leveldb_env.h"

namespace {

// Static member initialization.
base::LazyInstance<content::LevelDBEnv>::Leaky g_leveldb_env =
    LAZY_INSTANCE_INITIALIZER;

}  // namespace

namespace content {

LevelDBEnv::LevelDBEnv() : ChromiumEnv("LevelDBEnv.IDB") {}

LevelDBEnv* LevelDBEnv::Get() {
  return g_leveldb_env.Pointer();
}

}  // namespace content
