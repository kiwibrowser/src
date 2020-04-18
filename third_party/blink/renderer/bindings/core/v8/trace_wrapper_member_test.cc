// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/bindings/trace_wrapper_member.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/core/testing/death_aware_script_wrappable.h"

namespace blink {

namespace {

using Wrapper = TraceWrapperMember<DeathAwareScriptWrappable>;

}  // namespace

// These tests just ensure that entries are actually swapped. For write barrier
// testing see bindings/core/v8/ScriptWrappableVisitorTest.cpp.

TEST(TraceWrapperMemberTest, HeapVectorSwap) {
  HeapVector<Wrapper> vector1;
  DeathAwareScriptWrappable* entry1 = DeathAwareScriptWrappable::Create();
  vector1.push_back(entry1);

  HeapVector<Wrapper> vector2;
  DeathAwareScriptWrappable* entry2 = DeathAwareScriptWrappable::Create();
  vector2.push_back(entry2);

  swap(vector1, vector2);
  EXPECT_EQ(entry1, vector2.front());
  EXPECT_EQ(entry2, vector1.front());
}

TEST(TraceWrapperMemberTest, HeapVectorSwap2) {
  HeapVector<Wrapper> vector1;
  DeathAwareScriptWrappable* entry1 = DeathAwareScriptWrappable::Create();
  vector1.push_back(entry1);

  HeapVector<Member<DeathAwareScriptWrappable>> vector2;
  DeathAwareScriptWrappable* entry2 = DeathAwareScriptWrappable::Create();
  vector2.push_back(entry2);

  swap(vector1, vector2);
  EXPECT_EQ(entry1, vector2.front());
  EXPECT_EQ(entry2, vector1.front());
}

TEST(TraceWrapperMemberTest, HeapHashSet) {
  const size_t kContainerSize = 10000;
  HeapHashSet<Wrapper> set;
  // Loop enough so that underlying HashTable will rehash several times.
  for (size_t i = 1; i <= kContainerSize; ++i) {
    DeathAwareScriptWrappable* entry = DeathAwareScriptWrappable::Create();
    set.insert(entry);
  }
  EXPECT_EQ(kContainerSize, set.size());

  HeapHashSet<Wrapper> set2;
  swap(set, set2);
  EXPECT_EQ(0u, set.size());
  EXPECT_EQ(kContainerSize, set2.size());
}

TEST(TraceWrapperMemberTest, HeapHashMapValue) {
  const size_t kContainerSize = 10000;
  HeapHashMap<int, Wrapper> map;
  HeapVector<Wrapper> verification_vector;
  // Loop enough so that underlying HashTable will rehash several times.
  for (size_t i = 1; i <= kContainerSize; ++i) {
    DeathAwareScriptWrappable* entry = DeathAwareScriptWrappable::Create();
    map.insert(i, entry);
    verification_vector.push_back(entry);
  }
  EXPECT_EQ(kContainerSize, map.size());

  for (size_t i = 1; i <= kContainerSize; ++i) {
    auto wrapper = map.Take(i);
    EXPECT_EQ(verification_vector[i - 1], wrapper.Get());
  }
  EXPECT_EQ(0u, map.size());
}

}  // namespace blink
