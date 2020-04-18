// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/accessibility/event_handler_common.h"

#include "chrome/browser/chromeos/accessibility/accessibility_manager.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host.h"
#include "extensions/browser/extension_host.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/process_manager.h"

namespace chromeos {

extensions::ExtensionHost* GetAccessibilityExtensionHost(
    const std::string& extension_id) {
  if (!AccessibilityManager::Get())
    return nullptr;

  content::BrowserContext* context = ProfileManager::GetActiveUserProfile();
  if (!context)
    return nullptr;

  const extensions::Extension* extension =
      extensions::ExtensionRegistry::Get(context)->enabled_extensions().GetByID(
          extension_id);
  if (!extension)
    return nullptr;

  extensions::ExtensionHost* host =
      extensions::ProcessManager::Get(context)->GetBackgroundHostForExtension(
          extension->id());
  return host;
}

void ForwardKeyToExtension(const ui::KeyEvent& key_event,
                           extensions::ExtensionHost* host) {
  if (!host) {
    LOG(ERROR) << "Unable to forward key to extension";
    return;
  }

  content::RenderViewHost* rvh = host->render_view_host();
  if (!rvh) {
    LOG(ERROR) << "Unable to forward key to extension";
    return;
  }

  const content::NativeWebKeyboardEvent web_event(key_event);
  // Don't forward latency info, as these are getting forwarded to an extension.
  rvh->GetWidget()->ForwardKeyboardEvent(web_event);
}
}  // namespace chromeos
