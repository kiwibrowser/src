// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/preferences/unittest_common.h"

namespace prefs {

PrefStoreObserverMock::PrefStoreObserverMock() {}
PrefStoreObserverMock::~PrefStoreObserverMock() {}

void ExpectPrefChange(PrefStore* pref_store, base::StringPiece key) {
  PrefStoreObserverMock observer;
  pref_store->AddObserver(&observer);
  base::RunLoop run_loop;
  EXPECT_CALL(observer, OnPrefValueChanged(key.as_string()))
      .WillOnce(testing::WithoutArgs(
          testing::Invoke([&run_loop]() { run_loop.Quit(); })));
  run_loop.Run();
  pref_store->RemoveObserver(&observer);
}

}  // namespace prefs
