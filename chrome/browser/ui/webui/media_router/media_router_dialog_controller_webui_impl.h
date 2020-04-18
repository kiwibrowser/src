// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_MEDIA_ROUTER_MEDIA_ROUTER_DIALOG_CONTROLLER_WEBUI_IMPL_H_
#define CHROME_BROWSER_UI_WEBUI_MEDIA_ROUTER_MEDIA_ROUTER_DIALOG_CONTROLLER_WEBUI_IMPL_H_

#include <memory>

#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/ui/media_router/media_router_dialog_controller_impl_base.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

FORWARD_DECLARE_TEST(MediaRouterActionUnitTest, IconPressedState);

namespace media_router {

// A WebUI implementation of MediaRouterDialogController.
// This class is not thread safe and must be called on the UI thread.
class MediaRouterDialogControllerWebUIImpl
    : public content::WebContentsUserData<MediaRouterDialogControllerWebUIImpl>,
      public MediaRouterDialogControllerImplBase {
 public:
  static MediaRouterDialogControllerWebUIImpl* GetOrCreateForWebContents(
      content::WebContents* web_contents);

  ~MediaRouterDialogControllerWebUIImpl() override;

  // Returns the media router dialog WebContents.
  // Returns nullptr if there is no dialog.
  content::WebContents* GetMediaRouterDialog() const;

  // MediaRouterDialogController:
  void CreateMediaRouterDialog() override;
  void CloseMediaRouterDialog() override;
  bool IsShowingMediaRouterDialog() const override;
  void Reset() override;

  void UpdateMaxDialogSize();

 private:
  class DialogWebContentsObserver;
  friend class content::WebContentsUserData<
      MediaRouterDialogControllerWebUIImpl>;
  FRIEND_TEST_ALL_PREFIXES(::MediaRouterActionUnitTest, IconPressedState);

  // Use MediaRouterDialogControllerWebUIImpl::CreateForWebContents() to create
  // an instance.
  explicit MediaRouterDialogControllerWebUIImpl(
      content::WebContents* web_contents);

  // Invoked when the dialog WebContents has navigated.
  void OnDialogNavigated(const content::LoadCommittedDetails& details);

  void PopulateDialog(content::WebContents* media_router_dialog);

  std::unique_ptr<DialogWebContentsObserver> dialog_observer_;

  // True if the controller is waiting for a new media router dialog to be
  // created.
  bool media_router_dialog_pending_;

  base::WeakPtrFactory<MediaRouterDialogControllerWebUIImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(MediaRouterDialogControllerWebUIImpl);
};

}  // namespace media_router

#endif  // CHROME_BROWSER_UI_WEBUI_MEDIA_ROUTER_MEDIA_ROUTER_DIALOG_CONTROLLER_WEBUI_IMPL_H_
