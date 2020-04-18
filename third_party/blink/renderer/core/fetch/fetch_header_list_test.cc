// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/fetch/fetch_header_list.h"

#include <utility>
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/platform/wtf/std_lib_extras.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {
namespace {

TEST(FetchHeaderListTest, Append) {
  FetchHeaderList* headerList = FetchHeaderList::Create();
  headerList->Append("ConTenT-TyPe", "text/plain");
  headerList->Append("content-type", "application/xml");
  headerList->Append("CONTENT-type", "foo");
  headerList->Append("X-Foo", "bar");
  const std::pair<String, String> expectedHeaders[] = {
      std::make_pair("ConTenT-TyPe", "text/plain"),
      std::make_pair("ConTenT-TyPe", "application/xml"),
      std::make_pair("ConTenT-TyPe", "foo"), std::make_pair("X-Foo", "bar"),
  };
  EXPECT_EQ(arraysize(expectedHeaders), headerList->size());
  size_t i = 0;
  for (const auto& header : headerList->List()) {
    EXPECT_EQ(expectedHeaders[i].first, header.first);
    EXPECT_EQ(expectedHeaders[i].second, header.second);
    ++i;
  }
}

TEST(FetchHeaderListTest, Set) {
  FetchHeaderList* headerList = FetchHeaderList::Create();
  headerList->Append("ConTenT-TyPe", "text/plain");
  headerList->Append("content-type", "application/xml");
  headerList->Append("CONTENT-type", "foo");
  headerList->Append("X-Foo", "bar");
  headerList->Set("contENT-type", "quux");
  headerList->Set("some-header", "some value");
  EXPECT_EQ(3U, headerList->size());
  const std::pair<String, String> expectedHeaders[] = {
      std::make_pair("ConTenT-TyPe", "quux"),
      std::make_pair("some-header", "some value"),
      std::make_pair("X-Foo", "bar"),
  };
  EXPECT_EQ(arraysize(expectedHeaders), headerList->size());
  size_t i = 0;
  for (const auto& header : headerList->List()) {
    EXPECT_EQ(expectedHeaders[i].first, header.first);
    EXPECT_EQ(expectedHeaders[i].second, header.second);
    ++i;
  }
}

TEST(FetchHeaderListTest, Erase) {
  FetchHeaderList* headerList = FetchHeaderList::Create();
  headerList->Remove("foo");
  EXPECT_EQ(0U, headerList->size());
  headerList->Append("ConTenT-TyPe", "text/plain");
  headerList->Append("content-type", "application/xml");
  headerList->Append("CONTENT-type", "foo");
  headerList->Append("X-Foo", "bar");
  headerList->Remove("content-TYPE");
  EXPECT_EQ(1U, headerList->size());
  const std::pair<String, String> expectedHeaders[] = {
      std::make_pair("X-Foo", "bar"),
  };
  EXPECT_EQ(arraysize(expectedHeaders), headerList->size());
  size_t i = 0;
  for (const auto& header : headerList->List()) {
    EXPECT_EQ(expectedHeaders[i].first, header.first);
    EXPECT_EQ(expectedHeaders[i].second, header.second);
    ++i;
  }
}

TEST(FetchHeaderListTest, Combine) {
  FetchHeaderList* headerList = FetchHeaderList::Create();
  headerList->Append("ConTenT-TyPe", "text/plain");
  headerList->Append("content-type", "application/xml");
  headerList->Append("CONTENT-type", "foo");
  headerList->Append("X-Foo", "bar");
  String combinedValue;
  EXPECT_TRUE(headerList->Get("X-Foo", combinedValue));
  EXPECT_EQ("bar", combinedValue);
  EXPECT_TRUE(headerList->Get("content-TYPE", combinedValue));
  EXPECT_EQ("text/plain, application/xml, foo", combinedValue);
}

// This is going to be removed: see crbug.com/645492.
TEST(FetchHeaderListTest, GetAll) {
  FetchHeaderList* headerList = FetchHeaderList::Create();
  headerList->Append("ConTenT-TyPe", "text/plain");
  headerList->Append("content-type", "application/xml");
  headerList->Append("CONTENT-type", "foo");
  headerList->Append("X-Foo", "bar");
  Vector<String> combinedValues;
  headerList->GetAll("content-TYPE", combinedValues);
  EXPECT_EQ(Vector<String>({"text/plain", "application/xml", "foo"}),
            combinedValues);
  headerList->GetAll("x-foo", combinedValues);
  EXPECT_EQ(Vector<String>({"bar"}), combinedValues);
  headerList->GetAll("Host", combinedValues);
  EXPECT_TRUE(combinedValues.IsEmpty());
}

TEST(FetchHeaderListTest, Contains) {
  FetchHeaderList* headerList = FetchHeaderList::Create();
  headerList->Append("ConTenT-TyPe", "text/plain");
  headerList->Append("content-type", "application/xml");
  headerList->Append("X-Foo", "bar");
  EXPECT_TRUE(headerList->Has("CONTENT-TYPE"));
  EXPECT_TRUE(headerList->Has("X-Foo"));
  EXPECT_FALSE(headerList->Has("X-Bar"));
}

TEST(FetchHeaderListTest, ContainsNonCORSSafelistedHeader) {
  FetchHeaderList* headerList = FetchHeaderList::Create();
  EXPECT_FALSE(headerList->ContainsNonCORSSafelistedHeader());

  headerList->Append("Host", "foobar");
  headerList->Append("X-Foo", "bar");
  EXPECT_TRUE(headerList->ContainsNonCORSSafelistedHeader());

  headerList->ClearList();
  headerList->Append("ConTenT-TyPe", "text/plain");
  headerList->Append("content-type", "application/xml");
  headerList->Append("X-Foo", "bar");
  EXPECT_TRUE(headerList->ContainsNonCORSSafelistedHeader());

  headerList->ClearList();
  headerList->Append("ConTenT-TyPe", "multipart/form-data");
  headerList->Append("Accept", "xyz");
  EXPECT_FALSE(headerList->ContainsNonCORSSafelistedHeader());
}

TEST(FetchHeaderListTest, SortAndCombine) {
  FetchHeaderList* headerList = FetchHeaderList::Create();
  EXPECT_TRUE(headerList->SortAndCombine().IsEmpty());
  headerList->Append("content-type", "multipart/form-data");
  headerList->Append("ConTenT-TyPe", "application/xml");
  headerList->Append("Accept", "XYZ");
  headerList->Append("X-Foo", "bar");
  const std::pair<String, String> expectedHeaders[] = {
      std::make_pair("accept", "XYZ"),
      std::make_pair("content-type", "multipart/form-data, application/xml"),
      std::make_pair("x-foo", "bar")};
  const Vector<FetchHeaderList::Header> sortedAndCombined =
      headerList->SortAndCombine();
  EXPECT_EQ(arraysize(expectedHeaders), sortedAndCombined.size());
  size_t i = 0;
  for (const auto& headerPair : headerList->SortAndCombine()) {
    EXPECT_EQ(expectedHeaders[i].first, headerPair.first);
    EXPECT_EQ(expectedHeaders[i].second, headerPair.second);
    ++i;
  }
}

}  // namespace
}  // namespace blink
