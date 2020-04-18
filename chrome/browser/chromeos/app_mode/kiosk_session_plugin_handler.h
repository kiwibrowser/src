// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_APP_MODE_KIOSK_SESSION_PLUGIN_HANDLER_H_
#define CHROME_BROWSER_CHROMEOS_APP_MODE_KIOSK_SESSION_PLUGIN_HANDLER_H_

#include <memory>
#include <set>
#include <vector>

#include "base/macros.h"

namespace base {
class FilePath;
}

namespace content {
class WebContents;
}

namespace chromeos {

class KioskSessionPluginHandlerDelegate;

// A class to watch for plugin crash/hung in a kiosk session. Device will be
// rebooted after the first crash/hung is detected.
class KioskSessionPluginHandler {
 public:
  explicit KioskSessionPluginHandler(
      KioskSessionPluginHandlerDelegate* delegate);
  ~KioskSessionPluginHandler();

  void Observe(content::WebContents* contents);

 private:
  // Observes WebContents for plugin crash/hung.
  class Observer;

  void OnPluginCrashed(const base::FilePath& plugin_path);
  void OnPluginHung(const std::set<int>& hung_plugins);
  void OnWebContentsDestroyed(Observer* observer);

  KioskSessionPluginHandlerDelegate* const delegate_;
  std::vector<std::unique_ptr<Observer>> watchers_;

  DISALLOW_COPY_AND_ASSIGN(KioskSessionPluginHandler);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_APP_MODE_KIOSK_SESSION_PLUGIN_HANDLER_H_
