// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/html/forms/form_data.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace blink {

TEST(FormDataTest, get) {
  FormData* fd = FormData::Create(UTF8Encoding());
  fd->append("name1", "value1");

  FileOrUSVString result;
  fd->get("name1", result);
  EXPECT_TRUE(result.IsUSVString());
  EXPECT_EQ("value1", result.GetAsUSVString());

  const FormData::Entry& entry = *fd->Entries()[0];
  EXPECT_EQ("name1", entry.name());
  EXPECT_EQ("value1", entry.Value());
}

TEST(FormDataTest, getAll) {
  FormData* fd = FormData::Create(UTF8Encoding());
  fd->append("name1", "value1");

  HeapVector<FormDataEntryValue> results = fd->getAll("name1");
  EXPECT_EQ(1u, results.size());
  EXPECT_TRUE(results[0].IsUSVString());
  EXPECT_EQ("value1", results[0].GetAsUSVString());

  EXPECT_EQ(1u, fd->size());
}

TEST(FormDataTest, has) {
  FormData* fd = FormData::Create(UTF8Encoding());
  fd->append("name1", "value1");

  EXPECT_TRUE(fd->has("name1"));
  EXPECT_EQ(1u, fd->size());
}

}  // namespace blink
