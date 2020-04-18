// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/app_list/app_list_syncable_service.h"
#include "base/bind.h"
#include "base/files/scoped_temp_dir.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/ui/app_list/app_list_model_updater.h"
#include "chrome/browser/ui/app_list/app_list_test_util.h"
#include "chrome/browser/ui/app_list/chrome_app_list_item.h"
#include "chrome/browser/ui/app_list/test/fake_app_list_model_updater.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "components/crx_file/id_util.h"
#include "components/sync/model/fake_sync_change_processor.h"
#include "components/sync/model/sync_error_factory.h"
#include "components/sync/model/sync_error_factory_mock.h"
#include "components/sync/protocol/sync.pb.h"
#include "extensions/browser/extension_system.h"
#include "extensions/common/constants.h"

using crx_file::id_util::GenerateId;

namespace {

scoped_refptr<extensions::Extension> MakeApp(
    const std::string& name,
    const std::string& id,
    extensions::Extension::InitFromValueFlags flags) {
  std::string err;
  base::DictionaryValue value;
  value.SetString("name", name);
  value.SetString("version", "0.0");
  value.SetString("app.launch.web_url", "http://google.com");
  scoped_refptr<extensions::Extension> app = extensions::Extension::Create(
      base::FilePath(), extensions::Manifest::INTERNAL, value, flags, id, &err);
  EXPECT_EQ(err, "");
  return app;
}

// Creates next by natural sort ordering application id. Application id has to
// have 32 chars each in range 'a' to 'p' inclusively.
std::string CreateNextAppId(const std::string& app_id) {
  DCHECK(crx_file::id_util::IdIsValid(app_id));
  std::string next_app_id = app_id;
  size_t index = next_app_id.length() - 1;
  while (index > 0 && next_app_id[index] == 'p')
    next_app_id[index--] = 'a';
  DCHECK_NE(next_app_id[index], 'p');
  next_app_id[index]++;
  DCHECK(crx_file::id_util::IdIsValid(next_app_id));
  return next_app_id;
}

constexpr char kUnset[] = "__unset__";
constexpr char kDefault[] = "__default__";

// These constants are defined as functions so their values can be derived via
// function calls.  The constant naming scheme is kept to maintain readability.
const std::string kInvalidOrdinalsId() {
  return GenerateId("invalid_ordinals");
}
const std::string kEmptyItemNameId() {
  return GenerateId("empty_item_name");
}
const std::string kEmptyItemNameUnsetId() {
  return GenerateId("empty_item_name_unset");
}
const std::string kEmptyParentId() {
  return GenerateId("empty_parent_id");
}
const std::string kEmptyParentUnsetId() {
  return GenerateId("empty_parent_id_unset");
}
const std::string kEmptyOrdinalsId() {
  return GenerateId("empty_ordinals");
}
const std::string kEmptyOrdinalsUnsetId() {
  return GenerateId("empty_ordinals_unset");
}
const std::string kDupeItemId() {
  return GenerateId("dupe_item_id");
}
const std::string kParentId() {
  return GenerateId("parent_id");
}

syncer::SyncData CreateAppRemoteData(const std::string& id,
                                     const std::string& name,
                                     const std::string& parent_id,
                                     const std::string& item_ordinal,
                                     const std::string& item_pin_ordinal) {
  sync_pb::EntitySpecifics specifics;
  sync_pb::AppListSpecifics* app_list = specifics.mutable_app_list();
  if (id != kUnset)
    app_list->set_item_id(id);
  app_list->set_item_type(sync_pb::AppListSpecifics_AppListItemType_TYPE_APP);
  if (name != kUnset)
    app_list->set_item_name(name);
  if (parent_id != kUnset)
    app_list->set_parent_id(parent_id);
  if (item_ordinal != kUnset)
    app_list->set_item_ordinal(item_ordinal);
  if (item_pin_ordinal != kUnset)
    app_list->set_item_pin_ordinal(item_pin_ordinal);

  return syncer::SyncData::CreateRemoteData(std::hash<std::string>{}(id),
                                            specifics, base::Time());
}

syncer::SyncDataList CreateBadAppRemoteData(const std::string& id) {
  syncer::SyncDataList sync_list;
  // Invalid item_ordinal and item_pin_ordinal.
  sync_list.push_back(CreateAppRemoteData(
      id == kDefault ? kInvalidOrdinalsId() : id, "item_name", kParentId(),
      "$$invalid_ordinal$$", "$$invalid_ordinal$$"));
  // Empty item name.
  sync_list.push_back(
      CreateAppRemoteData(id == kDefault ? kEmptyItemNameId() : id, "",
                          kParentId(), "ordinal", "pinordinal"));
  sync_list.push_back(
      CreateAppRemoteData(id == kDefault ? kEmptyItemNameUnsetId() : id, kUnset,
                          kParentId(), "ordinal", "pinordinal"));
  // Empty parent ID.
  sync_list.push_back(
      CreateAppRemoteData(id == kDefault ? kEmptyParentId() : id, "item_name",
                          "", "ordinal", "pinordinal"));
  sync_list.push_back(
      CreateAppRemoteData(id == kDefault ? kEmptyParentUnsetId() : id,
                          "item_name", kUnset, "ordinal", "pinordinal"));
  // Empty item_ordinal and item_pin_ordinal.
  sync_list.push_back(
      CreateAppRemoteData(id == kDefault ? kEmptyOrdinalsId() : id, "item_name",
                          kParentId(), "", ""));
  sync_list.push_back(
      CreateAppRemoteData(id == kDefault ? kEmptyOrdinalsUnsetId() : id,
                          "item_name", kParentId(), kUnset, kUnset));
  // Duplicate item_id.
  sync_list.push_back(CreateAppRemoteData(id == kDefault ? kDupeItemId() : id,
                                          "item_name", kParentId(), "ordinal",
                                          "pinordinal"));
  sync_list.push_back(CreateAppRemoteData(id == kDefault ? kDupeItemId() : id,
                                          "item_name_dupe", kParentId(),
                                          "ordinal", "pinordinal"));
  // Empty item_id.
  sync_list.push_back(CreateAppRemoteData("", "item_name", kParentId(),
                                          "ordinal", "pinordinal"));
  sync_list.push_back(CreateAppRemoteData(kUnset, "item_name", kParentId(),
                                          "ordinal", "pinordinal"));
  // All fields empty.
  sync_list.push_back(CreateAppRemoteData("", "", "", "", ""));
  sync_list.push_back(
      CreateAppRemoteData(kUnset, kUnset, kUnset, kUnset, kUnset));

  return sync_list;
}

}  // namespace

class AppListSyncableServiceTest : public AppListTestBase {
 public:
  AppListSyncableServiceTest() = default;
  ~AppListSyncableServiceTest() override = default;

  void SetUp() override {
    AppListTestBase::SetUp();

    // Make sure we have a Profile Manager.
    DCHECK(temp_dir_.CreateUniqueTempDir());
    TestingBrowserProcess::GetGlobal()->SetProfileManager(
        new ProfileManagerWithoutInit(temp_dir_.GetPath()));

    extensions::ExtensionSystem* extension_system =
        extensions::ExtensionSystem::Get(profile_.get());
    DCHECK(extension_system);

    model_updater_factory_scope_ = std::make_unique<
        app_list::AppListSyncableService::ScopedModelUpdaterFactoryForTest>(
        base::Bind([]() -> std::unique_ptr<AppListModelUpdater> {
          return std::make_unique<FakeAppListModelUpdater>();
        }));

    app_list_syncable_service_ =
        std::make_unique<app_list::AppListSyncableService>(profile_.get(),
                                                           extension_system);
    model_updater_test_api_ =
        std::make_unique<AppListModelUpdater::TestApi>(model_updater());
  }

  void TearDown() override { app_list_syncable_service_.reset(); }

  AppListModelUpdater* model_updater() {
    return app_list_syncable_service_->GetModelUpdater();
  }

  AppListModelUpdater::TestApi* model_updater_test_api() {
    return model_updater_test_api_.get();
  }

  const app_list::AppListSyncableService::SyncItem* GetSyncItem(
      const std::string& id) const {
    return app_list_syncable_service_->GetSyncItem(id);
  }

 protected:
  app_list::AppListSyncableService* app_list_syncable_service() {
    return app_list_syncable_service_.get();
  }

 private:
  base::ScopedTempDir temp_dir_;
  std::unique_ptr<AppListModelUpdater::TestApi> model_updater_test_api_;
  std::unique_ptr<app_list::AppListSyncableService> app_list_syncable_service_;
  std::unique_ptr<
      app_list::AppListSyncableService::ScopedModelUpdaterFactoryForTest>
      model_updater_factory_scope_;

  DISALLOW_COPY_AND_ASSIGN(AppListSyncableServiceTest);
};

TEST_F(AppListSyncableServiceTest, OEMFolderForConflictingPos) {
  // Create a "web store" app.
  const std::string web_store_app_id(extensions::kWebStoreAppId);
  scoped_refptr<extensions::Extension> store =
      MakeApp("webstore", web_store_app_id,
              extensions::Extension::WAS_INSTALLED_BY_DEFAULT);
  service_->AddExtension(store.get());

  // Create some app. Note its id should be greater than web store app id in
  // order to move app in case of conflicting pos after web store app.
  const std::string some_app_id = CreateNextAppId(extensions::kWebStoreAppId);
  scoped_refptr<extensions::Extension> some_app =
      MakeApp("some_app", some_app_id,
              extensions ::Extension::WAS_INSTALLED_BY_DEFAULT);
  service_->AddExtension(some_app.get());

  ChromeAppListItem* web_store_item =
      model_updater()->FindItem(web_store_app_id);
  ASSERT_TRUE(web_store_item);
  ChromeAppListItem* some_app_item = model_updater()->FindItem(some_app_id);
  ASSERT_TRUE(some_app_item);

  // Simulate position conflict.
  model_updater_test_api()->SetItemPosition(web_store_item->id(),
                                            some_app_item->position());

  // Install an OEM app. It must be placed by default after web store app but in
  // case of app of the same position should be shifted next.
  const std::string oem_app_id = CreateNextAppId(some_app_id);
  scoped_refptr<extensions::Extension> oem_app = MakeApp(
      "oem_app", oem_app_id, extensions::Extension::WAS_INSTALLED_BY_OEM);
  service_->AddExtension(oem_app.get());

  size_t web_store_app_index;
  size_t some_app_index;
  EXPECT_TRUE(model_updater()->FindItemIndexForTest(web_store_app_id,
                                                    &web_store_app_index));
  EXPECT_TRUE(
      model_updater()->FindItemIndexForTest(some_app_id, &some_app_index));
  // OEM item is not top level element.
  ChromeAppListItem* oem_app_item = model_updater()->FindItem(oem_app_id);
  EXPECT_NE(nullptr, oem_app_item);
  EXPECT_EQ(oem_app_item->folder_id(), ash::kOemFolderId);
  // But OEM folder is.
  ChromeAppListItem* oem_folder = model_updater()->FindItem(ash::kOemFolderId);
  EXPECT_NE(nullptr, oem_folder);
  EXPECT_EQ(oem_folder->folder_id(), "");
}

TEST_F(AppListSyncableServiceTest, InitialMerge) {
  const std::string kItemId1 = GenerateId("item_id1");
  const std::string kItemId2 = GenerateId("item_id2");

  syncer::SyncDataList sync_list;
  sync_list.push_back(CreateAppRemoteData(kItemId1, "item_name1",
                                          GenerateId("parent_id1"), "ordinal",
                                          "pinordinal"));
  sync_list.push_back(CreateAppRemoteData(kItemId2, "item_name2",
                                          GenerateId("parent_id2"), "ordinal",
                                          "pinordinal"));

  app_list_syncable_service()->MergeDataAndStartSyncing(
      syncer::APP_LIST, sync_list,
      std::make_unique<syncer::FakeSyncChangeProcessor>(),
      std::make_unique<syncer::SyncErrorFactoryMock>());
  content::RunAllTasksUntilIdle();

  ASSERT_TRUE(GetSyncItem(kItemId1));
  EXPECT_EQ("item_name1", GetSyncItem(kItemId1)->item_name);
  EXPECT_EQ(GenerateId("parent_id1"), GetSyncItem(kItemId1)->parent_id);
  EXPECT_EQ("ordinal", GetSyncItem(kItemId1)->item_ordinal.ToDebugString());
  EXPECT_EQ("pinordinal",
            GetSyncItem(kItemId1)->item_pin_ordinal.ToDebugString());

  ASSERT_TRUE(GetSyncItem(kItemId2));
  EXPECT_EQ("item_name2", GetSyncItem(kItemId2)->item_name);
  EXPECT_EQ(GenerateId("parent_id2"), GetSyncItem(kItemId2)->parent_id);
  EXPECT_EQ("ordinal", GetSyncItem(kItemId2)->item_ordinal.ToDebugString());
  EXPECT_EQ("pinordinal",
            GetSyncItem(kItemId2)->item_pin_ordinal.ToDebugString());
}

TEST_F(AppListSyncableServiceTest, InitialMerge_BadData) {
  const syncer::SyncDataList sync_list = CreateBadAppRemoteData(kDefault);

  app_list_syncable_service()->MergeDataAndStartSyncing(
      syncer::APP_LIST, sync_list,
      std::make_unique<syncer::FakeSyncChangeProcessor>(),
      std::make_unique<syncer::SyncErrorFactoryMock>());
  content::RunAllTasksUntilIdle();

  // Invalid item_ordinal and item_pin_ordinal.
  // Invalid item_ordinal is fixed up.
  ASSERT_TRUE(GetSyncItem(kInvalidOrdinalsId()));
  EXPECT_EQ("n",
            GetSyncItem(kInvalidOrdinalsId())->item_ordinal.ToDebugString());
  EXPECT_EQ(
      "INVALID[$$invalid_ordinal$$]",
      GetSyncItem(kInvalidOrdinalsId())->item_pin_ordinal.ToDebugString());

  // Empty item name.
  ASSERT_TRUE(GetSyncItem(kEmptyItemNameId()));
  EXPECT_EQ("", GetSyncItem(kEmptyItemNameId())->item_name);
  EXPECT_TRUE(GetSyncItem(kEmptyItemNameUnsetId()));
  EXPECT_EQ("", GetSyncItem(kEmptyItemNameUnsetId())->item_name);

  // Empty parent ID.
  ASSERT_TRUE(GetSyncItem(kEmptyParentId()));
  EXPECT_EQ("", GetSyncItem(kEmptyParentId())->parent_id);
  EXPECT_TRUE(GetSyncItem(kEmptyParentUnsetId()));
  EXPECT_EQ("", GetSyncItem(kEmptyParentUnsetId())->parent_id);

  // Empty item_ordinal and item_pin_ordinal.
  // Empty item_ordinal is fixed up.
  ASSERT_TRUE(GetSyncItem(kEmptyOrdinalsId()));
  EXPECT_EQ("n", GetSyncItem(kEmptyOrdinalsId())->item_ordinal.ToDebugString());
  EXPECT_EQ("INVALID[]",
            GetSyncItem(kEmptyOrdinalsId())->item_pin_ordinal.ToDebugString());
  ASSERT_TRUE(GetSyncItem(kEmptyOrdinalsUnsetId()));
  EXPECT_EQ("n",
            GetSyncItem(kEmptyOrdinalsUnsetId())->item_ordinal.ToDebugString());
  EXPECT_EQ(
      "INVALID[]",
      GetSyncItem(kEmptyOrdinalsUnsetId())->item_pin_ordinal.ToDebugString());

  // Duplicate item_id overrides previous.
  ASSERT_TRUE(GetSyncItem(kDupeItemId()));
  EXPECT_EQ("item_name_dupe", GetSyncItem(kDupeItemId())->item_name);
}

TEST_F(AppListSyncableServiceTest, InitialMergeAndUpdate) {
  const std::string kItemId1 = GenerateId("item_id1");
  const std::string kItemId2 = GenerateId("item_id2");

  syncer::SyncDataList sync_list;
  sync_list.push_back(CreateAppRemoteData(kItemId1, "item_name1", kParentId(),
                                          "ordinal", "pinordinal"));
  sync_list.push_back(CreateAppRemoteData(kItemId2, "item_name2", kParentId(),
                                          "ordinal", "pinordinal"));

  app_list_syncable_service()->MergeDataAndStartSyncing(
      syncer::APP_LIST, sync_list,
      std::make_unique<syncer::FakeSyncChangeProcessor>(),
      std::make_unique<syncer::SyncErrorFactoryMock>());
  content::RunAllTasksUntilIdle();

  ASSERT_TRUE(GetSyncItem(kItemId1));
  ASSERT_TRUE(GetSyncItem(kItemId2));

  syncer::SyncChangeList change_list;
  change_list.push_back(syncer::SyncChange(
      FROM_HERE, syncer::SyncChange::ACTION_UPDATE,
      CreateAppRemoteData(kItemId1, "item_name1x", GenerateId("parent_id1x"),
                          "ordinalx", "pinordinalx")));
  change_list.push_back(syncer::SyncChange(
      FROM_HERE, syncer::SyncChange::ACTION_UPDATE,
      CreateAppRemoteData(kItemId2, "item_name2x", GenerateId("parent_id2x"),
                          "ordinalx", "pinordinalx")));

  app_list_syncable_service()->ProcessSyncChanges(base::Location(),
                                                  change_list);
  content::RunAllTasksUntilIdle();

  ASSERT_TRUE(GetSyncItem(kItemId1));
  EXPECT_EQ("item_name1x", GetSyncItem(kItemId1)->item_name);
  EXPECT_EQ(GenerateId("parent_id1x"), GetSyncItem(kItemId1)->parent_id);
  EXPECT_EQ("ordinalx", GetSyncItem(kItemId1)->item_ordinal.ToDebugString());
  EXPECT_EQ("pinordinalx",
            GetSyncItem(kItemId1)->item_pin_ordinal.ToDebugString());

  ASSERT_TRUE(GetSyncItem(kItemId2));
  EXPECT_EQ("item_name2x", GetSyncItem(kItemId2)->item_name);
  EXPECT_EQ(GenerateId("parent_id2x"), GetSyncItem(kItemId2)->parent_id);
  EXPECT_EQ("ordinalx", GetSyncItem(kItemId2)->item_ordinal.ToDebugString());
  EXPECT_EQ("pinordinalx",
            GetSyncItem(kItemId2)->item_pin_ordinal.ToDebugString());
}

TEST_F(AppListSyncableServiceTest, InitialMergeAndUpdate_BadData) {
  const std::string kItemId = GenerateId("item_id");

  syncer::SyncDataList sync_list;
  sync_list.push_back(CreateAppRemoteData(kItemId, "item_name", kParentId(),
                                          "ordinal", "pinordinal"));

  app_list_syncable_service()->MergeDataAndStartSyncing(
      syncer::APP_LIST, sync_list,
      std::make_unique<syncer::FakeSyncChangeProcessor>(),
      std::make_unique<syncer::SyncErrorFactoryMock>());
  content::RunAllTasksUntilIdle();

  ASSERT_TRUE(GetSyncItem(kItemId));

  syncer::SyncChangeList change_list;
  const syncer::SyncDataList update_list = CreateBadAppRemoteData(kItemId);
  for (syncer::SyncDataList::const_iterator iter = update_list.begin();
       iter != update_list.end(); ++iter) {
    change_list.push_back(syncer::SyncChange(
        FROM_HERE, syncer::SyncChange::ACTION_UPDATE, *iter));
  }

  // Validate items with bad data are processed without crashing.
  app_list_syncable_service()->ProcessSyncChanges(base::Location(),
                                                  change_list);
  content::RunAllTasksUntilIdle();

  ASSERT_TRUE(GetSyncItem(kItemId));
}

TEST_F(AppListSyncableServiceTest, InitialMerge_NoDriveAppData) {
  // Note that Drive app item id must start with "drive-app-" prefix as defined
  // in kDriveAppSyncIdPrefix in AppListSyncableService.
  constexpr char kDriveAppItemId[] = "drive-app-fake-drive-app";

  syncer::SyncDataList sync_list;
  sync_list.push_back(CreateAppRemoteData(
      kDriveAppItemId, "Fake Drive App", kParentId(), "ordinal", "pinordinal"));

  app_list_syncable_service()->MergeDataAndStartSyncing(
      syncer::APP_LIST, sync_list,
      std::make_unique<syncer::FakeSyncChangeProcessor>(),
      std::make_unique<syncer::SyncErrorFactoryMock>());
  content::RunAllTasksUntilIdle();

  ASSERT_FALSE(GetSyncItem(kDriveAppItemId));
}
