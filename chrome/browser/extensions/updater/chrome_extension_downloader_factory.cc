// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/updater/chrome_extension_downloader_factory.h"

#include <string>
#include <utility>

#include "base/command_line.h"
#include "chrome/browser/extensions/updater/extension_updater_switches.h"
#include "chrome/browser/google/google_brand.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/signin/profile_oauth2_token_service_factory.h"
#include "chrome/browser/signin/signin_manager_factory.h"
#include "components/signin/core/browser/profile_oauth2_token_service.h"
#include "components/signin/core/browser/signin_manager.h"
#include "components/update_client/update_query_params.h"
#include "content/public/common/service_manager_connection.h"
#include "extensions/browser/updater/extension_downloader.h"

using extensions::ExtensionDownloader;
using extensions::ExtensionDownloaderDelegate;
using update_client::UpdateQueryParams;

std::unique_ptr<ExtensionDownloader>
ChromeExtensionDownloaderFactory::CreateForRequestContext(
    net::URLRequestContextGetter* request_context,
    ExtensionDownloaderDelegate* delegate,
    service_manager::Connector* connector) {
  std::unique_ptr<ExtensionDownloader> downloader(
      new ExtensionDownloader(delegate, request_context, connector));
#if defined(GOOGLE_CHROME_BUILD)
  std::string brand;
  google_brand::GetBrand(&brand);
  if (!brand.empty() && !google_brand::IsOrganic(brand))
    downloader->set_brand_code(brand);
#endif  // defined(GOOGLE_CHROME_BUILD)
  std::string manifest_query_params =
      UpdateQueryParams::Get(UpdateQueryParams::CRX);
  base::CommandLine* command_line =
      base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(extensions::kSwitchTestRequestParam)) {
    manifest_query_params += "&testrequest=1";
  }
  downloader->set_manifest_query_params(manifest_query_params);
  downloader->set_ping_enabled_domain("google.com");
  return downloader;
}

std::unique_ptr<ExtensionDownloader>
ChromeExtensionDownloaderFactory::CreateForProfile(
    Profile* profile,
    ExtensionDownloaderDelegate* delegate) {
  service_manager::Connector* connector =
      content::ServiceManagerConnection::GetForProcess()->GetConnector();
  std::unique_ptr<ExtensionDownloader> downloader =
      CreateForRequestContext(profile->GetRequestContext(), delegate,
        connector);

  // NOTE: It is not obvious why it is OK to pass raw pointers to the token
  // service and signin manager here. The logic is as follows:
  // ExtensionDownloader is owned by ExtensionUpdater.
  // ExtensionUpdater is owned by ExtensionService.
  // ExtensionService is owned by ExtensionSystemImpl::Shared.
  // ExtensionSystemImpl::Shared is a KeyedService. Its factory
  // (ExtensionSystemSharedFactory) specifies that it depends on SigninManager
  // and ProfileOAuth2TokenService.
  // Hence, the SigninManager and ProfileOAuth2TokenService instances are
  // guaranteed to outlive |downloader|.
  // TODO(843519): Make this lifetime relationship more explicit/cleaner.
  downloader->SetWebstoreAuthenticationCapabilities(
      base::BindRepeating(
          &SigninManagerBase::GetAuthenticatedAccountId,
          base::Unretained(SigninManagerFactory::GetForProfile(profile))),
      ProfileOAuth2TokenServiceFactory::GetForProfile(profile));
  return downloader;
}
