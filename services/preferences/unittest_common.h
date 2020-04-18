// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_PREFERENCES_UNITTEST_COMMON_H_
#define SERVICES_PREFERENCES_UNITTEST_COMMON_H_

#include "base/run_loop.h"
#include "base/strings/string_piece.h"
#include "components/prefs/pref_store.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace prefs {

constexpr int kInitialValue = 1;
constexpr char kKey[] = "some_key";
constexpr char kDictionaryKey[] = "a.dictionary.pref";
constexpr char kOtherDictionaryKey[] = "another.dictionary.pref";

class PrefStoreObserverMock : public PrefStore::Observer {
 public:
  PrefStoreObserverMock();
  ~PrefStoreObserverMock() override;
  MOCK_METHOD1(OnPrefValueChanged, void(const std::string&));
  MOCK_METHOD1(OnInitializationCompleted, void(bool));
};

// Runs a nested runloop until the value of |key| changes in |pref_store|
// and then returns.
void ExpectPrefChange(PrefStore* pref_store, base::StringPiece key);

}  // namespace prefs

#endif  // SERVICES_PREFERENCES_UNITTEST_COMMON_H_
