// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_ASH_CHROME_NEW_WINDOW_CLIENT_H_
#define CHROME_BROWSER_UI_ASH_CHROME_NEW_WINDOW_CLIENT_H_

#include <memory>

#include "ash/public/interfaces/new_window.mojom.h"
#include "base/macros.h"
#include "mojo/public/cpp/bindings/associated_binding.h"
#include "url/gurl.h"

class ChromeNewWindowClient : public ash::mojom::NewWindowClient {
 public:
  ChromeNewWindowClient();
  ~ChromeNewWindowClient() override;

  static ChromeNewWindowClient* Get();

  // Overridden from ash::mojom::NewWindowClient:
  void NewTab() override;
  void NewTabWithUrl(const GURL& url) override;
  void NewWindow(bool incognito) override;
  void OpenFileManager() override;
  void OpenCrosh() override;
  void OpenGetHelp() override;
  void RestoreTab() override;
  void ShowKeyboardOverlay() override;
  void ShowKeyboardShortcutViewer() override;
  void ShowTaskManager() override;
  void OpenFeedbackPage() override;

 private:
  class TabRestoreHelper;

  std::unique_ptr<TabRestoreHelper> tab_restore_helper_;

  ash::mojom::NewWindowControllerPtr new_window_controller_;

  // Binds this object to the client interface.
  mojo::AssociatedBinding<ash::mojom::NewWindowClient> binding_;

  DISALLOW_COPY_AND_ASSIGN(ChromeNewWindowClient);
};

#endif  // CHROME_BROWSER_UI_ASH_CHROME_NEW_WINDOW_CLIENT_H_
