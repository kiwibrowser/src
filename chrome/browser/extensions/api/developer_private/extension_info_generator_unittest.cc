// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/developer_private/extension_info_generator.h"

#include <memory>
#include <string>
#include <utility>

#include "base/callback_helpers.h"
#include "base/json/json_file_value_serializer.h"
#include "base/json/json_writer.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/scoped_feature_list.h"
#include "build/build_config.h"
#include "chrome/browser/extensions/api/developer_private/inspectable_views_finder.h"
#include "chrome/browser/extensions/chrome_test_extension_loader.h"
#include "chrome/browser/extensions/error_console/error_console.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/extension_service_test_base.h"
#include "chrome/browser/extensions/extension_util.h"
#include "chrome/browser/extensions/permissions_updater.h"
#include "chrome/browser/extensions/scripting_permissions_modifier.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/extensions/api/developer_private.h"
#include "chrome/common/pref_names.h"
#include "components/crx_file/id_util.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_builder.h"
#include "extensions/common/extension_features.h"
#include "extensions/common/feature_switch.h"
#include "extensions/common/permissions/permission_message.h"
#include "extensions/common/permissions/permissions_data.h"
#include "extensions/common/value_builder.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace extensions {

namespace developer = api::developer_private;

namespace {

const char kAllHostsPermission[] = "*://*/*";

std::unique_ptr<base::DictionaryValue> DeserializeJSONTestData(
    const base::FilePath& path,
    std::string* error) {
  JSONFileValueDeserializer deserializer(path);
  return base::DictionaryValue::From(deserializer.Deserialize(nullptr, error));
}

// Returns a pointer to the ExtensionInfo for an extension with |id| if it
// is present in |list|.
const developer::ExtensionInfo* GetInfoFromList(
    const ExtensionInfoGenerator::ExtensionInfoList& list,
    const std::string& id) {
  for (const auto& item : list) {
    if (item.id == id)
      return &item;
  }
  return nullptr;
}

}  // namespace

class ExtensionInfoGeneratorUnitTest : public ExtensionServiceTestBase {
 public:
  ExtensionInfoGeneratorUnitTest() {}
  ~ExtensionInfoGeneratorUnitTest() override {}

 protected:
  void SetUp() override {
    ExtensionServiceTestBase::SetUp();
    InitializeEmptyExtensionService();
  }

  void OnInfoGenerated(std::unique_ptr<developer::ExtensionInfo>* info_out,
                       ExtensionInfoGenerator::ExtensionInfoList list) {
    EXPECT_EQ(1u, list.size());
    if (!list.empty())
      info_out->reset(new developer::ExtensionInfo(std::move(list[0])));
    std::move(quit_closure_).Run();
  }

  std::unique_ptr<developer::ExtensionInfo> GenerateExtensionInfo(
      const std::string& extension_id) {
    std::unique_ptr<developer::ExtensionInfo> info;
    base::RunLoop run_loop;
    quit_closure_ = run_loop.QuitClosure();
    std::unique_ptr<ExtensionInfoGenerator> generator(
        new ExtensionInfoGenerator(browser_context()));
    generator->CreateExtensionInfo(
        extension_id,
        base::Bind(&ExtensionInfoGeneratorUnitTest::OnInfoGenerated,
                   base::Unretained(this), base::Unretained(&info)));
    run_loop.Run();
    return info;
  }

  void OnInfosGenerated(ExtensionInfoGenerator::ExtensionInfoList* out,
                        ExtensionInfoGenerator::ExtensionInfoList list) {
    *out = std::move(list);
    std::move(quit_closure_).Run();
  }

  ExtensionInfoGenerator::ExtensionInfoList GenerateExtensionsInfo() {
    base::RunLoop run_loop;
    quit_closure_ = run_loop.QuitClosure();
    ExtensionInfoGenerator generator(browser_context());
    ExtensionInfoGenerator::ExtensionInfoList result;
    generator.CreateExtensionsInfo(
        true, /* include_disabled */
        true, /* include_terminated */
        base::Bind(&ExtensionInfoGeneratorUnitTest::OnInfosGenerated,
                   base::Unretained(this), base::Unretained(&result)));
    run_loop.Run();
    return result;
  }

  const scoped_refptr<const Extension> CreateExtension(
      const std::string& name,
      std::unique_ptr<base::ListValue> permissions,
      Manifest::Location location) {
    const std::string kId = crx_file::id_util::GenerateId(name);
    scoped_refptr<const Extension> extension =
        ExtensionBuilder()
            .SetManifest(DictionaryBuilder()
                             .Set("name", name)
                             .Set("description", "an extension")
                             .Set("manifest_version", 2)
                             .Set("version", "1.0.0")
                             .Set("permissions", std::move(permissions))
                             .Build())
            .SetLocation(location)
            .SetID(kId)
            .Build();

    ExtensionRegistry::Get(profile())->AddEnabled(extension);
    PermissionsUpdater(profile()).InitializePermissions(extension.get());
    return extension;
  }

  std::unique_ptr<developer::ExtensionInfo> CreateExtensionInfoFromPath(
      const base::FilePath& extension_path,
      Manifest::Location location) {
    ChromeTestExtensionLoader loader(browser_context());
    loader.set_location(location);
    loader.set_creation_flags(Extension::REQUIRE_KEY);
    scoped_refptr<const Extension> extension =
        loader.LoadExtension(extension_path);
    CHECK(extension.get());

    return GenerateExtensionInfo(extension->id());
  }

  void CompareExpectedAndActualOutput(
      const base::FilePath& extension_path,
      InspectableViewsFinder::ViewList views,
      const base::FilePath& expected_output_path) {
    std::string error;
    std::unique_ptr<base::DictionaryValue> expected_output_data(
        DeserializeJSONTestData(expected_output_path, &error));
    EXPECT_EQ(std::string(), error);

    // Produce test output.
    std::unique_ptr<developer::ExtensionInfo> info =
        CreateExtensionInfoFromPath(extension_path, Manifest::UNPACKED);
    info->views = std::move(views);
    std::unique_ptr<base::DictionaryValue> actual_output_data = info->ToValue();
    ASSERT_TRUE(actual_output_data);

    // Compare the outputs.
    // Ignore unknown fields in the actual output data.
    std::string paths_details = " - expected (" +
        expected_output_path.MaybeAsASCII() + ") vs. actual (" +
        extension_path.MaybeAsASCII() + ")";
    std::string expected_string;
    std::string actual_string;
    for (base::DictionaryValue::Iterator field(*expected_output_data);
         !field.IsAtEnd(); field.Advance()) {
      const base::Value& expected_value = field.value();
      base::Value* actual_value = nullptr;
      EXPECT_TRUE(actual_output_data->Get(field.key(), &actual_value)) <<
          field.key() + " is missing" + paths_details;
      if (!actual_value)
        continue;
      if (!actual_value->Equals(&expected_value)) {
        base::JSONWriter::Write(expected_value, &expected_string);
        base::JSONWriter::Write(*actual_value, &actual_string);
        EXPECT_EQ(expected_string, actual_string) <<
            field.key() << paths_details;
      }
    }
  }

 private:
  base::OnceClosure quit_closure_;

  DISALLOW_COPY_AND_ASSIGN(ExtensionInfoGeneratorUnitTest);
};

// Test some of the basic fields.
TEST_F(ExtensionInfoGeneratorUnitTest, BasicInfoTest) {
  // Enable error console for testing.
  FeatureSwitch::ScopedOverride error_console_override(
      FeatureSwitch::error_console(), true);
  profile()->GetPrefs()->SetBoolean(prefs::kExtensionsUIDeveloperMode, true);

  const char kName[] = "extension name";
  const char kVersion[] = "1.0.0.1";
  std::string id = crx_file::id_util::GenerateId("alpha");
  std::unique_ptr<base::DictionaryValue> manifest =
      DictionaryBuilder()
          .Set("name", kName)
          .Set("version", kVersion)
          .Set("manifest_version", 2)
          .Set("description", "an extension")
          .Set("permissions", ListBuilder()
                                  .Append("file://*/*")
                                  .Append("tabs")
                                  .Append("*://*.google.com/*")
                                  .Append("*://*.example.com/*")
                                  .Append("*://*.foo.bar/*")
                                  .Append("*://*.chromium.org/*")
                                  .Build())
          .Build();
  std::unique_ptr<base::DictionaryValue> manifest_copy(manifest->DeepCopy());
  scoped_refptr<const Extension> extension =
      ExtensionBuilder()
          .SetManifest(std::move(manifest))
          .SetLocation(Manifest::UNPACKED)
          .SetPath(data_dir())
          .SetID(id)
          .Build();
  service()->AddExtension(extension.get());
  ErrorConsole* error_console = ErrorConsole::Get(profile());
  const GURL kContextUrl("http://example.com");
  error_console->ReportError(std::make_unique<RuntimeError>(
      extension->id(), false, base::UTF8ToUTF16("source"),
      base::UTF8ToUTF16("message"),
      StackTrace(1, StackFrame(1, 1, base::UTF8ToUTF16("source"),
                               base::UTF8ToUTF16("function"))),
      kContextUrl, logging::LOG_ERROR, 1, 1));
  error_console->ReportError(std::make_unique<ManifestError>(
      extension->id(), base::UTF8ToUTF16("message"), base::UTF8ToUTF16("key"),
      base::string16()));
  error_console->ReportError(std::make_unique<RuntimeError>(
      extension->id(), false, base::UTF8ToUTF16("source"),
      base::UTF8ToUTF16("message"),
      StackTrace(1, StackFrame(1, 1, base::UTF8ToUTF16("source"),
                               base::UTF8ToUTF16("function"))),
      kContextUrl, logging::LOG_WARNING, 1, 1));

  // It's not feasible to validate every field here, because that would be
  // a duplication of the logic in the method itself. Instead, test a handful
  // of fields for sanity.
  std::unique_ptr<api::developer_private::ExtensionInfo> info =
      GenerateExtensionInfo(extension->id());
  ASSERT_TRUE(info.get());
  EXPECT_EQ(kName, info->name);
  EXPECT_EQ(id, info->id);
  EXPECT_EQ(kVersion, info->version);
  EXPECT_EQ(info->location, developer::LOCATION_UNPACKED);
  ASSERT_TRUE(info->path);
  EXPECT_EQ(data_dir(), base::FilePath::FromUTF8Unsafe(*info->path));
  EXPECT_EQ(api::developer_private::EXTENSION_STATE_ENABLED, info->state);
  EXPECT_EQ(api::developer_private::EXTENSION_TYPE_EXTENSION, info->type);
  EXPECT_TRUE(info->file_access.is_enabled);
  EXPECT_FALSE(info->file_access.is_active);
  EXPECT_TRUE(info->incognito_access.is_enabled);
  EXPECT_FALSE(info->incognito_access.is_active);
  PermissionMessages messages =
      extension->permissions_data()->GetPermissionMessages();
  ASSERT_EQ(messages.size(), info->permissions.size());
  size_t i = 0;
  for (const PermissionMessage& message : messages) {
    const api::developer_private::Permission& info_permission =
        info->permissions[i];
    EXPECT_EQ(message.message(), base::UTF8ToUTF16(info_permission.message));
    const std::vector<base::string16>& submessages = message.submessages();
    ASSERT_EQ(submessages.size(), info_permission.submessages.size());
    for (size_t j = 0; j < submessages.size(); ++j) {
      EXPECT_EQ(submessages[j],
                base::UTF8ToUTF16(info_permission.submessages[j]));
    }
    ++i;
  }
  ASSERT_EQ(2u, info->runtime_errors.size());
  const api::developer_private::RuntimeError& runtime_error =
      info->runtime_errors[0];
  EXPECT_EQ(extension->id(), runtime_error.extension_id);
  EXPECT_EQ(api::developer_private::ERROR_TYPE_RUNTIME, runtime_error.type);
  EXPECT_EQ(api::developer_private::ERROR_LEVEL_ERROR,
            runtime_error.severity);
  EXPECT_EQ(kContextUrl, GURL(runtime_error.context_url));
  EXPECT_EQ(1u, runtime_error.stack_trace.size());
  ASSERT_EQ(1u, info->manifest_errors.size());
  const api::developer_private::RuntimeError& runtime_error_verbose =
      info->runtime_errors[1];
  EXPECT_EQ(api::developer_private::ERROR_LEVEL_WARN,
            runtime_error_verbose.severity);
  const api::developer_private::ManifestError& manifest_error =
      info->manifest_errors[0];
  EXPECT_EQ(extension->id(), manifest_error.extension_id);

  // Test an extension that isn't unpacked.
  manifest_copy->SetString("update_url",
                           "https://clients2.google.com/service/update2/crx");
  id = crx_file::id_util::GenerateId("beta");
  extension = ExtensionBuilder()
                  .SetManifest(std::move(manifest_copy))
                  .SetLocation(Manifest::EXTERNAL_PREF)
                  .SetID(id)
                  .Build();
  service()->AddExtension(extension.get());
  info = GenerateExtensionInfo(extension->id());
  EXPECT_EQ(developer::LOCATION_THIRD_PARTY, info->location);
  EXPECT_FALSE(info->path);
}

// Test three generated json outputs.
TEST_F(ExtensionInfoGeneratorUnitTest, GenerateExtensionsJSONData) {
  // Test Extension1
  base::FilePath extension_path =
      data_dir().AppendASCII("good")
                .AppendASCII("Extensions")
                .AppendASCII("behllobkkfkfnphdnhnkndlbkcpglgmj")
                .AppendASCII("1.0.0.0");

  base::FilePath expected_outputs_path =
      data_dir().AppendASCII("api_test")
                .AppendASCII("developer")
                .AppendASCII("generated_output");

  {
    InspectableViewsFinder::ViewList views;
    views.push_back(InspectableViewsFinder::ConstructView(
        GURL("chrome-extension://behllobkkfkfnphdnhnkndlbkcpglgmj/bar.html"),
        42, 88, true, false, VIEW_TYPE_TAB_CONTENTS));
    views.push_back(InspectableViewsFinder::ConstructView(
        GURL("chrome-extension://behllobkkfkfnphdnhnkndlbkcpglgmj/dog.html"), 0,
        0, false, true, VIEW_TYPE_TAB_CONTENTS));

    CompareExpectedAndActualOutput(
        extension_path, std::move(views),
        expected_outputs_path.AppendASCII(
            "behllobkkfkfnphdnhnkndlbkcpglgmj.json"));
  }

#if !defined(OS_CHROMEOS)
  // Test Extension2
  extension_path = data_dir().AppendASCII("good")
                             .AppendASCII("Extensions")
                             .AppendASCII("hpiknbiabeeppbpihjehijgoemciehgk")
                             .AppendASCII("2");

  {
    // It's OK to have duplicate URLs, so long as the IDs are different.
    InspectableViewsFinder::ViewList views;
    views.push_back(InspectableViewsFinder::ConstructView(
        GURL("chrome-extension://hpiknbiabeeppbpihjehijgoemciehgk/bar.html"),
        42, 88, true, false, VIEW_TYPE_TAB_CONTENTS));
    views.push_back(InspectableViewsFinder::ConstructView(
        GURL("chrome-extension://hpiknbiabeeppbpihjehijgoemciehgk/bar.html"), 0,
        0, false, true, VIEW_TYPE_TAB_CONTENTS));

    CompareExpectedAndActualOutput(
        extension_path, std::move(views),
        expected_outputs_path.AppendASCII(
            "hpiknbiabeeppbpihjehijgoemciehgk.json"));
  }
#endif

  // Test Extension3
  extension_path = data_dir().AppendASCII("good")
                             .AppendASCII("Extensions")
                             .AppendASCII("bjafgdebaacbbbecmhlhpofkepfkgcpa")
                             .AppendASCII("1.0");
  CompareExpectedAndActualOutput(extension_path,
                                 InspectableViewsFinder::ViewList(),
                                 expected_outputs_path.AppendASCII(
                                     "bjafgdebaacbbbecmhlhpofkepfkgcpa.json"));
}

// Test that the all_urls checkbox only shows up for extensions that want all
// urls, and only when the switch is on.
TEST_F(ExtensionInfoGeneratorUnitTest, ExtensionInfoRunOnAllUrls) {
  // Start with the switch enabled.
  auto scoped_feature_list = std::make_unique<base::test::ScopedFeatureList>();
  scoped_feature_list->InitAndEnableFeature(features::kRuntimeHostPermissions);

  // Two extensions - one with all urls, one without.
  scoped_refptr<const Extension> all_urls_extension = CreateExtension(
      "all_urls", ListBuilder().Append(kAllHostsPermission).Build(),
      Manifest::INTERNAL);
  scoped_refptr<const Extension> no_urls_extension =
      CreateExtension("no urls", ListBuilder().Build(), Manifest::INTERNAL);

  std::unique_ptr<developer::ExtensionInfo> info =
      GenerateExtensionInfo(all_urls_extension->id());

  // The extension should want all urls, and have it currently granted.
  EXPECT_TRUE(info->run_on_all_urls.is_enabled);
  EXPECT_TRUE(info->run_on_all_urls.is_active);

  // Revoke the all urls permission.
  ScriptingPermissionsModifier permissions_modifier(profile(),
                                                    all_urls_extension);
  permissions_modifier.SetAllowedOnAllUrls(false);

  // Now the extension want all urls, but not have it granted.
  info = GenerateExtensionInfo(all_urls_extension->id());
  EXPECT_TRUE(info->run_on_all_urls.is_enabled);
  EXPECT_FALSE(info->run_on_all_urls.is_active);

  // The other extension should neither want nor have all urls.
  info = GenerateExtensionInfo(no_urls_extension->id());
  EXPECT_FALSE(info->run_on_all_urls.is_enabled);
  EXPECT_FALSE(info->run_on_all_urls.is_active);

  // Turn off the switch. With the switch off, the run_on_all_urls control
  // should never be enabled.
  scoped_feature_list.reset();
  info = GenerateExtensionInfo(all_urls_extension->id());
  EXPECT_FALSE(info->run_on_all_urls.is_enabled);
  EXPECT_FALSE(info->run_on_all_urls.is_active);
}

// Test that file:// access checkbox does not show up when the user can't
// modify an extension's settings. https://crbug.com/173640.
TEST_F(ExtensionInfoGeneratorUnitTest, ExtensionInfoLockedAllUrls) {
  // Force installed extensions aren't user modifyable.
  scoped_refptr<const Extension> locked_extension =
      CreateExtension("locked", ListBuilder().Append("file://*/*").Build(),
                      Manifest::EXTERNAL_POLICY_DOWNLOAD);

  std::unique_ptr<developer::ExtensionInfo> info =
      GenerateExtensionInfo(locked_extension->id());

  // Extension wants file:// access but the checkbox will not appear
  // in chrome://extensions.
  EXPECT_TRUE(locked_extension->wants_file_access());
  EXPECT_FALSE(info->file_access.is_enabled);
  EXPECT_FALSE(info->file_access.is_active);
}

// Tests that blacklisted extensions are returned by the ExtensionInfoGenerator.
TEST_F(ExtensionInfoGeneratorUnitTest, Blacklisted) {
  const scoped_refptr<const Extension> extension1 = CreateExtension(
      "test1", std::make_unique<base::ListValue>(), Manifest::INTERNAL);
  const scoped_refptr<const Extension> extension2 = CreateExtension(
      "test2", std::make_unique<base::ListValue>(), Manifest::INTERNAL);

  std::string id1 = extension1->id();
  std::string id2 = extension2->id();
  ASSERT_NE(id1, id2);

  ExtensionInfoGenerator::ExtensionInfoList info_list =
      GenerateExtensionsInfo();
  const developer::ExtensionInfo* info1 = GetInfoFromList(info_list, id1);
  const developer::ExtensionInfo* info2 = GetInfoFromList(info_list, id2);
  ASSERT_NE(nullptr, info1);
  ASSERT_NE(nullptr, info2);
  EXPECT_EQ(developer::EXTENSION_STATE_ENABLED, info1->state);
  EXPECT_EQ(developer::EXTENSION_STATE_ENABLED, info2->state);

  service()->BlacklistExtensionForTest(id1);

  info_list = GenerateExtensionsInfo();
  info1 = GetInfoFromList(info_list, id1);
  info2 = GetInfoFromList(info_list, id2);
  ASSERT_NE(nullptr, info1);
  ASSERT_NE(nullptr, info2);
  EXPECT_EQ(developer::EXTENSION_STATE_BLACKLISTED, info1->state);
  EXPECT_EQ(developer::EXTENSION_STATE_ENABLED, info2->state);
}

}  // namespace extensions
