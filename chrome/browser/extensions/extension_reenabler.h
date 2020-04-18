// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_EXTENSION_REENABLER_H_
#define CHROME_BROWSER_EXTENSIONS_EXTENSION_REENABLER_H_

#include <memory>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observer.h"
#include "chrome/browser/extensions/extension_install_prompt.h"
#include "chrome/browser/extensions/webstore_data_fetcher_delegate.h"
#include "extensions/browser/extension_registry_observer.h"

namespace content {
class BrowserContext;
}

namespace extensions {

class Extension;
class ExtensionRegistry;
class WebstoreDataFetcher;

// A class to handle reenabling an extension disabled due to a permissions
// increase.
// TODO(devlin): Once we get the UI figured out, we should also have this handle
// other disable reasons.
class ExtensionReenabler : public ExtensionRegistryObserver,
                           public WebstoreDataFetcherDelegate {
 public:
  enum ReenableResult {
    REENABLE_SUCCESS,  // The extension has been successfully re-enabled.
    USER_CANCELED,     // The user chose to not re-enable the extension.
    NOT_ALLOWED,       // The re-enable is not allowed.
    ABORTED,           // The re-enable process was aborted due to, e.g.,
                       // shutdown or a bad webstore response.
  };

  using Callback = base::Callback<void(ReenableResult)>;

  ~ExtensionReenabler() override;

  // Prompts the user to reenable the given |extension|, and calls |callback|
  // upon completion.
  // If |referrer_url| is non-empty, then this will also check to make sure
  // that the referrer_url is listed as a trusted url by the extension.
  static std::unique_ptr<ExtensionReenabler> PromptForReenable(
      const scoped_refptr<const Extension>& extension,
      content::BrowserContext* browser_context,
      content::WebContents* web_contents,
      const GURL& referrer_url,
      const Callback& callback);

  // Like PromptForReenable, but allows tests to inject the
  // ExtensionInstallPrompt.
  static std::unique_ptr<ExtensionReenabler>
  PromptForReenableWithCallbackForTest(
      const scoped_refptr<const Extension>& extension,
      content::BrowserContext* browser_context,
      const Callback& callback,
      const ExtensionInstallPrompt::ShowDialogCallback& show_callback);

 private:
  ExtensionReenabler(
      const scoped_refptr<const Extension>& extension,
      content::BrowserContext* browser_context,
      const GURL& referrer_url,
      const Callback& callback,
      content::WebContents* web_contents,
      const ExtensionInstallPrompt::ShowDialogCallback& show_callback);

  void OnInstallPromptDone(ExtensionInstallPrompt::Result result);

  // ExtensionRegistryObserver:
  void OnExtensionLoaded(content::BrowserContext* browser_context,
                         const Extension* extension) override;
  void OnExtensionUninstalled(content::BrowserContext* browser_context,
                              const Extension* extension,
                              UninstallReason reason) override;

  // WebstoreDataFetcherDelegate:
  void OnWebstoreRequestFailure() override;
  void OnWebstoreResponseParseSuccess(
      std::unique_ptr<base::DictionaryValue> webstore_data) override;
  void OnWebstoreResponseParseFailure(const std::string& error) override;

  // Sets the |finished_| bit and runs |callback_| with the given |result|.
  void Finish(ReenableResult result);

  // The extension to be re-enabled.
  scoped_refptr<const Extension> extension_;

  // The associated browser context.
  content::BrowserContext* browser_context_;

  // The url of the referrer, if any. If this is non-empty, it means we have
  // to check that the url is trusted by the extension.
  GURL referrer_url_;

  // The callback to run upon completion.
  Callback callback_;

  // The callback to use to show the dialog.
  ExtensionInstallPrompt::ShowDialogCallback show_dialog_callback_;

  // The re-enable prompt.
  std::unique_ptr<ExtensionInstallPrompt> install_prompt_;

  // Indicates whether the re-enable process finished.
  bool finished_;

  // The data fetcher for retrieving webstore data.
  std::unique_ptr<WebstoreDataFetcher> webstore_data_fetcher_;

  ScopedObserver<ExtensionRegistry, ExtensionRegistryObserver>
      registry_observer_;

  base::WeakPtrFactory<ExtensionReenabler> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ExtensionReenabler);
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_EXTENSION_REENABLER_H_
