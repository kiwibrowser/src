// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/ui/preloaded_web_view.h"

#include "base/callback_helpers.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/chromeos/idle_detector.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "content/public/browser/web_contents.h"
#include "ui/views/controls/webview/webview.h"

namespace chromeos {

namespace {
// Duration of user inactivity before running the preload function.
constexpr int kIdleSecondsBeforePreloadingLockScreen = 8;
}  // namespace

PreloadedWebView::PreloadedWebView(Profile* profile)
    : profile_(profile), weak_factory_(this) {
  registrar_.Add(this, chrome::NOTIFICATION_APP_TERMINATING,
                 content::NotificationService::AllSources());
  memory_pressure_listener_ =
      std::make_unique<base::MemoryPressureListener>(base::Bind(
          &PreloadedWebView::OnMemoryPressure, weak_factory_.GetWeakPtr()));
}

PreloadedWebView::~PreloadedWebView() {}

void PreloadedWebView::PreloadOnIdle(PreloadCallback preload) {
  preload_function_ = std::move(preload);
  idle_detector_ = std::make_unique<chromeos::IdleDetector>(
      base::Bind(&PreloadedWebView::RunPreloader, weak_factory_.GetWeakPtr()));
  idle_detector_->Start(
      base::TimeDelta::FromSeconds(kIdleSecondsBeforePreloadingLockScreen));
}

std::unique_ptr<views::WebView> PreloadedWebView::TryTake() {
  idle_detector_.reset();

  // Clear cached reference if it is no longer valid (ie, destroyed in task
  // manager).
  if (preloaded_instance_ && !preloaded_instance_->GetWebContents()
                                  ->GetRenderViewHost()
                                  ->GetWidget()
                                  ->GetView()) {
    preloaded_instance_.reset();
  }

  return std::move(preloaded_instance_);
}

void PreloadedWebView::Shutdown() {
  preloaded_instance_.reset();
}

void PreloadedWebView::Observe(int type,
                               const content::NotificationSource& source,
                               const content::NotificationDetails& details) {
  DCHECK_EQ(chrome::NOTIFICATION_APP_TERMINATING, type);
  preloaded_instance_.reset();
}

void PreloadedWebView::OnMemoryPressure(
    base::MemoryPressureListener::MemoryPressureLevel level) {
  switch (level) {
    case base::MemoryPressureListener::MEMORY_PRESSURE_LEVEL_NONE:
      break;
    case base::MemoryPressureListener::MEMORY_PRESSURE_LEVEL_MODERATE:
    case base::MemoryPressureListener::MEMORY_PRESSURE_LEVEL_CRITICAL:
      preloaded_instance_.reset();
      break;
  }
}

void PreloadedWebView::RunPreloader() {
  idle_detector_.reset();
  preloaded_instance_ = std::move(preload_function_).Run(profile_);
}

}  // namespace chromeos
