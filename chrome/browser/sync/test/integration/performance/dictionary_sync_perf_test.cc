// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include "base/macros.h"
#include "base/strings/string_number_conversions.h"
#include "chrome/browser/sync/test/integration/dictionary_helper.h"
#include "chrome/browser/sync/test/integration/performance/sync_timing_helper.h"
#include "chrome/browser/sync/test/integration/profile_sync_service_harness.h"
#include "chrome/browser/sync/test/integration/sync_test.h"
#include "components/spellcheck/common/spellcheck_common.h"

using sync_timing_helper::PrintResult;
using sync_timing_helper::TimeMutualSyncCycle;

class DictionarySyncPerfTest : public SyncTest {
 public:
  DictionarySyncPerfTest() : SyncTest(TWO_CLIENT) {}
  ~DictionarySyncPerfTest() override {}

 private:
  DISALLOW_COPY_AND_ASSIGN(DictionarySyncPerfTest);
};

IN_PROC_BROWSER_TEST_F(DictionarySyncPerfTest, P0) {
  ASSERT_TRUE(SetupSync()) << "SetupSync() failed.";
  dictionary_helper::LoadDictionaries();
  ASSERT_TRUE(dictionary_helper::DictionariesMatch());

  base::TimeDelta dt;
  for (size_t i = 0; i < spellcheck::kMaxSyncableDictionaryWords; ++i) {
    ASSERT_TRUE(dictionary_helper::AddWord(0, "foo" + base::NumberToString(i)));
  }
  dt = TimeMutualSyncCycle(GetClient(0), GetClient(1));
  ASSERT_EQ(spellcheck::kMaxSyncableDictionaryWords,
            dictionary_helper::GetDictionarySize(1));
  PrintResult("dictionary", "add_words", dt);

  for (size_t i = 0; i < spellcheck::kMaxSyncableDictionaryWords; ++i) {
    ASSERT_TRUE(
        dictionary_helper::RemoveWord(0, "foo" + base::NumberToString(i)));
  }
  dt = TimeMutualSyncCycle(GetClient(0), GetClient(1));
  ASSERT_EQ(0UL, dictionary_helper::GetDictionarySize(1));
  PrintResult("dictionary", "remove_words", dt);
}
