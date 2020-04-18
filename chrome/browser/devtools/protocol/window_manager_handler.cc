// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/devtools/protocol/window_manager_handler.h"

#include "ash/shell.h"
#include "ash/wm/overview/window_selector_controller.h"
#include "content/public/browser/browser_thread.h"

WindowManagerHandler::WindowManagerHandler(
    protocol::UberDispatcher* dispatcher) {
  protocol::WindowManager::Dispatcher::wire(dispatcher, this);
}

WindowManagerHandler::~WindowManagerHandler() = default;

protocol::Response WindowManagerHandler::EnterOverviewMode() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  bool toggled =
      ash::Shell::Get()->window_selector_controller()->ToggleOverview();
  return toggled ? protocol::Response::OK()
                 : protocol::Response::Error("Overview failed");
}

protocol::Response WindowManagerHandler::ExitOverviewMode() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  bool toggled =
      ash::Shell::Get()->window_selector_controller()->ToggleOverview();
  return toggled ? protocol::Response::OK()
                 : protocol::Response::Error("Overview failed");
}
