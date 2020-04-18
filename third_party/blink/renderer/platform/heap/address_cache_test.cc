// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/heap/address_cache.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/platform/heap/heap_page.h"

namespace blink {

namespace {

const Address kObjectAddress = reinterpret_cast<Address>(kBlinkPageSize);

}  // namespace

TEST(AddressCacheTest, InitialIsEmpty) {
  AddressCache cache;
  cache.EnableLookup();
  EXPECT_TRUE(cache.IsEmpty());
}

TEST(AddressCacheTest, LookupOnEmpty) {
  AddressCache cache;
  cache.EnableLookup();
  EXPECT_FALSE(cache.Lookup(kObjectAddress));
}

TEST(AddressCacheTest, LookupAfterAddEntry) {
  AddressCache cache;
  cache.EnableLookup();
  cache.AddEntry(kObjectAddress);
  EXPECT_TRUE(cache.Lookup(kObjectAddress));
}

TEST(AddressCacheTest, AddEntryAddsWholePage) {
  AddressCache cache;
  cache.EnableLookup();
  cache.AddEntry(kObjectAddress);
  for (Address current = kObjectAddress;
       current < (kObjectAddress + kBlinkPageSize); current++) {
    EXPECT_TRUE(cache.Lookup(current));
  }
}

TEST(AddressCacheTest, AddEntryOnlyAddsPageForGivenAddress) {
  AddressCache cache;
  cache.EnableLookup();
  cache.AddEntry(kObjectAddress);
  EXPECT_FALSE(cache.Lookup(kObjectAddress - 1));
  EXPECT_FALSE(cache.Lookup(kObjectAddress + kBlinkPageSize + 1));
}

TEST(AddressCacheTest, FlushIfDirtyIgnoresNonDirty) {
  AddressCache cache;
  cache.EnableLookup();
  cache.AddEntry(kObjectAddress);
  cache.FlushIfDirty();
  // Cannot do lookup in dirty cache.
  EXPECT_FALSE(cache.IsEmpty());
}

TEST(AddressCacheTest, FlushIfDirtyHandlesDirty) {
  AddressCache cache;
  cache.EnableLookup();
  cache.AddEntry(kObjectAddress);
  cache.MarkDirty();
  cache.FlushIfDirty();
  // Cannot do lookup in dirty cache.
  EXPECT_TRUE(cache.IsEmpty());
}

}  // namespace blink
