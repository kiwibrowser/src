// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_MEDIA_ROUTER_MEDIA_ROUTER_DIALOG_CONTROLLER_IMPL_BASE_H_
#define CHROME_BROWSER_UI_MEDIA_ROUTER_MEDIA_ROUTER_DIALOG_CONTROLLER_IMPL_BASE_H_

#include "base/macros.h"
#include "chrome/browser/media/router/media_router_dialog_controller.h"

class MediaRouterAction;
class MediaRouterActionController;

namespace media_router {

class MediaRouterUIBase;

// The base class for desktop implementations of MediaRouterDialogController.
// This class is not thread safe and must be called on the UI thread.
class MediaRouterDialogControllerImplBase : public MediaRouterDialogController {
 public:
  ~MediaRouterDialogControllerImplBase() override;

  static MediaRouterDialogControllerImplBase* GetOrCreateForWebContents(
      content::WebContents* web_contents);

  // Sets the action to notify when a dialog gets shown or hidden.
  void SetMediaRouterAction(const base::WeakPtr<MediaRouterAction>& action);

  // MediaRouterDialogController:
  void CreateMediaRouterDialog() override;
  void Reset() override;

  MediaRouterAction* action() { return action_.get(); }

 protected:
  // Use MediaRouterDialogControllerImplBase::CreateForWebContents() to create
  // an instance.
  explicit MediaRouterDialogControllerImplBase(
      content::WebContents* web_contents);

  // Called by subclasses to initialize |media_router_ui| that they use.
  void InitializeMediaRouterUI(MediaRouterUIBase* media_router_ui);

 private:
  // |action_| refers to the MediaRouterAction on the toolbar, rather than
  // overflow menu. A MediaRouterAction is always created for the toolbar
  // first. Any subsequent creations for the overflow menu will not be set as
  // |action_|.
  // The lifetime of |action_| is dependent on the creation and destruction of
  // a browser window. The overflow menu's MediaRouterAction is only created
  // when the overflow menu is opened and destroyed when the menu is closed.
  base::WeakPtr<MediaRouterAction> action_;

  // |action_controller_| is responsible for showing and hiding the toolbar
  // action. It's owned by MediaRouterUIService, which outlives |this|.
  MediaRouterActionController* const action_controller_;

  DISALLOW_COPY_AND_ASSIGN(MediaRouterDialogControllerImplBase);
};

}  // namespace media_router

#endif  // CHROME_BROWSER_UI_MEDIA_ROUTER_MEDIA_ROUTER_DIALOG_CONTROLLER_IMPL_BASE_H_
