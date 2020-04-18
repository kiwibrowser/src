// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_MEDIA_ROUTER_MEDIA_ROUTER_VIEWS_UI_H_
#define CHROME_BROWSER_UI_VIEWS_MEDIA_ROUTER_MEDIA_ROUTER_VIEWS_UI_H_

#include "base/macros.h"
#include "base/observer_list.h"
#include "chrome/browser/ui/media_router/cast_dialog_controller.h"
#include "chrome/browser/ui/media_router/cast_dialog_model.h"
#include "chrome/browser/ui/media_router/media_router_ui_base.h"

namespace media_router {

// Functions as an intermediary between MediaRouter and Views Cast dialog.
class MediaRouterViewsUI : public MediaRouterUIBase,
                           public CastDialogController {
 public:
  MediaRouterViewsUI();
  ~MediaRouterViewsUI() override;

  // CastDialogController:
  void AddObserver(CastDialogController::Observer* observer) override;
  void RemoveObserver(CastDialogController::Observer* observer) override;
  void StartCasting(const std::string& sink_id,
                    MediaCastMode cast_mode) override;
  void StopCasting(const std::string& route_id) override;

 private:
  FRIEND_TEST_ALL_PREFIXES(MediaRouterViewsUITest, NotifyObserver);

  // MediaRouterUIBase:
  void OnRoutesUpdated(
      const std::vector<MediaRoute>& routes,
      const std::vector<MediaRoute::Id>& joinable_route_ids) override;
  void UpdateSinks() override;

  // Contains up-to-date data to show in the dialog.
  CastDialogModel model_;

  // Observers for dialog model updates.
  base::ObserverList<CastDialogController::Observer> observers_;

  DISALLOW_COPY_AND_ASSIGN(MediaRouterViewsUI);
};

}  // namespace media_router

#endif  // CHROME_BROWSER_UI_VIEWS_MEDIA_ROUTER_MEDIA_ROUTER_VIEWS_UI_H_
