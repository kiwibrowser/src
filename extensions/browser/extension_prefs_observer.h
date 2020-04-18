// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_EXTENSION_PREFS_OBSERVER_H_
#define EXTENSIONS_BROWSER_EXTENSION_PREFS_OBSERVER_H_

#include <string>

#include "base/time/time.h"

namespace extensions {

class ExtensionPrefs;

class ExtensionPrefsObserver {
 public:
  // Called when the reasons for an extension being disabled have changed.
  // This is *not* called when the disable reasons change due to the extension
  // being enabled/disabled.
  virtual void OnExtensionDisableReasonsChanged(const std::string& extension_id,
                                                int disabled_reasons) {}

  // Called when an extension is registered with ExtensionPrefs.
  virtual void OnExtensionRegistered(const std::string& extension_id,
                                     const base::Time& install_time,
                                     bool is_enabled) {}

  // Called when an extension's prefs have been loaded.
  virtual void OnExtensionPrefsLoaded(const std::string& extension_id,
                                      const ExtensionPrefs* prefs) {}

  // Called when an extension's prefs are deleted.
  virtual void OnExtensionPrefsDeleted(const std::string& extension_id) {}

  // Called when an extension's enabled state pref is changed.
  // Note: This does not necessarily correspond to the extension being loaded/
  // unloaded. For that, observe the ExtensionRegistry, and reconcile that the
  // events might not match up.
  virtual void OnExtensionStateChanged(const std::string& extension_id,
                                       bool state) {}
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_EXTENSION_PREFS_OBSERVER_H_
