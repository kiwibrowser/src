// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/common/extensions/chrome_manifest_handlers.h"

#include "build/build_config.h"
#include "chrome/common/extensions/api/commands/commands_handler.h"
#include "chrome/common/extensions/api/omnibox/omnibox_handler.h"
#include "chrome/common/extensions/api/speech/tts_engine_manifest_handler.h"
#include "chrome/common/extensions/api/spellcheck/spellcheck_handler.h"
#include "chrome/common/extensions/api/storage/storage_schema_manifest_handler.h"
#include "chrome/common/extensions/api/system_indicator/system_indicator_handler.h"
#include "chrome/common/extensions/api/url_handlers/url_handlers_parser.h"
#include "chrome/common/extensions/chrome_manifest_url_handlers.h"
#include "chrome/common/extensions/manifest_handlers/app_icon_color_info.h"
#include "chrome/common/extensions/manifest_handlers/app_launch_info.h"
#include "chrome/common/extensions/manifest_handlers/app_theme_color_info.h"
#include "chrome/common/extensions/manifest_handlers/extension_action_handler.h"
#include "chrome/common/extensions/manifest_handlers/linked_app_icons.h"
#include "chrome/common/extensions/manifest_handlers/minimum_chrome_version_checker.h"
#include "chrome/common/extensions/manifest_handlers/settings_overrides_handler.h"
#include "chrome/common/extensions/manifest_handlers/theme_handler.h"
#include "chrome/common/extensions/manifest_handlers/ui_overrides_handler.h"
#include "extensions/common/manifest_handlers/app_isolation_info.h"
#include "extensions/common/manifest_handlers/automation.h"
#include "extensions/common/manifest_handlers/content_scripts_handler.h"
#include "extensions/common/manifest_handlers/options_page_info.h"
#include "extensions/common/manifest_url_handlers.h"

#if defined(OS_CHROMEOS)
#include "chrome/common/extensions/api/file_browser_handlers/file_browser_handler.h"
#include "chrome/common/extensions/api/file_system_provider_capabilities/file_system_provider_capabilities_handler.h"
#include "chrome/common/extensions/api/input_ime/input_components_handler.h"
#endif

namespace extensions {

void RegisterChromeManifestHandlers() {
  DCHECK(!ManifestHandler::IsRegistrationFinalized());
  (new AboutPageHandler)->Register();
  (new AppIconColorHandler)->Register();
  (new AppThemeColorHandler)->Register();
  (new AppIsolationHandler)->Register();
  (new AppLaunchManifestHandler)->Register();
  (new AutomationHandler)->Register();
  (new CommandsHandler)->Register();
  (new ContentScriptsHandler)->Register();
  (new DevToolsPageHandler)->Register();
  (new ExtensionActionHandler)->Register();
  (new HomepageURLHandler)->Register();
  (new LinkedAppIconsHandler)->Register();
  (new MinimumChromeVersionChecker)->Register();
  (new OmniboxHandler)->Register();
  (new OptionsPageManifestHandler)->Register();
  (new SettingsOverridesHandler)->Register();
  (new SpellcheckHandler)->Register();
  (new StorageSchemaManifestHandler)->Register();
  (new SystemIndicatorHandler)->Register();
  (new ThemeHandler)->Register();
  (new TtsEngineManifestHandler)->Register();
  (new UIOverridesHandler)->Register();
  (new UrlHandlersParser)->Register();
  (new URLOverridesHandler)->Register();
#if defined(OS_CHROMEOS)
  (new FileBrowserHandlerParser)->Register();
  (new FileSystemProviderCapabilitiesHandler)->Register();
  (new InputComponentsHandler)->Register();
#endif
}

}  // namespace extensions
