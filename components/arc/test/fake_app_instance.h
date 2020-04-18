// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_ARC_TEST_FAKE_APP_INSTANCE_H_
#define COMPONENTS_ARC_TEST_FAKE_APP_INSTANCE_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "components/arc/common/app.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace arc {

class FakeAppInstance : public mojom::AppInstance {
 public:
  class Request {
   public:
    Request(const std::string& package_name, const std::string& activity)
        : package_name_(package_name), activity_(activity) {}
    ~Request() {}

    const std::string& package_name() const { return package_name_; }

    const std::string& activity() const { return activity_; }

    bool IsForApp(const mojom::AppInfo& app_info) const {
      return package_name_ == app_info.package_name &&
             activity_ == app_info.activity;
    }

   private:
    std::string package_name_;
    std::string activity_;

    DISALLOW_COPY_AND_ASSIGN(Request);
  };

  class IconRequest : public Request {
   public:
    IconRequest(const std::string& package_name,
                const std::string& activity,
                mojom::ScaleFactor scale_factor)
        : Request(package_name, activity),
          scale_factor_(static_cast<int>(scale_factor)) {}
    ~IconRequest() {}

    int scale_factor() const { return scale_factor_; }

   private:
    int scale_factor_;

    DISALLOW_COPY_AND_ASSIGN(IconRequest);
  };

  class ShortcutIconRequest {
   public:
    ShortcutIconRequest(const std::string& icon_resource_id,
                        mojom::ScaleFactor scale_factor)
        : icon_resource_id_(icon_resource_id),
          scale_factor_(static_cast<int>(scale_factor)) {}
    ~ShortcutIconRequest() {}

    const std::string& icon_resource_id() const { return icon_resource_id_; }
    int scale_factor() const { return scale_factor_; }

   private:
    std::string icon_resource_id_;
    int scale_factor_;

    DISALLOW_COPY_AND_ASSIGN(ShortcutIconRequest);
  };

  explicit FakeAppInstance(mojom::AppHost* app_host);
  ~FakeAppInstance() override;

  // mojom::AppInstance overrides:
  void InitDeprecated(mojom::AppHostPtr host_ptr) override;
  void Init(mojom::AppHostPtr host_ptr, InitCallback callback) override;
  void RefreshAppList() override;
  void LaunchAppDeprecated(const std::string& package_name,
                           const std::string& activity,
                           const base::Optional<gfx::Rect>& dimension) override;
  void LaunchApp(const std::string& package_name,
                 const std::string& activity,
                 int64_t display_id) override;
  void LaunchAppShortcutItem(const std::string& package_name,
                             const std::string& shortcut_id,
                             int64_t display_id) override;
  void RequestAppIcon(const std::string& package_name,
                      const std::string& activity,
                      mojom::ScaleFactor scale_factor) override;
  void LaunchIntentDeprecated(
      const std::string& intent_uri,
      const base::Optional<gfx::Rect>& dimension_on_screen) override;
  void LaunchIntent(const std::string& intent_uri, int64_t display_id) override;
  void RequestIcon(const std::string& icon_resource_id,
                   mojom::ScaleFactor scale_factor,
                   RequestIconCallback callback) override;
  void RemoveCachedIcon(const std::string& icon_resource_id) override;
  void CanHandleResolutionDeprecated(
      const std::string& package_name,
      const std::string& activity,
      const gfx::Rect& dimension,
      CanHandleResolutionDeprecatedCallback callback) override;
  void UninstallPackage(const std::string& package_name) override;
  void GetTaskInfo(int32_t task_id, GetTaskInfoCallback callback) override;
  void SetTaskActive(int32_t task_id) override;
  void CloseTask(int32_t task_id) override;
  void ShowPackageInfoDeprecated(const std::string& package_name,
                                 const gfx::Rect& dimension_on_screen) override;
  void ShowPackageInfoOnPageDeprecated(
      const std::string& package_name,
      mojom::ShowPackageInfoPage page,
      const gfx::Rect& dimension_on_screen) override;
  void ShowPackageInfoOnPage(const std::string& package_name,
                             mojom::ShowPackageInfoPage page,
                             int64_t display_id) override;
  void SetNotificationsEnabled(const std::string& package_name,
                               bool enabled) override;
  void InstallPackage(mojom::ArcPackageInfoPtr arcPackageInfo) override;
  void GetRecentAndSuggestedAppsFromPlayStore(
      const std::string& query,
      int32_t max_results,
      GetRecentAndSuggestedAppsFromPlayStoreCallback callback) override;
  void GetIcingGlobalQueryResults(
      const std::string& query,
      int32_t max_results,
      GetIcingGlobalQueryResultsCallback callback) override;
  void GetAppShortcutItems(const std::string& package_name,
                           GetAppShortcutItemsCallback callback) override;
  void StartPaiFlow() override;

  // Methods to reply messages.
  void SendRefreshAppList(const std::vector<mojom::AppInfo>& apps);
  void SendAppAdded(const mojom::AppInfo& app);
  void SendPackageAppListRefreshed(const std::string& package_name,
                                   const std::vector<mojom::AppInfo>& apps);
  void SendTaskCreated(int32_t taskId,
                       const mojom::AppInfo& app,
                       const std::string& intent);
  void SendTaskDescription(int32_t taskId,
                           const std::string& label,
                           const std::string& icon_png_data_as_string);
  void SendTaskDestroyed(int32_t taskId);
  bool GenerateAndSendIcon(const mojom::AppInfo& app,
                           mojom::ScaleFactor scale_factor,
                           std::string* png_data_as_string);
  void GenerateAndSendBadIcon(const mojom::AppInfo& app,
                              mojom::ScaleFactor scale_factor);
  void SendInstallShortcut(const mojom::ShortcutInfo& shortcut);
  void SendUninstallShortcut(const std::string& package_name,
                             const std::string& intent_uri);
  void SendInstallShortcuts(const std::vector<mojom::ShortcutInfo>& shortcuts);
  void SetTaskInfo(int32_t task_id,
                   const std::string& package_name,
                   const std::string& activity);
  void SendRefreshPackageList(
      const std::vector<mojom::ArcPackageInfo>& packages);
  void SendPackageAdded(const mojom::ArcPackageInfo& package);
  void SendPackageModified(const mojom::ArcPackageInfo& package);
  void SendPackageUninstalled(const std::string& pacakge_name);

  void SendInstallationStarted(const std::string& package_name);
  void SendInstallationFinished(const std::string& package_name,
                                bool success);

  int refresh_app_list_count() const { return refresh_app_list_count_; }

  int start_pai_request_count() const { return start_pai_request_count_; }

  int launch_app_shortcut_item_count() const {
    return launch_app_shortcut_item_count_;
  }

  const std::vector<std::unique_ptr<Request>>& launch_requests() const {
    return launch_requests_;
  }

  const std::vector<std::string>& launch_intents() const {
    return launch_intents_;
  }

  const std::vector<std::unique_ptr<IconRequest>>& icon_requests() const {
    return icon_requests_;
  }

  const std::vector<std::unique_ptr<ShortcutIconRequest>>&
  shortcut_icon_requests() const {
    return shortcut_icon_requests_;
  }

 private:
  using TaskIdToInfo = std::map<int32_t, std::unique_ptr<Request>>;
  // Mojo endpoints.
  mojom::AppHost* app_host_;
  // Number of RefreshAppList calls.
  int refresh_app_list_count_ = 0;
  // Number of requests to start PAI flows.
  int start_pai_request_count_ = 0;
  // Keeps information about launch app shortcut requests.
  int launch_app_shortcut_item_count_ = 0;
  // Keeps information about launch requests.
  std::vector<std::unique_ptr<Request>> launch_requests_;
  // Keeps information about launch intents.
  std::vector<std::string> launch_intents_;
  // Keeps information about icon load requests.
  std::vector<std::unique_ptr<IconRequest>> icon_requests_;
  // Keeps information about shortcut icon load requests.
  std::vector<std::unique_ptr<ShortcutIconRequest>> shortcut_icon_requests_;
  // Keeps information for running tasks.
  TaskIdToInfo task_id_to_info_;

  // Keeps the binding alive so that calls to this class can be correctly
  // routed.
  mojom::AppHostPtr host_;

  bool GetFakeIcon(mojom::ScaleFactor scale_factor,
                   std::string* png_data_as_string);

  DISALLOW_COPY_AND_ASSIGN(FakeAppInstance);
};

}  // namespace arc

#endif  // COMPONENTS_ARC_TEST_FAKE_APP_INSTANCE_H_
