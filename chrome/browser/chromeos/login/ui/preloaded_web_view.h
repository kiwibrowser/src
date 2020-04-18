// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LOGIN_UI_PRELOADED_WEB_VIEW_H_
#define CHROME_BROWSER_CHROMEOS_LOGIN_UI_PRELOADED_WEB_VIEW_H_

#include <memory>

#include "base/callback_forward.h"
#include "base/macros.h"
#include "base/memory/memory_pressure_listener.h"
#include "base/memory/weak_ptr.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/notification_observer.h"
#include "content/public/browser/notification_registrar.h"

class Profile;

namespace views {
class WebView;
}

namespace chromeos {

class IdleDetector;

// Stores and fetches a views::WebView instance that is ulimately owned by the
// signin profile. This allows for a WebView to be reused over time or
// preloaded. Use PreloadedWebViewFactory to get an instance of this class.
class PreloadedWebView : public KeyedService,
                         public content::NotificationObserver {
 public:
  using PreloadCallback =
      base::OnceCallback<std::unique_ptr<views::WebView>(Profile*)>;

  explicit PreloadedWebView(Profile* profile);
  ~PreloadedWebView() override;

  // Executes the given |preload| function when the device is idle.
  void PreloadOnIdle(PreloadCallback preload);

  // Try to fetch a preloaded instance. Returns nullptr if no instance has been
  // preloaded. Calling this function will cancel any pending preload.
  std::unique_ptr<views::WebView> TryTake();

 private:
  // KeyedSerivce:
  void Shutdown() override;

  // content::NotificationObserver:
  void Observe(int type,
               const content::NotificationSource& source,
               const content::NotificationDetails& details) override;

  // Called when there is a memory pressure event.
  void OnMemoryPressure(
      base::MemoryPressureListener::MemoryPressureLevel level);

  // Runs the preload function. Called only by |idle_detector_|.
  void RunPreloader();

  // Used to execute the preload function when the user is idle.
  std::unique_ptr<IdleDetector> idle_detector_;
  // The preload function. Can only be called once.
  PreloadCallback preload_function_;
  // The result of the preload function.
  std::unique_ptr<views::WebView> preloaded_instance_;
  // Profile passed into the preload function.
  Profile* profile_;

  // Used to destroy a preloaded but not used WebView on shutdown.
  content::NotificationRegistrar registrar_;
  // Used to destroy a preloaded but not used WebView on low-memory.
  std::unique_ptr<base::MemoryPressureListener> memory_pressure_listener_;

  base::WeakPtrFactory<PreloadedWebView> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(PreloadedWebView);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LOGIN_UI_PRELOADED_WEB_VIEW_H_
