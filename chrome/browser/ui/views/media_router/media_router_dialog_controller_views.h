// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_VIEWS_MEDIA_ROUTER_MEDIA_ROUTER_DIALOG_CONTROLLER_VIEWS_H_
#define CHROME_BROWSER_UI_VIEWS_MEDIA_ROUTER_MEDIA_ROUTER_DIALOG_CONTROLLER_VIEWS_H_

#include "base/macros.h"
#include "chrome/browser/ui/media_router/media_router_dialog_controller_impl_base.h"
#include "chrome/browser/ui/views/media_router/media_router_views_ui.h"
#include "ui/views/widget/widget_observer.h"

namespace media_router {

// A Views implementation of MediaRouterDialogController.
class MediaRouterDialogControllerViews
    : public content::WebContentsUserData<MediaRouterDialogControllerViews>,
      public MediaRouterDialogControllerImplBase,
      public views::WidgetObserver {
 public:
  ~MediaRouterDialogControllerViews() override;

  static MediaRouterDialogControllerViews* GetOrCreateForWebContents(
      content::WebContents* web_contents);

  // MediaRouterDialogController:
  void CreateMediaRouterDialog() override;
  void CloseMediaRouterDialog() override;
  bool IsShowingMediaRouterDialog() const override;
  void Reset() override;

  // views::WidgetObserver:
  void OnWidgetClosing(views::Widget* widget) override;

 private:
  friend class content::WebContentsUserData<MediaRouterDialogControllerViews>;

  // Use MediaRouterDialogController::GetOrCreateForWebContents() to create
  // an instance.
  explicit MediaRouterDialogControllerViews(content::WebContents* web_contents);

  // Responsible for notifying the dialog view of dialog model updates and
  // sending route requests to MediaRouter. Set to nullptr when the dialog is
  // closed.
  std::unique_ptr<MediaRouterViewsUI> ui_;

  DISALLOW_COPY_AND_ASSIGN(MediaRouterDialogControllerViews);
};

}  // namespace media_router

#endif  // CHROME_BROWSER_UI_VIEWS_MEDIA_ROUTER_MEDIA_ROUTER_DIALOG_CONTROLLER_VIEWS_H_
