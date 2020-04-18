// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/extension_reenabler.h"

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "chrome/browser/extensions/extension_service.h"
#include "chrome/browser/extensions/webstore_data_fetcher.h"
#include "chrome/browser/extensions/webstore_inline_installer.h"
#include "chrome/browser/net/system_network_context_manager.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "extensions/browser/extension_prefs.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_system.h"
#include "extensions/common/extension.h"

namespace extensions {

ExtensionReenabler::~ExtensionReenabler() {
  if (!finished_)
    Finish(ABORTED);
}

// static
std::unique_ptr<ExtensionReenabler> ExtensionReenabler::PromptForReenable(
    const scoped_refptr<const Extension>& extension,
    content::BrowserContext* browser_context,
    content::WebContents* web_contents,
    const GURL& referrer_url,
    const Callback& callback) {
#if DCHECK_IS_ON()
  // We should only try to reenable an extension that is, in fact, disabled.
  DCHECK(ExtensionRegistry::Get(browser_context)->disabled_extensions().
             Contains(extension->id()));
  // Currently, this should only be used for extensions that are disabled due
  // to a permissions increase.
  int disable_reasons =
      ExtensionPrefs::Get(browser_context)->GetDisableReasons(extension->id());
  DCHECK_NE(0, disable_reasons & disable_reason::DISABLE_PERMISSIONS_INCREASE);
#endif  // DCHECK_IS_ON()

  return base::WrapUnique(new ExtensionReenabler(
      extension, browser_context, referrer_url, callback, web_contents,
      ExtensionInstallPrompt::GetDefaultShowDialogCallback()));
}

// static
std::unique_ptr<ExtensionReenabler>
ExtensionReenabler::PromptForReenableWithCallbackForTest(
    const scoped_refptr<const Extension>& extension,
    content::BrowserContext* browser_context,
    const Callback& callback,
    const ExtensionInstallPrompt::ShowDialogCallback& show_dialog_callback) {
  return base::WrapUnique(new ExtensionReenabler(extension, browser_context,
                                                 GURL(), callback, nullptr,
                                                 show_dialog_callback));
}

ExtensionReenabler::ExtensionReenabler(
    const scoped_refptr<const Extension>& extension,
    content::BrowserContext* browser_context,
    const GURL& referrer_url,
    const Callback& callback,
    content::WebContents* web_contents,
    const ExtensionInstallPrompt::ShowDialogCallback& show_dialog_callback)
    : extension_(extension),
      browser_context_(browser_context),
      referrer_url_(referrer_url),
      callback_(callback),
      show_dialog_callback_(show_dialog_callback),
      finished_(false),
      registry_observer_(this),
      weak_factory_(this) {
  DCHECK(extension_.get());
  registry_observer_.Add(ExtensionRegistry::Get(browser_context_));

  install_prompt_.reset(new ExtensionInstallPrompt(web_contents));

  // If we have a non-empty referrer, then we have to validate that it's a valid
  // url for the extension.
  if (!referrer_url_.is_empty()) {
    webstore_data_fetcher_.reset(
        new WebstoreDataFetcher(this, referrer_url_, extension->id()));
    webstore_data_fetcher_->Start(
        content::BrowserContext::GetDefaultStoragePartition(browser_context_)
            ->GetURLLoaderFactoryForBrowserProcess()
            .get());
  } else {
    ExtensionInstallPrompt::PromptType type =
        ExtensionInstallPrompt::GetReEnablePromptTypeForExtension(
            browser_context, extension.get());
    install_prompt_->ShowDialog(
        base::Bind(&ExtensionReenabler::OnInstallPromptDone,
                   weak_factory_.GetWeakPtr()),
        extension.get(), nullptr,
        std::make_unique<ExtensionInstallPrompt::Prompt>(type),
        show_dialog_callback_);
  }
}

void ExtensionReenabler::OnInstallPromptDone(
    ExtensionInstallPrompt::Result install_result) {
  ReenableResult result = ABORTED;
  switch (install_result) {
    case ExtensionInstallPrompt::Result::ACCEPTED: {
      // Stop observing - we don't want to see our own enablement.
      registry_observer_.RemoveAll();

      ExtensionService* extension_service =
          ExtensionSystem::Get(browser_context_)->extension_service();
      if (extension_service->browser_terminating()) {
        result = ABORTED;
      } else {
        extension_service->GrantPermissionsAndEnableExtension(extension_.get());
        // The re-enable could have failed if the extension is disallowed by
        // policy.
        bool enabled = ExtensionRegistry::Get(browser_context_)
                           ->enabled_extensions()
                           .GetByID(extension_->id()) != nullptr;
        result = enabled ? REENABLE_SUCCESS : NOT_ALLOWED;
      }
      break;
    }
    case ExtensionInstallPrompt::Result::USER_CANCELED:
      result = USER_CANCELED;
      break;
    case ExtensionInstallPrompt::Result::ABORTED:
      result = ABORTED;
      break;
  }

  Finish(result);
}

void ExtensionReenabler::OnExtensionLoaded(
    content::BrowserContext* browser_context,
    const Extension* extension) {
  // If the user chose to manually re-enable the extension then, for all
  // intents and purposes, this was a success.
  if (extension == extension_.get())
    Finish(REENABLE_SUCCESS);
}

void ExtensionReenabler::OnExtensionUninstalled(
    content::BrowserContext* browser_context,
    const Extension* extension,
    UninstallReason reason) {
  if (extension == extension_.get())
    Finish(USER_CANCELED);
}

void ExtensionReenabler::OnWebstoreRequestFailure() {
  Finish(ABORTED);
}

void ExtensionReenabler::OnWebstoreResponseParseSuccess(
    std::unique_ptr<base::DictionaryValue> webstore_data) {
  DCHECK(!referrer_url_.is_empty());
  std::string error;
  if (!WebstoreInlineInstaller::IsRequestorPermitted(*webstore_data,
                                                     referrer_url_,
                                                     &error)) {
    Finish(NOT_ALLOWED);
  } else {
    ExtensionInstallPrompt::PromptType type =
        ExtensionInstallPrompt::GetReEnablePromptTypeForExtension(
            browser_context_, extension_.get());
    install_prompt_->ShowDialog(
        base::Bind(&ExtensionReenabler::OnInstallPromptDone,
                   weak_factory_.GetWeakPtr()),
        extension_.get(), nullptr,
        std::make_unique<ExtensionInstallPrompt::Prompt>(type),
        show_dialog_callback_);
  }
}

void ExtensionReenabler::OnWebstoreResponseParseFailure(
    const std::string& error) {
  Finish(ABORTED);
}

void ExtensionReenabler::Finish(ReenableResult result) {
  DCHECK(!finished_);
  finished_ = true;
  registry_observer_.RemoveAll();
  callback_.Run(result);
}

}  // namespace extensions
