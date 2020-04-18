// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/login/help_app_launcher.h"

#include <string>

#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/chromeos/login/ui/login_web_dialog.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/grit/locale_settings.h"
#include "content/public/browser/browser_thread.h"
#include "extensions/browser/extension_registry.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "url/gurl.h"

using content::BrowserThread;
using extensions::ExtensionRegistry;

namespace {

const char kHelpAppFormat[] =
    "chrome-extension://honijodknafkokifofgiaalefdiedpko/oobe.html?id=%d";

}  // namespace

namespace chromeos {

///////////////////////////////////////////////////////////////////////////////
// HelpApp, public:

HelpAppLauncher::HelpAppLauncher(gfx::NativeWindow parent_window)
    : parent_window_(parent_window) {}

void HelpAppLauncher::ShowHelpTopic(HelpTopic help_topic_id) {
  Profile* profile = ProfileHelper::GetSigninProfile();
  ExtensionRegistry* registry = ExtensionRegistry::Get(profile);

  DCHECK(registry);
  if (!registry)
    return;

  GURL url(base::StringPrintf(kHelpAppFormat, static_cast<int>(help_topic_id)));
  // HelpApp component extension presents only in official builds so we can
  // show help only when the extensions is installed.
  if (registry->enabled_extensions().GetByID(url.host()))
    ShowHelpTopicDialog(profile, GURL(url));
}

///////////////////////////////////////////////////////////////////////////////
// HelpApp, protected:

HelpAppLauncher::~HelpAppLauncher() {}

///////////////////////////////////////////////////////////////////////////////
// HelpApp, private:

void HelpAppLauncher::ShowHelpTopicDialog(Profile* profile,
                                          const GURL& topic_url) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  LoginWebDialog* dialog = new LoginWebDialog(
      profile, NULL, parent_window_,
      l10n_util::GetStringUTF16(IDS_LOGIN_OOBE_HELP_DIALOG_TITLE), topic_url);
  dialog->SetDialogSize(l10n_util::GetLocalizedContentsWidthInPixels(
                            IDS_HELP_APP_DIALOG_WIDTH_PIXELS),
                        l10n_util::GetLocalizedContentsWidthInPixels(
                            IDS_HELP_APP_DIALOG_HEIGHT_PIXELS));
  dialog->Show();
  // The dialog object will be deleted on dialog close.
}

}  // namespace chromeos
