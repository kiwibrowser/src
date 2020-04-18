// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_web_ui_override_registrar.h"

#include "base/lazy_instance.h"
#include "chrome/browser/extensions/extension_web_ui.h"
#include "chrome/browser/profiles/profile.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/common/one_shot_event.h"

namespace extensions {

ExtensionWebUIOverrideRegistrar::ExtensionWebUIOverrideRegistrar(
    content::BrowserContext* context)
    : extension_registry_observer_(this),
      weak_factory_(this) {
  ExtensionWebUI::InitializeChromeURLOverrides(
      Profile::FromBrowserContext(context));
  extension_registry_observer_.Add(ExtensionRegistry::Get(context));
  ExtensionSystem::Get(context)->ready().Post(
      FROM_HERE,
      base::Bind(&ExtensionWebUIOverrideRegistrar::OnExtensionSystemReady,
                 weak_factory_.GetWeakPtr(),
                 context));
}

ExtensionWebUIOverrideRegistrar::~ExtensionWebUIOverrideRegistrar() {
}

void ExtensionWebUIOverrideRegistrar::OnExtensionLoaded(
    content::BrowserContext* browser_context,
    const Extension* extension) {
  ExtensionWebUI::RegisterOrActivateChromeURLOverrides(
      Profile::FromBrowserContext(browser_context),
      URLOverrides::GetChromeURLOverrides(extension));
}

void ExtensionWebUIOverrideRegistrar::OnExtensionUnloaded(
    content::BrowserContext* browser_context,
    const Extension* extension,
    UnloadedExtensionReason reason) {
  ExtensionWebUI::DeactivateChromeURLOverrides(
      Profile::FromBrowserContext(browser_context),
      URLOverrides::GetChromeURLOverrides(extension));
}

void ExtensionWebUIOverrideRegistrar::OnExtensionUninstalled(
    content::BrowserContext* browser_context,
    const Extension* extension,
    UninstallReason reason) {
  ExtensionWebUI::UnregisterChromeURLOverrides(
      Profile::FromBrowserContext(browser_context),
      URLOverrides::GetChromeURLOverrides(extension));
}

void ExtensionWebUIOverrideRegistrar::OnExtensionSystemReady(
    content::BrowserContext* context) {
  ExtensionWebUI::ValidateChromeURLOverrides(
      Profile::FromBrowserContext(context));
}

static base::LazyInstance<BrowserContextKeyedAPIFactory<
    ExtensionWebUIOverrideRegistrar>>::DestructorAtExit
    g_extension_web_ui_override_registrar_factory = LAZY_INSTANCE_INITIALIZER;

// static
BrowserContextKeyedAPIFactory<ExtensionWebUIOverrideRegistrar>*
ExtensionWebUIOverrideRegistrar::GetFactoryInstance() {
  return g_extension_web_ui_override_registrar_factory.Pointer();
}

}  // namespace extensions
