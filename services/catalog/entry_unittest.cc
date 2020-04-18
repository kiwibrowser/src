// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/catalog/entry.h"

#include "base/files/file_path.h"
#include "base/json/json_file_value_serializer.h"
#include "base/macros.h"
#include "base/path_service.h"
#include "base/values.h"
#include "build/build_config.h"
#include "services/service_manager/public/cpp/interface_provider_spec.h"
#include "services/service_manager/public/mojom/interface_provider_spec.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace catalog {

class EntryTest : public testing::Test {
 public:
  EntryTest() {}
  ~EntryTest() override {}

 protected:
  std::unique_ptr<Entry> ReadEntry(const std::string& manifest,
                                   std::unique_ptr<base::Value>* out_value) {
    std::unique_ptr<base::Value> value = ReadManifest(manifest);
    base::DictionaryValue* dictionary = nullptr;
    CHECK(value->GetAsDictionary(&dictionary));
    if (out_value)
      *out_value = std::move(value);
    return Entry::Deserialize(*dictionary);
  }

  std::unique_ptr<base::Value> ReadManifest(const std::string& manifest) {
    base::FilePath manifest_path;
    base::PathService::Get(base::DIR_SOURCE_ROOT, &manifest_path);
    manifest_path =
        manifest_path.AppendASCII("services/catalog/test_data/" + manifest);

    JSONFileValueDeserializer deserializer(manifest_path);
    int error = 0;
    std::string message;
    // TODO(beng): probably want to do more detailed error checking. This should
    //             be done when figuring out if to unblock connection
    //             completion.
    return deserializer.Deserialize(&error, &message);
  }

 private:
  void SetUp() override {}
  void TearDown() override {}

  DISALLOW_COPY_AND_ASSIGN(EntryTest);
};

TEST_F(EntryTest, Simple) {
  std::unique_ptr<Entry> entry = ReadEntry("simple", nullptr);
  EXPECT_EQ("foo", entry->name());
  EXPECT_EQ("Foo", entry->display_name());
  EXPECT_EQ("none", entry->sandbox_type());
}

TEST_F(EntryTest, Instance) {
  std::unique_ptr<Entry> entry = ReadEntry("instance", nullptr);
  EXPECT_EQ("foo", entry->name());
  EXPECT_EQ("Foo", entry->display_name());
  EXPECT_EQ("", entry->sandbox_type());
}

TEST_F(EntryTest, ConnectionSpec) {
  std::unique_ptr<Entry> entry = ReadEntry("connection_spec", nullptr);

  EXPECT_EQ("foo", entry->name());
  EXPECT_EQ("Foo", entry->display_name());
  service_manager::InterfaceProviderSpec spec;
  service_manager::CapabilitySet capabilities;
  capabilities.insert("bar:bar");
  spec.requires["bar"] = capabilities;
  service_manager::InterfaceProviderSpecMap specs;
  specs[service_manager::mojom::kServiceManager_ConnectorSpec] = spec;
  EXPECT_EQ(specs, entry->interface_provider_specs());
}

TEST_F(EntryTest, RequiredFiles) {
  std::unique_ptr<Entry> entry = ReadEntry("required_files", nullptr);
  EXPECT_EQ("foo", entry->name());
  EXPECT_EQ("Foo", entry->display_name());
  auto required_files = entry->required_file_paths();
  EXPECT_EQ(2U, required_files.size());
  auto iter = required_files.find("all_platforms");
  ASSERT_NE(required_files.end(), iter);
  bool checked_platform_specific_file = false;
#if defined(OS_WIN)
  EXPECT_EQ(base::FilePath(L"/all/platforms/windows"), iter->second);
  iter = required_files.find("windows_only");
  ASSERT_NE(required_files.end(), iter);
  EXPECT_EQ(base::FilePath(L"/windows/only"), iter->second);
  checked_platform_specific_file = true;
#elif defined(OS_FUCHSIA)
  EXPECT_EQ(base::FilePath("/all/platforms/fuchsia"), iter->second);
  iter = required_files.find("fuchsia_only");
  ASSERT_NE(required_files.end(), iter);
  EXPECT_EQ(base::FilePath("/fuchsia/only"), iter->second);
  checked_platform_specific_file = true;
#elif defined(OS_LINUX)
  EXPECT_EQ(base::FilePath("/all/platforms/linux"), iter->second);
  iter = required_files.find("linux_only");
  ASSERT_NE(required_files.end(), iter);
  EXPECT_EQ(base::FilePath("/linux/only"), iter->second);
  checked_platform_specific_file = true;
#elif defined(OS_MACOSX)
  EXPECT_EQ(base::FilePath("/all/platforms/macosx"), iter->second);
  iter = required_files.find("macosx_only");
  ASSERT_NE(required_files.end(), iter);
  EXPECT_EQ(base::FilePath("/macosx/only"), iter->second);
  checked_platform_specific_file = true;
#elif defined(OS_ANDROID)
  EXPECT_EQ(base::FilePath("/all/platforms/android"), iter->second);
  iter = required_files.find("android_only");
  ASSERT_NE(required_files.end(), iter);
  EXPECT_EQ(base::FilePath("/android/only"), iter->second);
  checked_platform_specific_file = true;
#endif
  EXPECT_TRUE(checked_platform_specific_file);
}

TEST_F(EntryTest, Malformed) {
  std::unique_ptr<base::Value> value = ReadManifest("malformed");
  EXPECT_FALSE(value.get());
}


}  // namespace catalog
