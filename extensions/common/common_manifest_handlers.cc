// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/common/common_manifest_handlers.h"

#include "components/nacl/common/buildflags.h"
#include "extensions/common/api/bluetooth/bluetooth_manifest_handler.h"
#include "extensions/common/api/declarative/declarative_manifest_handler.h"
#include "extensions/common/api/declarative_net_request/dnr_manifest_handler.h"
#include "extensions/common/api/printer_provider/usb_printer_manifest_handler.h"
#include "extensions/common/api/sockets/sockets_manifest_handler.h"
#include "extensions/common/manifest_handler.h"
#include "extensions/common/manifest_handlers/background_info.h"
#include "extensions/common/manifest_handlers/content_capabilities_handler.h"
#include "extensions/common/manifest_handlers/csp_info.h"
#include "extensions/common/manifest_handlers/default_locale_handler.h"
#include "extensions/common/manifest_handlers/externally_connectable.h"
#include "extensions/common/manifest_handlers/file_handler_info.h"
#include "extensions/common/manifest_handlers/icons_handler.h"
#include "extensions/common/manifest_handlers/incognito_info.h"
#include "extensions/common/manifest_handlers/kiosk_mode_info.h"
#include "extensions/common/manifest_handlers/mime_types_handler.h"
#include "extensions/common/manifest_handlers/nacl_modules_handler.h"
#include "extensions/common/manifest_handlers/oauth2_manifest_handler.h"
#include "extensions/common/manifest_handlers/offline_enabled_info.h"
#include "extensions/common/manifest_handlers/requirements_info.h"
#include "extensions/common/manifest_handlers/sandboxed_page_info.h"
#include "extensions/common/manifest_handlers/shared_module_info.h"
#include "extensions/common/manifest_handlers/web_accessible_resources_info.h"
#include "extensions/common/manifest_handlers/webview_info.h"
#include "extensions/common/manifest_url_handlers.h"

#if defined(OS_CHROMEOS)
#include "extensions/common/manifest_handlers/action_handlers_handler.h"
#endif

namespace extensions {

void RegisterCommonManifestHandlers() {
  DCHECK(!ManifestHandler::IsRegistrationFinalized());
#if defined(OS_CHROMEOS)
  (new ActionHandlersHandler)->Register();
#endif
  (new BackgroundManifestHandler)->Register();
  (new BluetoothManifestHandler)->Register();
  (new ContentCapabilitiesHandler)->Register();
  (new CSPHandler(false))->Register();
  (new CSPHandler(true))->Register();
  (new declarative_net_request::DNRManifestHandler)->Register();
  (new DeclarativeManifestHandler)->Register();
  (new DefaultLocaleHandler)->Register();
  (new ExternallyConnectableHandler)->Register();
  (new FileHandlersParser)->Register();
  (new IconsHandler)->Register();
  (new IncognitoHandler)->Register();
  (new KioskModeHandler)->Register();
  (new MimeTypesHandlerParser)->Register();
#if BUILDFLAG(ENABLE_NACL)
  (new NaClModulesHandler)->Register();
#endif
  (new OAuth2ManifestHandler)->Register();
  (new OfflineEnabledHandler)->Register();
  (new RequirementsHandler)->Register();
  (new SandboxedPageHandler)->Register();
  (new SharedModuleHandler)->Register();
  (new SocketsManifestHandler)->Register();
  (new UpdateURLHandler)->Register();
  (new UsbPrinterManifestHandler)->Register();
  (new WebAccessibleResourcesHandler)->Register();
  (new WebviewHandler)->Register();
}

}  // namespace extensions
