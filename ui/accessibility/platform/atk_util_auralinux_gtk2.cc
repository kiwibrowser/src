// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <glib-2.0/gmodule.h>

#include "base/bind.h"
#include "base/environment.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/task_scheduler/post_task.h"
#include "ui/accessibility/platform/atk_util_auralinux.h"

typedef void (*GnomeAccessibilityModuleInitFunc)();

const char kAtkBridgeModule[] = "atk-bridge";
const char kAtkBridgePath[] = "gtk-2.0/modules/libatk-bridge.so";
const char kAtkBridgeSymbolName[] = "gnome_accessibility_module_init";
const char kGtkModules[] = "GTK_MODULES";

namespace ui {

// Returns a function pointer to be invoked on the main thread to init
// the gnome accessibility module if it's enabled (nullptr otherwise).
GnomeAccessibilityModuleInitFunc GetAccessibilityModuleInitFunc() {
  base::AssertBlockingAllowed();

  // Try to load libatk-bridge.so.
  base::FilePath atk_bridge_path(ATK_LIB_DIR);
  atk_bridge_path = atk_bridge_path.Append(kAtkBridgePath);
  GModule* bridge = g_module_open(atk_bridge_path.value().c_str(),
                                  static_cast<GModuleFlags>(0));
  if (!bridge) {
    VLOG(1) << "Unable to open module " << atk_bridge_path.value();
    return nullptr;
  }

  GnomeAccessibilityModuleInitFunc init_func = nullptr;

  if (!g_module_symbol(bridge, kAtkBridgeSymbolName, (gpointer*)&init_func)) {
    VLOG(1) << "Unable to get symbol pointer from " << atk_bridge_path.value();
    return nullptr;
  }

  DCHECK(init_func);
  return init_func;
}

void FinishAccessibilityInitOnMainThread(
    GnomeAccessibilityModuleInitFunc init_func) {
  if (!init_func) {
    VLOG(1) << "Will not enable ATK accessibility support.";
    return;
  }

  init_func();
}

bool AtkUtilAuraLinux::PlatformShouldEnableAccessibility() {
  std::unique_ptr<base::Environment> env(base::Environment::Create());
  std::string gtk_modules;
  if (!env->GetVar(kGtkModules, &gtk_modules))
    return false;

  for (const std::string& module :
       base::SplitString(gtk_modules, ":", base::TRIM_WHITESPACE,
                         base::SPLIT_WANT_NONEMPTY)) {
    if (module == kAtkBridgeModule)
      return true;
  }
  return false;
}

void AtkUtilAuraLinux::PlatformInitializeAsync() {
  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN},
      base::Bind(&GetAccessibilityModuleInitFunc),
      base::Bind(&FinishAccessibilityInitOnMainThread));
}

}  // namespace ui
