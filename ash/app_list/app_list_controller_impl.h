// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_APP_LIST_APP_LIST_CONTROLLER_IMPL_H_
#define ASH_APP_LIST_APP_LIST_CONTROLLER_IMPL_H_

#include <memory>
#include <string>
#include <vector>

#include "ash/app_list/app_list_presenter_impl.h"
#include "ash/app_list/model/app_list_model.h"
#include "ash/app_list/model/app_list_model_observer.h"
#include "ash/app_list/model/app_list_view_state.h"
#include "ash/app_list/model/search/search_model.h"
#include "ash/ash_export.h"
#include "ash/public/cpp/app_list/app_list_constants.h"
#include "ash/public/interfaces/app_list.mojom.h"
#include "ash/session/session_observer.h"
#include "ash/shell_observer.h"
#include "ash/wallpaper/wallpaper_controller_observer.h"
#include "ash/wm/tablet_mode/tablet_mode_observer.h"
#include "base/scoped_observer.h"
#include "components/sync/model/string_ordinal.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "mojo/public/cpp/bindings/interface_ptr_set.h"
#include "ui/app_list/app_list_view_delegate.h"
#include "ui/keyboard/keyboard_controller_observer.h"

namespace ui {
class MouseWheelEvent;
}  // namespace ui

namespace app_list {
class AnswerCardContentsRegistry;
}  // namespace app_list

namespace ash {

// Ash's AppListController owns the AppListModel and implements interface
// functions that allow Chrome to modify and observe the Shelf and AppListModel
// state.
class ASH_EXPORT AppListControllerImpl
    : public mojom::AppListController,
      public SessionObserver,
      public app_list::AppListModelObserver,
      public app_list::AppListViewDelegate,
      public ash::ShellObserver,
      public TabletModeObserver,
      public keyboard::KeyboardControllerObserver,
      public WallpaperControllerObserver {
 public:
  using AppListItemMetadataPtr = mojom::AppListItemMetadataPtr;
  using SearchResultMetadataPtr = mojom::SearchResultMetadataPtr;
  AppListControllerImpl();
  ~AppListControllerImpl() override;

  // Binds the mojom::AppListController interface request to this object.
  void BindRequest(mojom::AppListControllerRequest request);

  app_list::AppListPresenterImpl* presenter() { return &presenter_; }

  // mojom::AppListController:
  void SetClient(mojom::AppListClientPtr client_ptr) override;
  void AddItem(AppListItemMetadataPtr app_item) override;
  void AddItemToFolder(AppListItemMetadataPtr app_item,
                       const std::string& folder_id) override;
  void RemoveItem(const std::string& id) override;
  void RemoveUninstalledItem(const std::string& id) override;
  void MoveItemToFolder(const std::string& id,
                        const std::string& folder_id) override;
  void SetStatus(ash::AppListModelStatus status) override;
  void SetState(ash::AppListState state) override;
  void HighlightItemInstalledFromUI(const std::string& id) override;
  void SetSearchEngineIsGoogle(bool is_google) override;
  void SetSearchTabletAndClamshellAccessibleName(
      const base::string16& tablet_accessible_name,
      const base::string16& clamshell_accessible_name) override;
  void SetSearchHintText(const base::string16& hint_text) override;
  void UpdateSearchBox(const base::string16& text,
                       bool initiated_by_user) override;
  void PublishSearchResults(
      std::vector<SearchResultMetadataPtr> results) override;
  void SetItemMetadata(const std::string& id,
                       AppListItemMetadataPtr data) override;
  void SetItemIcon(const std::string& id, const gfx::ImageSkia& icon) override;
  void SetItemIsInstalling(const std::string& id, bool is_installing) override;
  void SetItemPercentDownloaded(const std::string& id,
                                int32_t percent_downloaded) override;
  void SetModelData(std::vector<AppListItemMetadataPtr> apps,
                    bool is_search_engine_google) override;

  void SetSearchResultMetadata(SearchResultMetadataPtr metadata) override;
  void SetSearchResultIsInstalling(const std::string& id,
                                   bool is_installing) override;
  void SetSearchResultPercentDownloaded(const std::string& id,
                                        int32_t percent_downloaded) override;
  void NotifySearchResultItemInstalled(const std::string& id) override;

  void GetIdToAppListIndexMap(GetIdToAppListIndexMapCallback callback) override;
  void FindOrCreateOemFolder(
      const std::string& oem_folder_name,
      const syncer::StringOrdinal& preferred_oem_position,
      FindOrCreateOemFolderCallback callback) override;
  void ResolveOemFolderPosition(
      const syncer::StringOrdinal& preferred_oem_position,
      ResolveOemFolderPositionCallback callback) override;

  void DismissAppList() override;
  void GetAppInfoDialogBounds(GetAppInfoDialogBoundsCallback callback) override;
  void ShowAppListAndSwitchToState(ash::AppListState state) override;
  void ShowAppList() override;

  // app_list::AppListModelObserver:
  void OnAppListItemAdded(app_list::AppListItem* item) override;
  void OnAppListItemWillBeDeleted(app_list::AppListItem* item) override;
  void OnAppListItemUpdated(app_list::AppListItem* item) override;

  // SessionObserver:
  void OnSessionStateChanged(session_manager::SessionState state) override;

  // Methods used in ash:
  bool GetTargetVisibility() const;
  bool IsVisible() const;
  void Show(int64_t display_id,
            app_list::AppListShowSource show_source,
            base::TimeTicks event_time_stamp);
  void UpdateYPositionAndOpacity(int y_position_in_screen,
                                 float background_opacity);
  void EndDragFromShelf(app_list::AppListViewState app_list_state);
  void ProcessMouseWheelEvent(const ui::MouseWheelEvent& event);
  void ToggleAppList(int64_t display_id,
                     app_list::AppListShowSource show_source,
                     base::TimeTicks event_time_stamp);
  app_list::AppListViewState GetAppListViewState();

  // app_list::AppListViewDelegate:
  app_list::AppListModel* GetModel() override;
  app_list::SearchModel* GetSearchModel() override;
  void StartSearch(const base::string16& raw_query) override;
  void OpenSearchResult(const std::string& result_id, int event_flags) override;
  void InvokeSearchResultAction(const std::string& result_id,
                                int action_index,
                                int event_flags) override;
  using GetContextMenuModelCallback =
      AppListViewDelegate::GetContextMenuModelCallback;
  void GetSearchResultContextMenuModel(
      const std::string& result_id,
      GetContextMenuModelCallback callback) override;
  void SearchResultContextMenuItemSelected(const std::string& result_id,
                                           int command_id,
                                           int event_flags) override;
  void ViewShown(int64_t display_id) override;
  void ViewClosing() override;
  void GetWallpaperProminentColors(
      GetWallpaperProminentColorsCallback callback) override;
  void ActivateItem(const std::string& id, int event_flags) override;
  void GetContextMenuModel(const std::string& id,
                           GetContextMenuModelCallback callback) override;
  void ContextMenuItemSelected(const std::string& id,
                               int command_id,
                               int event_flags) override;
  void ShowWallpaperContextMenu(const gfx::Point& onscreen_location,
                                ui::MenuSourceType source_type) override;
  void OnVisibilityChanged(bool visible);
  void OnTargetVisibilityChanged(bool visible);
  void StartVoiceInteractionSession();
  void ToggleVoiceInteractionSession();

  void FlushForTesting();

  // ShellObserver:
  void OnVirtualKeyboardStateChanged(bool activated,
                                     aura::Window* root_window) override;
  void OnOverviewModeStarting() override;
  void OnOverviewModeEnding() override;

  // TabletModeObserver:
  void OnTabletModeStarted() override;
  void OnTabletModeEnded() override;

  // KeyboardControllerObserver:
  void OnKeyboardAvailabilityChanged(const bool is_available) override;

  // WallpaperControllerObserver:
  void OnWallpaperColorsChanged() override;
  void OnWallpaperPreviewStarted() override;
  void OnWallpaperPreviewEnded() override;

  bool onscreen_keyboard_shown() const { return onscreen_keyboard_shown_; }

  // Returns true if the home launcher is enabled in tablet mode.
  bool IsHomeLauncherEnabledInTabletMode() const;

 private:
  syncer::StringOrdinal GetOemFolderPos();
  std::unique_ptr<app_list::AppListItem> CreateAppListItem(
      AppListItemMetadataPtr metadata);
  app_list::AppListFolderItem* FindFolderItem(const std::string& folder_id);

  // Update the visibility of the home launcher based on e.g. if the device is
  // in overview mode.
  void UpdateHomeLauncherVisibility();

  base::string16 last_raw_query_;

  mojom::AppListClientPtr client_;

  app_list::AppListModel model_;
  app_list::SearchModel search_model_;

  // |presenter_| should be put below |client_| and |model_| to prevent a crash
  // in destruction.
  app_list::AppListPresenterImpl presenter_;

  // Bindings for the AppListController interface.
  mojo::BindingSet<mojom::AppListController> bindings_;

  // Token to view map for classic/mus ash (i.e. non-mash).
  std::unique_ptr<app_list::AnswerCardContentsRegistry>
      answer_card_contents_registry_;

  ScopedObserver<keyboard::KeyboardController,
                 keyboard::KeyboardControllerObserver>
      keyboard_observer_;

  // Whether the on-screen keyboard is shown.
  bool onscreen_keyboard_shown_ = false;

  // Whether the home launcher feature is enabled.
  const bool is_home_launcher_enabled_;

  // Whether the device is in overview mode. The home launcher (if enabled)
  // should be hidden during overview mode.
  bool in_overview_mode_ = false;

  // Whether the wallpaper is being previewed. The home launcher (if enabled)
  // should be hidden during wallpaper preview.
  bool in_wallpaper_preview_ = false;

  DISALLOW_COPY_AND_ASSIGN(AppListControllerImpl);
};

}  // namespace ash

#endif  // ASH_APP_LIST_APP_LIST_CONTROLLER_IMPL_H_
