// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include <memory>
#include <utility>
#include <vector>

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/run_loop.h"
#include "base/test/scoped_feature_list.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/values.h"
#include "components/crx_file/id_util.h"
#include "components/update_client/crx_update_item.h"
#include "components/update_client/update_client.h"
#include "content/public/browser/notification_service.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/public/test/test_utils.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extensions_test.h"
#include "extensions/browser/mock_extension_system.h"
#include "extensions/browser/notification_types.h"
#include "extensions/browser/test_extensions_browser_client.h"
#include "extensions/browser/uninstall_ping_sender.h"
#include "extensions/browser/updater/extension_downloader.h"
#include "extensions/browser/updater/extension_update_data.h"
#include "extensions/browser/updater/update_service.h"
#include "extensions/common/extension_builder.h"
#include "extensions/common/extension_features.h"
#include "extensions/common/extension_urls.h"
#include "extensions/common/extensions_client.h"
#include "extensions/common/manifest_url_handlers.h"
#include "extensions/common/value_builder.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

using UpdateClientEvents = update_client::UpdateClient::Observer::Events;

class FakeUpdateClient : public update_client::UpdateClient {
 public:
  FakeUpdateClient();

  // Returns the data we've gotten from the CrxDataCallback for ids passed to
  // the Update function.
  std::vector<std::unique_ptr<update_client::CrxComponent>>* data() {
    return &data_;
  }

  // Used for tests that uninstall pings get requested properly.
  struct UninstallPing {
    std::string id;
    base::Version version;
    int reason;
    UninstallPing(const std::string& id,
                  const base::Version& version,
                  int reason)
        : id(id), version(version), reason(reason) {}
  };
  std::vector<UninstallPing>& uninstall_pings() { return uninstall_pings_; }

  struct UpdateRequest {
    std::vector<std::string> extension_ids;
    update_client::Callback callback;
  };

  // update_client::UpdateClient
  void AddObserver(Observer* observer) override {
    if (observer)
      observers_.push_back(observer);
  }
  void RemoveObserver(Observer* observer) override {}
  void Install(const std::string& id,
               CrxDataCallback crx_data_callback,
               update_client::Callback callback) override {}
  void Update(const std::vector<std::string>& ids,
              CrxDataCallback crx_data_callback,
              bool is_foreground,
              update_client::Callback callback) override;
  bool GetCrxUpdateState(
      const std::string& id,
      update_client::CrxUpdateItem* update_item) const override {
    update_item->next_version = base::Version("2.0");
    return true;
  }
  bool IsUpdating(const std::string& id) const override { return false; }
  void Stop() override {}
  void SendUninstallPing(const std::string& id,
                         const base::Version& version,
                         int reason,
                         update_client::Callback callback) override {
    uninstall_pings_.emplace_back(id, version, reason);
  }

  void FireEvent(Observer::Events event, const std::string& extension_id) {
    for (Observer* observer : observers_)
      observer->OnEvent(event, extension_id);
  }

  void set_delay_update() { delay_update_ = true; }

  bool delay_update() const { return delay_update_; }

  UpdateRequest& update_request(int index) { return delayed_requests_[index]; }
  int num_update_requests() const {
    return static_cast<int>(delayed_requests_.size());
  }

  void RunDelayedUpdate(int index) {
    UpdateRequest& request = update_request(index);
    for (const std::string& id : request.extension_ids)
      FireEvent(Observer::Events::COMPONENT_UPDATED, id);
    std::move(request.callback).Run(update_client::Error::NONE);
  }

 protected:
  friend class base::RefCounted<FakeUpdateClient>;
  ~FakeUpdateClient() override {}

  std::vector<std::unique_ptr<update_client::CrxComponent>> data_;
  std::vector<UninstallPing> uninstall_pings_;
  std::vector<Observer*> observers_;

  bool delay_update_;
  std::vector<UpdateRequest> delayed_requests_;

 private:
  DISALLOW_COPY_AND_ASSIGN(FakeUpdateClient);
};

FakeUpdateClient::FakeUpdateClient() : delay_update_(false) {}

void FakeUpdateClient::Update(const std::vector<std::string>& ids,
                              CrxDataCallback crx_data_callback,
                              bool is_foreground,
                              update_client::Callback callback) {
  data_ = std::move(crx_data_callback).Run(ids);

  if (delay_update()) {
    delayed_requests_.push_back({ids, std::move(callback)});
  } else {
    for (const std::string& id : ids)
      FireEvent(Observer::Events::COMPONENT_UPDATED, id);
    std::move(callback).Run(update_client::Error::NONE);
  }
}

class UpdateFoundNotificationObserver : public content::NotificationObserver {
 public:
  UpdateFoundNotificationObserver(const std::string& id,
                                  const std::string& version)
      : id_(id), version_(version) {
    registrar_.Add(this, extensions::NOTIFICATION_EXTENSION_UPDATE_FOUND,
                   content::NotificationService::AllSources());
  }
  ~UpdateFoundNotificationObserver() override {
    registrar_.Remove(this, extensions::NOTIFICATION_EXTENSION_UPDATE_FOUND,
                      content::NotificationService::AllSources());
  }

  void reset() { found_notification_ = false; }

  bool found_notification() const { return found_notification_; }

 private:
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override {
    ASSERT_EQ(extensions::NOTIFICATION_EXTENSION_UPDATE_FOUND, type);
    EXPECT_EQ(id_, content::Details<extensions::UpdateDetails>(details)->id);
    EXPECT_EQ(version_, content::Details<extensions::UpdateDetails>(details)
                            ->version.GetString());
    found_notification_ = true;
  }

 private:
  content::NotificationRegistrar registrar_;
  bool found_notification_ = false;
  std::string id_;
  std::string version_;
};

}  // namespace

namespace extensions {

namespace {

// A global variable for controlling whether uninstalls should cause uninstall
// pings to be sent.
UninstallPingSender::FilterResult g_should_ping =
    UninstallPingSender::DO_NOT_SEND_PING;

// Helper method to serve as an uninstall ping filter.
UninstallPingSender::FilterResult ShouldPing(const Extension* extension,
                                             UninstallReason reason) {
  return g_should_ping;
}

// A fake ExtensionSystem that lets us intercept calls to install new
// versions of an extension.
class FakeExtensionSystem : public MockExtensionSystem {
 public:
  using InstallUpdateCallback = MockExtensionSystem::InstallUpdateCallback;
  explicit FakeExtensionSystem(content::BrowserContext* context)
      : MockExtensionSystem(context) {}
  ~FakeExtensionSystem() override {}

  struct InstallUpdateRequest {
    std::string extension_id;
    base::FilePath temp_dir;
  };

  std::vector<InstallUpdateRequest>* install_requests() {
    return &install_requests_;
  }

  void set_install_callback(base::OnceClosure callback) {
    next_install_callback_ = std::move(callback);
  }

  // ExtensionSystem override
  void InstallUpdate(const std::string& extension_id,
                     const std::string& public_key,
                     const base::FilePath& temp_dir,
                     InstallUpdateCallback install_update_callback) override {
    base::DeleteFile(temp_dir, true /*recursive*/);
    InstallUpdateRequest request;
    request.extension_id = extension_id;
    request.temp_dir = temp_dir;
    install_requests_.push_back(request);
    if (!next_install_callback_.is_null()) {
      std::move(next_install_callback_).Run();
    }
    std::move(install_update_callback).Run(base::nullopt);
  }

 private:
  std::vector<InstallUpdateRequest> install_requests_;
  base::OnceClosure next_install_callback_;
};

class UpdateServiceTest : public ExtensionsTest {
 public:
  UpdateServiceTest() {}
  ~UpdateServiceTest() override {}

  void SetUp() override {
    ExtensionsTest::SetUp();
    extensions_browser_client()->set_extension_system_factory(
        &fake_extension_system_factory_);
    extensions_browser_client()->SetUpdateClientFactory(base::Bind(
        &UpdateServiceTest::CreateUpdateClient, base::Unretained(this)));

    update_service_ = UpdateService::Get(browser_context());
  }

 protected:
  UpdateService* update_service() const { return update_service_; }
  FakeUpdateClient* update_client() const { return update_client_.get(); }

  update_client::UpdateClient* CreateUpdateClient() {
    // We only expect that this will get called once, so consider it an error
    // if our update_client_ is already non-null.
    EXPECT_EQ(nullptr, update_client_.get());
    update_client_ = base::MakeRefCounted<FakeUpdateClient>();
    return update_client_.get();
  }

  // Helper function that creates a file at |relative_path| within |directory|
  // and fills it with |content|.
  bool AddFileToDirectory(const base::FilePath& directory,
                          const base::FilePath& relative_path,
                          const std::string& content) {
    base::FilePath full_path = directory.Append(relative_path);
    if (!CreateDirectory(full_path.DirName()))
      return false;
    int result = base::WriteFile(full_path, content.data(), content.size());
    return (static_cast<size_t>(result) == content.size());
  }

  FakeExtensionSystem* extension_system() {
    return static_cast<FakeExtensionSystem*>(
        fake_extension_system_factory_.GetForBrowserContext(browser_context()));
  }

 private:
  UpdateService* update_service_ = nullptr;
  scoped_refptr<FakeUpdateClient> update_client_;
  MockExtensionSystemFactory<FakeExtensionSystem>
      fake_extension_system_factory_;
};

TEST_F(UpdateServiceTest, BasicUpdateOperations) {
  // Create a temporary directory that a fake extension will live in and fill
  // it with some test files.
  base::ScopedTempDir temp_dir;
  ASSERT_TRUE(temp_dir.CreateUniqueTempDir());
  base::FilePath foo_js(FILE_PATH_LITERAL("foo.js"));
  base::FilePath bar_html(FILE_PATH_LITERAL("bar/bar.html"));
  ASSERT_TRUE(AddFileToDirectory(temp_dir.GetPath(), foo_js, "hello"))
      << "Failed to write " << temp_dir.GetPath().value() << "/"
      << foo_js.value();
  ASSERT_TRUE(AddFileToDirectory(temp_dir.GetPath(), bar_html, "world"));

  ExtensionBuilder builder;
  builder.SetManifest(DictionaryBuilder()
                          .Set("name", "Foo")
                          .Set("version", "1.0")
                          .Set("manifest_version", 2)
                          .Build());
  builder.SetID(crx_file::id_util::GenerateId("whatever"));
  builder.SetPath(temp_dir.GetPath());

  scoped_refptr<Extension> extension1(builder.Build());

  ExtensionRegistry::Get(browser_context())->AddEnabled(extension1);

  ExtensionUpdateCheckParams update_check_params;
  update_check_params.update_info[extension1->id()] = ExtensionUpdateData();

  // Start an update check and verify that the UpdateClient was sent the right
  // data.
  bool executed = false;
  update_service()->StartUpdateCheck(
      update_check_params,
      base::BindOnce([](bool* executed) { *executed = true; }, &executed));
  ASSERT_TRUE(executed);
  const auto* data = update_client()->data();
  ASSERT_NE(nullptr, data);
  ASSERT_EQ(1u, data->size());

  ASSERT_EQ(data->at(0)->version, extension1->version());
  update_client::CrxInstaller* installer = data->at(0)->installer.get();
  ASSERT_NE(installer, nullptr);

  // The GetInstalledFile method is used when processing differential updates
  // to get a path to an existing file in an extension. We want to test a
  // number of scenarios to be user we handle invalid relative paths, don't
  // accidentally return paths outside the extension's dir, etc.
  base::FilePath tmp;
  EXPECT_TRUE(installer->GetInstalledFile(foo_js.MaybeAsASCII(), &tmp));
  EXPECT_EQ(temp_dir.GetPath().Append(foo_js), tmp) << tmp.value();

  EXPECT_TRUE(installer->GetInstalledFile(bar_html.MaybeAsASCII(), &tmp));
  EXPECT_EQ(temp_dir.GetPath().Append(bar_html), tmp) << tmp.value();

  EXPECT_FALSE(installer->GetInstalledFile("does_not_exist", &tmp));
  EXPECT_FALSE(installer->GetInstalledFile("does/not/exist", &tmp));
  EXPECT_FALSE(installer->GetInstalledFile("/does/not/exist", &tmp));
  EXPECT_FALSE(installer->GetInstalledFile("C:\\tmp", &tmp));

  base::FilePath system_temp_dir;
  ASSERT_TRUE(base::GetTempDir(&system_temp_dir));
  EXPECT_FALSE(
      installer->GetInstalledFile(system_temp_dir.MaybeAsASCII(), &tmp));

  // Test the install callback.
  base::ScopedTempDir new_version_dir;
  ASSERT_TRUE(new_version_dir.CreateUniqueTempDir());

  bool done = false;
  installer->Install(
      new_version_dir.GetPath(), std::string(),
      base::BindOnce(
          [](bool* done, const update_client::CrxInstaller::Result& result) {
            *done = true;
            EXPECT_EQ(0, result.error);
            EXPECT_EQ(0, result.extended_error);
          },
          &done));

  scoped_refptr<content::MessageLoopRunner> loop_runner =
      base::MakeRefCounted<content::MessageLoopRunner>();
  extension_system()->set_install_callback(loop_runner->QuitClosure());
  loop_runner->Run();

  std::vector<FakeExtensionSystem::InstallUpdateRequest>* requests =
      extension_system()->install_requests();
  ASSERT_EQ(1u, requests->size());
  EXPECT_EQ(requests->at(0).extension_id, extension1->id());
  EXPECT_EQ(requests->at(0).temp_dir.value(),
            new_version_dir.GetPath().value());
  EXPECT_TRUE(done);
}

TEST_F(UpdateServiceTest, UninstallPings) {
  UninstallPingSender sender(ExtensionRegistry::Get(browser_context()),
                             base::Bind(&ShouldPing));

  // Build 3 extensions.
  scoped_refptr<Extension> extension1 =
      ExtensionBuilder("1").SetManifestKey("version", "1.2").Build();
  scoped_refptr<Extension> extension2 =
      ExtensionBuilder("2").SetManifestKey("version", "2.3").Build();
  scoped_refptr<Extension> extension3 =
      ExtensionBuilder("3").SetManifestKey("version", "3.4").Build();
  EXPECT_TRUE(extension1->id() != extension2->id() &&
              extension1->id() != extension3->id() &&
              extension2->id() != extension3->id());

  ExtensionRegistry* registry = ExtensionRegistry::Get(browser_context());

  // Run tests for each uninstall reason.
  for (int reason_val = static_cast<int>(UNINSTALL_REASON_FOR_TESTING);
       reason_val < static_cast<int>(UNINSTALL_REASON_MAX); ++reason_val) {
    UninstallReason reason = static_cast<UninstallReason>(reason_val);

    // Start with 2 enabled and 1 disabled extensions.
    EXPECT_TRUE(registry->AddEnabled(extension1)) << reason;
    EXPECT_TRUE(registry->AddEnabled(extension2)) << reason;
    EXPECT_TRUE(registry->AddDisabled(extension3)) << reason;

    // Uninstall the first extension, instructing our filter not to send pings,
    // and verify none were sent.
    g_should_ping = UninstallPingSender::DO_NOT_SEND_PING;
    EXPECT_TRUE(registry->RemoveEnabled(extension1->id())) << reason;
    registry->TriggerOnUninstalled(extension1.get(), reason);
    EXPECT_TRUE(update_client()->uninstall_pings().empty()) << reason;

    // Uninstall the second and third extensions, instructing the filter to
    // send pings, and make sure we got the expected data.
    g_should_ping = UninstallPingSender::SEND_PING;
    EXPECT_TRUE(registry->RemoveEnabled(extension2->id())) << reason;
    registry->TriggerOnUninstalled(extension2.get(), reason);
    EXPECT_TRUE(registry->RemoveDisabled(extension3->id())) << reason;
    registry->TriggerOnUninstalled(extension3.get(), reason);

    std::vector<FakeUpdateClient::UninstallPing>& pings =
        update_client()->uninstall_pings();
    ASSERT_EQ(2u, pings.size()) << reason;

    EXPECT_EQ(extension2->id(), pings[0].id) << reason;
    EXPECT_EQ(extension2->version(), pings[0].version) << reason;
    EXPECT_EQ(reason, pings[0].reason) << reason;

    EXPECT_EQ(extension3->id(), pings[1].id) << reason;
    EXPECT_EQ(extension3->version(), pings[1].version) << reason;
    EXPECT_EQ(reason, pings[1].reason) << reason;

    pings.clear();
  }
}

TEST_F(UpdateServiceTest, UpdateFoundNotification) {
  UpdateFoundNotificationObserver notification_observer("id", "2.0");

  // Fire UpdateClientEvents::COMPONENT_UPDATE_FOUND and verify that
  // NOTIFICATION_EXTENSION_UPDATE_FOUND notification is sent.
  update_client()->FireEvent(UpdateClientEvents::COMPONENT_UPDATE_FOUND, "id");
  EXPECT_TRUE(notification_observer.found_notification());

  notification_observer.reset();
  update_client()->FireEvent(UpdateClientEvents::COMPONENT_CHECKING_FOR_UPDATES,
                             "id");
  EXPECT_FALSE(notification_observer.found_notification());
}

TEST_F(UpdateServiceTest, InProgressUpdate_Successful) {
  update_client()->set_delay_update();
  ExtensionUpdateCheckParams update_check_params;
  update_check_params.update_info["A"] = ExtensionUpdateData();
  update_check_params.update_info["B"] = ExtensionUpdateData();
  update_check_params.update_info["C"] = ExtensionUpdateData();
  update_check_params.update_info["D"] = ExtensionUpdateData();
  update_check_params.update_info["E"] = ExtensionUpdateData();

  bool executed = false;
  update_service()->StartUpdateCheck(
      update_check_params,
      base::BindOnce([](bool* executed) { *executed = true; }, &executed));
  EXPECT_FALSE(executed);

  const auto& request = update_client()->update_request(0);
  EXPECT_THAT(request.extension_ids,
              testing::ElementsAre("A", "B", "C", "D", "E"));

  update_client()->RunDelayedUpdate(0);
  EXPECT_TRUE(executed);
}

TEST_F(UpdateServiceTest, InProgressUpdate_Duplicate) {
  update_client()->set_delay_update();
  ExtensionUpdateCheckParams uc1, uc2;
  uc1.update_info["A"] = ExtensionUpdateData();
  uc1.update_info["B"] = ExtensionUpdateData();
  uc1.update_info["C"] = ExtensionUpdateData();
  uc1.update_info["D"] = ExtensionUpdateData();
  uc1.update_info["E"] = ExtensionUpdateData();

  uc2.update_info["A"] = ExtensionUpdateData();
  uc2.update_info["B"] = ExtensionUpdateData();
  uc2.update_info["C"] = ExtensionUpdateData();
  uc2.update_info["D"] = ExtensionUpdateData();
  uc2.update_info["E"] = ExtensionUpdateData();

  bool executed1 = false;
  update_service()->StartUpdateCheck(
      uc1,
      base::BindOnce([](bool* executed) { *executed = true; }, &executed1));
  EXPECT_FALSE(executed1);

  bool executed2 = false;
  update_service()->StartUpdateCheck(
      uc2,
      base::BindOnce([](bool* executed) { *executed = true; }, &executed2));
  EXPECT_FALSE(executed2);

  ASSERT_EQ(1, update_client()->num_update_requests());

  const auto& request = update_client()->update_request(0);
  EXPECT_THAT(request.extension_ids,
              testing::ElementsAre("A", "B", "C", "D", "E"));

  update_client()->RunDelayedUpdate(0);
  EXPECT_TRUE(executed1);
  EXPECT_TRUE(executed2);
}

TEST_F(UpdateServiceTest, InProgressUpdate_NonOverlapped) {
  // 2 non-overallped update requests.
  update_client()->set_delay_update();
  ExtensionUpdateCheckParams uc1, uc2;

  uc1.update_info["A"] = ExtensionUpdateData();
  uc1.update_info["B"] = ExtensionUpdateData();
  uc1.update_info["C"] = ExtensionUpdateData();

  uc2.update_info["D"] = ExtensionUpdateData();
  uc2.update_info["E"] = ExtensionUpdateData();

  bool executed1 = false;
  update_service()->StartUpdateCheck(
      uc1,
      base::BindOnce([](bool* executed) { *executed = true; }, &executed1));
  EXPECT_FALSE(executed1);

  bool executed2 = false;
  update_service()->StartUpdateCheck(
      uc2,
      base::BindOnce([](bool* executed) { *executed = true; }, &executed2));
  EXPECT_FALSE(executed2);

  ASSERT_EQ(2, update_client()->num_update_requests());
  const auto& request1 = update_client()->update_request(0);
  const auto& request2 = update_client()->update_request(1);

  EXPECT_THAT(request1.extension_ids, testing::ElementsAre("A", "B", "C"));
  EXPECT_THAT(request2.extension_ids, testing::ElementsAre("D", "E"));

  update_client()->RunDelayedUpdate(0);
  EXPECT_TRUE(executed1);
  EXPECT_FALSE(executed2);

  update_client()->RunDelayedUpdate(1);
  EXPECT_TRUE(executed2);
}

TEST_F(UpdateServiceTest, InProgressUpdate_Overlapped) {
  update_client()->set_delay_update();
  ExtensionUpdateCheckParams uc1, uc2;

  uc1.update_info["A"] = ExtensionUpdateData();
  uc1.update_info["B"] = ExtensionUpdateData();
  uc1.update_info["C"] = ExtensionUpdateData();

  uc2.update_info["C"] = ExtensionUpdateData();
  uc2.update_info["D"] = ExtensionUpdateData();

  bool executed1 = false;
  update_service()->StartUpdateCheck(
      uc1,
      base::BindOnce([](bool* executed) { *executed = true; }, &executed1));
  EXPECT_FALSE(executed1);

  bool executed2 = false;
  update_service()->StartUpdateCheck(
      uc2,
      base::BindOnce([](bool* executed) { *executed = true; }, &executed2));
  EXPECT_FALSE(executed2);

  const auto& request1 = update_client()->update_request(0);
  const auto& request2 = update_client()->update_request(1);

  EXPECT_THAT(request1.extension_ids, testing::ElementsAre("A", "B", "C"));
  EXPECT_THAT(request2.extension_ids, testing::ElementsAre("D"));

  update_client()->RunDelayedUpdate(0);
  ASSERT_TRUE(executed1);
  ASSERT_FALSE(executed2);

  update_client()->RunDelayedUpdate(1);
  ASSERT_TRUE(executed2);
}

TEST_F(UpdateServiceTest, InProgressUpdate_3Overlapped) {
  // 3 overlapped requests. The 3rd request have all of its IDs in request1 and
  // request2.
  update_client()->set_delay_update();
  ExtensionUpdateCheckParams uc1, uc2, uc3;

  uc1.update_info["A"] = ExtensionUpdateData();
  uc1.update_info["B"] = ExtensionUpdateData();
  uc1.update_info["C"] = ExtensionUpdateData();

  uc2.update_info["C"] = ExtensionUpdateData();
  uc2.update_info["D"] = ExtensionUpdateData();
  uc2.update_info["E"] = ExtensionUpdateData();

  uc3.update_info["A"] = ExtensionUpdateData();
  uc3.update_info["B"] = ExtensionUpdateData();
  uc3.update_info["C"] = ExtensionUpdateData();
  uc3.update_info["D"] = ExtensionUpdateData();
  uc3.update_info["E"] = ExtensionUpdateData();

  bool executed1 = false;
  update_service()->StartUpdateCheck(
      uc1,
      base::BindOnce([](bool* executed) { *executed = true; }, &executed1));
  EXPECT_FALSE(executed1);

  bool executed2 = false;
  update_service()->StartUpdateCheck(
      uc2,
      base::BindOnce([](bool* executed) { *executed = true; }, &executed2));
  EXPECT_FALSE(executed2);

  bool executed3 = false;
  update_service()->StartUpdateCheck(
      uc3,
      base::BindOnce([](bool* executed) { *executed = true; }, &executed3));
  EXPECT_FALSE(executed3);

  ASSERT_EQ(2, update_client()->num_update_requests());
  const auto& request1 = update_client()->update_request(0);
  const auto& request2 = update_client()->update_request(1);

  EXPECT_THAT(request1.extension_ids, testing::ElementsAre("A", "B", "C"));
  EXPECT_THAT(request2.extension_ids, testing::ElementsAre("D", "E"));

  update_client()->RunDelayedUpdate(0);
  ASSERT_TRUE(executed1);
  ASSERT_FALSE(executed2);
  ASSERT_FALSE(executed3);

  update_client()->RunDelayedUpdate(1);
  ASSERT_TRUE(executed2);
  ASSERT_TRUE(executed3);
}

TEST_F(UpdateServiceTest, InProgressUpdate_4Overlapped) {
  // Similar to 3Overlapped, but the 4th request doesn't overlap with the first
  // 3 requests.
  update_client()->set_delay_update();
  ExtensionUpdateCheckParams uc1, uc2, uc3, uc4;

  uc1.update_info["A"] = ExtensionUpdateData();
  uc1.update_info["B"] = ExtensionUpdateData();
  uc1.update_info["C"] = ExtensionUpdateData();

  uc2.update_info["C"] = ExtensionUpdateData();
  uc2.update_info["D"] = ExtensionUpdateData();
  uc2.update_info["E"] = ExtensionUpdateData();

  uc3.update_info["A"] = ExtensionUpdateData();
  uc3.update_info["B"] = ExtensionUpdateData();
  uc3.update_info["C"] = ExtensionUpdateData();
  uc3.update_info["D"] = ExtensionUpdateData();
  uc3.update_info["E"] = ExtensionUpdateData();

  uc4.update_info["G"] = ExtensionUpdateData();
  uc4.update_info["H"] = ExtensionUpdateData();
  uc4.update_info["I"] = ExtensionUpdateData();
  uc4.update_info["J"] = ExtensionUpdateData();

  bool executed1 = false;
  update_service()->StartUpdateCheck(
      uc1,
      base::BindOnce([](bool* executed) { *executed = true; }, &executed1));
  EXPECT_FALSE(executed1);

  bool executed2 = false;
  update_service()->StartUpdateCheck(
      uc2,
      base::BindOnce([](bool* executed) { *executed = true; }, &executed2));
  EXPECT_FALSE(executed2);

  bool executed3 = false;
  update_service()->StartUpdateCheck(
      uc3,
      base::BindOnce([](bool* executed) { *executed = true; }, &executed3));
  EXPECT_FALSE(executed3);

  bool executed4 = false;
  update_service()->StartUpdateCheck(
      uc4,
      base::BindOnce([](bool* executed) { *executed = true; }, &executed4));
  EXPECT_FALSE(executed4);

  ASSERT_EQ(3, update_client()->num_update_requests());
  const auto& request1 = update_client()->update_request(0);
  const auto& request2 = update_client()->update_request(1);
  const auto& request3 = update_client()->update_request(2);

  EXPECT_THAT(request1.extension_ids, testing::ElementsAre("A", "B", "C"));
  EXPECT_THAT(request2.extension_ids, testing::ElementsAre("D", "E"));
  EXPECT_THAT(request3.extension_ids, testing::ElementsAre("G", "H", "I", "J"));

  update_client()->RunDelayedUpdate(0);
  ASSERT_TRUE(executed1);
  ASSERT_FALSE(executed2);
  ASSERT_FALSE(executed3);
  ASSERT_FALSE(executed4);

  update_client()->RunDelayedUpdate(1);
  ASSERT_TRUE(executed2);
  ASSERT_TRUE(executed3);
  ASSERT_FALSE(executed4);

  update_client()->RunDelayedUpdate(2);
  ASSERT_TRUE(executed4);
}

TEST_F(UpdateServiceTest, InProgressUpdate_Batch) {
  // Verify that extensions are batched when the number of extensions exceeds
  // 20.
  update_client()->set_delay_update();
  ExtensionUpdateCheckParams uc;

  for (int i = 0; i < 50; ++i)
    uc.update_info[base::StringPrintf("A%02d", i)] = ExtensionUpdateData();

  bool executed = false;
  update_service()->StartUpdateCheck(
      uc, base::BindOnce([](bool* executed) { *executed = true; }, &executed));
  EXPECT_FALSE(executed);

  ASSERT_EQ(3, update_client()->num_update_requests());

  const auto& request1 = update_client()->update_request(0);
  const auto& request2 = update_client()->update_request(1);
  const auto& request3 = update_client()->update_request(2);

  EXPECT_THAT(
      request1.extension_ids,
      testing::ElementsAre("A00", "A01", "A02", "A03", "A04", "A05", "A06",
                           "A07", "A08", "A09", "A10", "A11", "A12", "A13",
                           "A14", "A15", "A16", "A17", "A18", "A19"));
  EXPECT_THAT(
      request2.extension_ids,
      testing::ElementsAre("A20", "A21", "A22", "A23", "A24", "A25", "A26",
                           "A27", "A28", "A29", "A30", "A31", "A32", "A33",
                           "A34", "A35", "A36", "A37", "A38", "A39"));
  EXPECT_THAT(request3.extension_ids,
              testing::ElementsAre("A40", "A41", "A42", "A43", "A44", "A45",
                                   "A46", "A47", "A48", "A49"));

  update_client()->RunDelayedUpdate(0);
  EXPECT_FALSE(executed);

  update_client()->RunDelayedUpdate(1);
  EXPECT_FALSE(executed);

  update_client()->RunDelayedUpdate(2);
  EXPECT_TRUE(executed);
}

TEST_F(UpdateServiceTest, InProgressUpdate_NoBatchAndBatch) {
  update_client()->set_delay_update();
  ExtensionUpdateCheckParams uc1;
  ExtensionUpdateCheckParams uc2;

  uc1.update_info["AA"] = ExtensionUpdateData();
  uc1.update_info["BB"] = ExtensionUpdateData();
  uc1.update_info["CC"] = ExtensionUpdateData();
  uc1.update_info["DD"] = ExtensionUpdateData();

  for (int i = 0; i < 50; ++i)
    uc2.update_info[base::StringPrintf("A%02d", i)] = ExtensionUpdateData();

  bool executed1 = false;
  update_service()->StartUpdateCheck(
      uc1,
      base::BindOnce([](bool* executed) { *executed = true; }, &executed1));
  EXPECT_FALSE(executed1);

  bool executed2 = false;
  update_service()->StartUpdateCheck(
      uc2,
      base::BindOnce([](bool* executed) { *executed = true; }, &executed2));
  EXPECT_FALSE(executed2);

  ASSERT_EQ(4, update_client()->num_update_requests());

  const auto& request1 = update_client()->update_request(0);
  const auto& request2 = update_client()->update_request(1);
  const auto& request3 = update_client()->update_request(2);
  const auto& request4 = update_client()->update_request(3);

  EXPECT_THAT(request1.extension_ids,
              testing::ElementsAre("AA", "BB", "CC", "DD"));

  EXPECT_THAT(
      request2.extension_ids,
      testing::ElementsAre("A00", "A01", "A02", "A03", "A04", "A05", "A06",
                           "A07", "A08", "A09", "A10", "A11", "A12", "A13",
                           "A14", "A15", "A16", "A17", "A18", "A19"));
  EXPECT_THAT(
      request3.extension_ids,
      testing::ElementsAre("A20", "A21", "A22", "A23", "A24", "A25", "A26",
                           "A27", "A28", "A29", "A30", "A31", "A32", "A33",
                           "A34", "A35", "A36", "A37", "A38", "A39"));
  EXPECT_THAT(request4.extension_ids,
              testing::ElementsAre("A40", "A41", "A42", "A43", "A44", "A45",
                                   "A46", "A47", "A48", "A49"));

  update_client()->RunDelayedUpdate(0);
  EXPECT_TRUE(executed1);
  EXPECT_FALSE(executed2);

  update_client()->RunDelayedUpdate(1);
  EXPECT_FALSE(executed2);

  update_client()->RunDelayedUpdate(2);
  EXPECT_FALSE(executed2);

  update_client()->RunDelayedUpdate(3);
  EXPECT_TRUE(executed2);
}

class UpdateServiceCanUpdateTest : public UpdateServiceTest,
                                   public ::testing::WithParamInterface<bool> {
 public:
  UpdateServiceCanUpdateTest() {}
  ~UpdateServiceCanUpdateTest() override {}

  void SetUp() override {
    UpdateServiceTest::SetUp();

    if (GetParam()) {
      scoped_feature_list_.InitAndEnableFeature(
          features::kNewExtensionUpdaterService);
    } else {
      scoped_feature_list_.InitAndDisableFeature(
          features::kNewExtensionUpdaterService);
    }

    store_extension_ =
        ExtensionBuilder("store_extension")
            .MergeManifest(
                DictionaryBuilder()
                    .Set("update_url",
                         extension_urls::GetDefaultWebstoreUpdateUrl().spec())
                    .Build())
            .Build();
    offstore_extension_ =
        ExtensionBuilder("offstore_extension")
            .MergeManifest(
                DictionaryBuilder()
                    .Set("update_url", "http://localhost/test/updates.xml")
                    .Build())
            .Build();

    ASSERT_TRUE(store_extension_.get());
    ASSERT_TRUE(ExtensionRegistry::Get(browser_context())
                    ->AddEnabled(store_extension_));
    ASSERT_TRUE(offstore_extension_.get());
    ASSERT_TRUE(ExtensionRegistry::Get(browser_context())
                    ->AddEnabled(offstore_extension_));
  }

 protected:
  base::test::ScopedFeatureList scoped_feature_list_;
  scoped_refptr<Extension> store_extension_;
  scoped_refptr<Extension> offstore_extension_;
};

class UpdateServiceCanUpdateFeatureEnabledNonDefaultUpdateUrl
    : public UpdateServiceCanUpdateTest {
 public:
  void SetUp() override {
    UpdateServiceCanUpdateTest::SetUp();

    // Change the webstore update url.
    auto* command_line = base::CommandLine::ForCurrentProcess();
    // Note: |offstore_extension_|'s update url is the same.
    command_line->AppendSwitchASCII("apps-gallery-update-url",
                                    "http://localhost/test/updates.xml");
    ExtensionsClient::Get()->InitializeWebStoreUrls(
        base::CommandLine::ForCurrentProcess());
  }
};

TEST_P(UpdateServiceCanUpdateTest, UpdateService_CanUpdate) {
  // Update service can only update webstore extensions when enabled.
  EXPECT_EQ(GetParam(), update_service()->CanUpdate(store_extension_->id()));
  // It can't update off-store extrensions.
  EXPECT_FALSE(update_service()->CanUpdate(offstore_extension_->id()));
  // ... or extensions that don't exist.
  EXPECT_FALSE(update_service()->CanUpdate(std::string(32, 'a')));
}

TEST_P(UpdateServiceCanUpdateFeatureEnabledNonDefaultUpdateUrl,
       UpdateService_CanUpdate) {
  // Update service cannot update extensions when the default webstore update
  // url is changed.
  EXPECT_FALSE(update_service()->CanUpdate(store_extension_->id()));
  EXPECT_FALSE(update_service()->CanUpdate(offstore_extension_->id()));
  EXPECT_FALSE(update_service()->CanUpdate(std::string(32, 'a')));
}

INSTANTIATE_TEST_CASE_P(CanUpdateTest,
                        UpdateServiceCanUpdateTest,
                        ::testing::Bool());

INSTANTIATE_TEST_CASE_P(CanUpdateTest,
                        UpdateServiceCanUpdateFeatureEnabledNonDefaultUpdateUrl,
                        ::testing::Bool());

}  // namespace

}  // namespace extensions
