// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/inline_install_private/inline_install_private_api.h"

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string_number_conversions.h"
#include "chrome/browser/extensions/webstore_install_with_prompt.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/extensions/api/inline_install_private.h"
#include "chrome/common/extensions/api/webstore/webstore_api_constants.h"
#include "components/crx_file/id_util.h"
#include "content/public/browser/browser_thread.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/view_type_utils.h"

namespace extensions {

namespace {

class Installer : public WebstoreInstallWithPrompt {
 public:
  Installer(const std::string& id,
            const GURL& requestor_url,
            Profile* profile,
            const Callback& callback);
 protected:
  friend class base::RefCountedThreadSafe<Installer>;
  ~Installer() override;

  // Needed so that we send the right referrer value in requests to the
  // webstore.
  const GURL& GetRequestorURL() const override { return requestor_url_; }

  std::unique_ptr<ExtensionInstallPrompt::Prompt> CreateInstallPrompt()
      const override;

  void OnManifestParsed() override;

  GURL requestor_url_;
};

Installer::Installer(const std::string& id,
                     const GURL& requestor_url,
                     Profile* profile,
                     const Callback& callback) :
    WebstoreInstallWithPrompt(id, profile, callback),
    requestor_url_(requestor_url) {
  set_show_post_install_ui(false);
}

Installer::~Installer() {
}

std::unique_ptr<ExtensionInstallPrompt::Prompt> Installer::CreateInstallPrompt()
    const {
  std::unique_ptr<ExtensionInstallPrompt::Prompt> prompt(
      new ExtensionInstallPrompt::Prompt(
          ExtensionInstallPrompt::INLINE_INSTALL_PROMPT));
  prompt->SetWebstoreData(localized_user_count(),
                          show_user_count(),
                          average_rating(),
                          rating_count());
  return prompt;
}


void Installer::OnManifestParsed() {
  if (manifest() == nullptr) {
    CompleteInstall(webstore_install::INVALID_MANIFEST, std::string());
    return;
  }

  Manifest parsed_manifest(Manifest::INTERNAL,
                           base::WrapUnique(manifest()->DeepCopy()));

  std::string manifest_error;
  std::vector<InstallWarning> warnings;

  if (!parsed_manifest.is_platform_app()) {
    CompleteInstall(webstore_install::NOT_PERMITTED, std::string());
    return;
  }

  ProceedWithInstallPrompt();
}


}  // namespace

InlineInstallPrivateInstallFunction::
    InlineInstallPrivateInstallFunction() {
}

InlineInstallPrivateInstallFunction::
    ~InlineInstallPrivateInstallFunction() {
}

ExtensionFunction::ResponseAction
InlineInstallPrivateInstallFunction::Run() {
  typedef api::inline_install_private::Install::Params Params;
  std::unique_ptr<Params> params(Params::Create(*args_));

  if (!user_gesture())
    return RespondNow(CreateResponse("Must be called with a user gesture",
                                     webstore_install::NOT_PERMITTED));

  content::WebContents* web_contents = GetSenderWebContents();
  if (!web_contents || GetViewType(web_contents) != VIEW_TYPE_APP_WINDOW) {
    return RespondNow(CreateResponse("Must be called from a foreground page",
                                     webstore_install::NOT_PERMITTED));
  }

  ExtensionRegistry* registry = ExtensionRegistry::Get(browser_context());
  if (registry->GetExtensionById(params->id, ExtensionRegistry::EVERYTHING))
    return RespondNow(CreateResponse("Already installed",
                                     webstore_install::OTHER_ERROR));

  scoped_refptr<Installer> installer = new Installer(
      params->id, source_url(), Profile::FromBrowserContext(browser_context()),
      base::Bind(&InlineInstallPrivateInstallFunction::InstallerCallback,
                 this));
  installer->BeginInstall();

  return RespondLater();
}

void InlineInstallPrivateInstallFunction::InstallerCallback(
    bool success,
    const std::string& error,
    webstore_install::Result result) {

  Respond(CreateResponse(success ? std::string() : error, result));
}

ExtensionFunction::ResponseValue
InlineInstallPrivateInstallFunction::CreateResponse(
    const std::string& error, webstore_install::Result result) {
  return ArgumentList(api::inline_install_private::Install::Results::Create(
      error,
      api::webstore::kInstallResultCodes[result]));
}

}  // namespace extensions
