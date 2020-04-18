// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/service_manager/embedder/manifest_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace service_manager {

TEST(MergeManifestWithOverlayTest, Merge) {
  // |manifest| & |overlay| have three properties, "string", "list" and
  // "dictionary", which are then merged.
  base::DictionaryValue manifest;
  manifest.SetString("string", "Hello, ");
  std::unique_ptr<base::DictionaryValue> dict_value_original(
      std::make_unique<base::DictionaryValue>());
  dict_value_original->SetString("key1", "original");
  dict_value_original->SetString("key3", "original");
  manifest.Set("dictionary", std::move(dict_value_original));
  std::unique_ptr<base::ListValue> list(std::make_unique<base::ListValue>());
  list->AppendString("A");
  list->AppendString("B");
  manifest.Set("list", std::move(list));

  base::DictionaryValue overlay;
  overlay.SetString("string", "World!");
  std::unique_ptr<base::DictionaryValue> dict_value_replacement(
      std::make_unique<base::DictionaryValue>());
  dict_value_replacement->SetString("key1", "new");
  dict_value_replacement->SetString("key2", "new");
  overlay.Set("dictionary", std::move(dict_value_replacement));
  list = std::make_unique<base::ListValue>();
  list->AppendString("C");
  overlay.Set("list", std::move(list));

  MergeManifestWithOverlay(&manifest, &overlay);

  // Simple string value should have been clobbered.
  std::string out_string;
  EXPECT_TRUE(manifest.GetString("string", &out_string));
  EXPECT_EQ(out_string, "World!");

  // Dictionary should have been merged, with key1 being clobbered, key2 added
  // and key3 preserved.
  base::DictionaryValue* out_dictionary = nullptr;
  EXPECT_TRUE(manifest.GetDictionary("dictionary", &out_dictionary));
  EXPECT_EQ(3u, out_dictionary->size());
  std::string value1, value2, value3;
  EXPECT_TRUE(out_dictionary->GetString("key1", &value1));
  EXPECT_TRUE(out_dictionary->GetString("key2", &value2));
  EXPECT_TRUE(out_dictionary->GetString("key3", &value3));
  EXPECT_EQ(value1, "new");
  EXPECT_EQ(value2, "new");
  EXPECT_EQ(value3, "original");

  // List should have been merged, with the items from overlay appended to the
  // items from manifest.
  base::ListValue* out_list = nullptr;
  EXPECT_TRUE(manifest.GetList("list", &out_list));
  EXPECT_EQ(3u, out_list->GetSize());
  std::string a, b, c;
  EXPECT_TRUE(out_list->GetString(0, &a));
  EXPECT_TRUE(out_list->GetString(1, &b));
  EXPECT_TRUE(out_list->GetString(2, &c));
  EXPECT_EQ("A", a);
  EXPECT_EQ("B", b);
  EXPECT_EQ("C", c);
}

}  // namespace service_manager
