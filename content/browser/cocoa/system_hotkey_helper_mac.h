// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_COCOA_SYSTEM_HOTKEY_HELPER_MAC_H_
#define CONTENT_BROWSER_COCOA_SYSTEM_HOTKEY_HELPER_MAC_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "base/memory/weak_ptr.h"

#ifdef __OBJC__
@class NSDictionary;
#else
class NSDictionary;
#endif

namespace content {

class SystemHotkeyMap;

// This singleton holds a global mapping of hotkeys reserved by OSX.
class SystemHotkeyHelperMac {
 public:
  // Return pointer to the singleton instance for the current process.
  static SystemHotkeyHelperMac* GetInstance();

  // Loads the system hot keys after a brief delay, to reduce file system access
  // immediately after launch.
  void DeferredLoadSystemHotkeys();

  // Guaranteed to not be NULL.
  SystemHotkeyMap* map() { return map_.get(); }

 private:
  friend struct base::DefaultSingletonTraits<SystemHotkeyHelperMac>;

  SystemHotkeyHelperMac();
  ~SystemHotkeyHelperMac();

  // Must be called from the FILE thread. Loads the file containing the system
  // hotkeys into a NSDictionary* object, and passes the result to FileDidLoad
  // on the UI thread.
  void LoadSystemHotkeys();

  // Must be called from the UI thread.  This takes ownership of |dictionary|.
  // Parses the system hotkeys from the plist stored in |dictionary|.
  void FileDidLoad(NSDictionary* dictionary);

  std::unique_ptr<SystemHotkeyMap> map_;

  DISALLOW_COPY_AND_ASSIGN(SystemHotkeyHelperMac);
};

}  // namespace content

#endif  // CONTENT_BROWSER_COCOA_SYSTEM_HOTKEY_HELPER_MAC_H_
