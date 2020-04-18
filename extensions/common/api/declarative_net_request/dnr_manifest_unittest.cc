// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/json/json_file_value_serializer.h"
#include "base/strings/pattern.h"
#include "base/strings/string_util.h"
#include "components/version_info/version_info.h"
#include "extensions/common/api/declarative_net_request/constants.h"
#include "extensions/common/api/declarative_net_request/dnr_manifest_data.h"
#include "extensions/common/api/declarative_net_request/test_utils.h"
#include "extensions/common/constants.h"
#include "extensions/common/error_utils.h"
#include "extensions/common/extension.h"
#include "extensions/common/features/feature_channel.h"
#include "extensions/common/file_util.h"
#include "extensions/common/manifest_constants.h"
#include "extensions/common/value_builder.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace extensions {
namespace keys = manifest_keys;
namespace errors = manifest_errors;

namespace declarative_net_request {
namespace {

constexpr char kJSONRulesFilename[] = "rules_file.json";
const base::FilePath::CharType kJSONRulesetFilepath[] =
    FILE_PATH_LITERAL("rules_file.json");

std::string GetRuleResourcesKey() {
  return base::JoinString(
      {keys::kDeclarativeNetRequestKey, keys::kDeclarativeRuleResourcesKey},
      ".");
}

// Fixture testing the kDeclarativeNetRequestKey manifest key.
class DNRManifestTest : public testing::Test {
 public:
  DNRManifestTest() : channel_(::version_info::Channel::UNKNOWN) {}

 protected:
  // Overrides the default rules file path. |path| should be relative to the
  // extension directory.
  void SetRulesFilePath(base::FilePath path) {
    rules_file_path_ = std::move(path);
  }

  // Overrides the default manifest.
  void SetManifest(std::unique_ptr<base::Value> manifest) {
    manifest_ = std::move(manifest);
  }

  // Loads the extension and verifies the |expected_error|.
  void LoadAndExpectError(const base::StringPiece expected_error) {
    WriteManifestAndRuleset();
    std::string error;
    scoped_refptr<Extension> extension = file_util::LoadExtension(
        temp_dir_.GetPath(), Manifest::UNPACKED, Extension::NO_FLAGS, &error);
    EXPECT_FALSE(extension);
    EXPECT_EQ(expected_error, error);
  }

  // Loads the extension and verifies that the JSON ruleset location is
  // correctly set up.
  void LoadAndExpectSuccess() {
    WriteManifestAndRuleset();
    std::string error;
    scoped_refptr<Extension> extension = file_util::LoadExtension(
        temp_dir_.GetPath(), Manifest::UNPACKED, Extension::NO_FLAGS, &error);
    EXPECT_TRUE(extension);
    EXPECT_TRUE(error.empty());
    const ExtensionResource* resource =
        DNRManifestData::GetRulesetResource(extension.get());
    ASSERT_TRUE(resource);
    EXPECT_EQ(base::MakeAbsoluteFilePath(
                  temp_dir_.GetPath().Append(rules_file_path_)),
              resource->GetFilePath());
  }

 private:
  void WriteManifestAndRuleset() {
    EXPECT_TRUE(temp_dir_.CreateUniqueTempDir());

    base::FilePath rules_path = temp_dir_.GetPath().Append(rules_file_path_);

    // Create parent directory of |rules_path| if it doesn't exist.
    EXPECT_TRUE(base::CreateDirectory(rules_path.DirName()));

    // Persist an empty ruleset file.
    EXPECT_EQ(0, base::WriteFile(rules_path, nullptr /*data*/, 0 /*size*/));

    // Persist manifest file.
    JSONFileValueSerializer(temp_dir_.GetPath().Append(kManifestFilename))
        .Serialize(*manifest_);
  }

  std::unique_ptr<base::Value> manifest_ = CreateManifest(kJSONRulesFilename);
  base::FilePath rules_file_path_ = base::FilePath(kJSONRulesetFilepath);
  base::ScopedTempDir temp_dir_;
  ScopedCurrentChannel channel_;

  DISALLOW_COPY_AND_ASSIGN(DNRManifestTest);
};

TEST_F(DNRManifestTest, EmptyRuleset) {
  LoadAndExpectSuccess();
}

TEST_F(DNRManifestTest, InvalidManifestKey) {
  std::unique_ptr<base::DictionaryValue> manifest =
      CreateManifest(kJSONRulesFilename);
  manifest->SetInteger(keys::kDeclarativeNetRequestKey, 3);
  SetManifest(std::move(manifest));
  LoadAndExpectError(
      ErrorUtils::FormatErrorMessage(errors::kInvalidDeclarativeNetRequestKey,
                                     keys::kDeclarativeNetRequestKey));
}

TEST_F(DNRManifestTest, InvalidRulesFileKey) {
  std::unique_ptr<base::DictionaryValue> manifest =
      CreateManifest(kJSONRulesFilename);
  manifest->SetInteger(GetRuleResourcesKey(), 3);
  SetManifest(std::move(manifest));
  LoadAndExpectError(ErrorUtils::FormatErrorMessage(
      errors::kInvalidDeclarativeRulesFileKey, keys::kDeclarativeNetRequestKey,
      keys::kDeclarativeRuleResourcesKey));
}

TEST_F(DNRManifestTest, MultipleRulesFile) {
  std::unique_ptr<base::DictionaryValue> manifest =
      CreateManifest(kJSONRulesFilename);
  manifest->SetList(GetRuleResourcesKey(),
                    ToListValue({"file1.json", "file2.json"}));
  SetManifest(std::move(manifest));
  LoadAndExpectError(ErrorUtils::FormatErrorMessage(
      errors::kInvalidDeclarativeRulesFileKey, keys::kDeclarativeNetRequestKey,
      keys::kDeclarativeRuleResourcesKey));
}

TEST_F(DNRManifestTest, NonExistentRulesFile) {
  std::unique_ptr<base::DictionaryValue> manifest =
      CreateManifest(kJSONRulesFilename);
  manifest->SetList(GetRuleResourcesKey(), ToListValue({"invalid_file.json"}));
  SetManifest(std::move(manifest));
  LoadAndExpectError(ErrorUtils::FormatErrorMessage(
      errors::kRulesFileIsInvalid, keys::kDeclarativeNetRequestKey,
      keys::kDeclarativeRuleResourcesKey));
}

TEST_F(DNRManifestTest, NeedsDeclarativeNetRequestPermission) {
  std::unique_ptr<base::DictionaryValue> manifest =
      CreateManifest(kJSONRulesFilename);
  // Remove "declarativeNetRequest" permission.
  manifest->Remove(keys::kPermissions, nullptr);
  SetManifest(std::move(manifest));
  LoadAndExpectError(ErrorUtils::FormatErrorMessage(
      errors::kDeclarativeNetRequestPermissionNeeded, kAPIPermission,
      keys::kDeclarativeNetRequestKey));
}

TEST_F(DNRManifestTest, RulesFileInNestedDirectory) {
  base::FilePath nested_path =
      base::FilePath(FILE_PATH_LITERAL("dir")).Append(kJSONRulesetFilepath);
  SetRulesFilePath(nested_path);
  std::unique_ptr<base::DictionaryValue> manifest =
      CreateManifest(kJSONRulesFilename);
  manifest->SetList(GetRuleResourcesKey(),
                    ToListValue({"dir/rules_file.json"}));
  SetManifest(std::move(manifest));
  LoadAndExpectSuccess();
}

}  // namespace
}  // namespace declarative_net_request
}  // namespace extensions
