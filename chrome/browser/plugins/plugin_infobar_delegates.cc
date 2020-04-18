// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/plugins/plugin_infobar_delegates.h"

#include <utility>

#include "base/bind.h"
#include "base/metrics/user_metrics.h"
#include "base/path_service.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "chrome/browser/lifetime/application_lifetime.h"
#include "chrome/browser/plugins/chrome_plugin_service_filter.h"
#include "chrome/browser/plugins/plugin_installer.h"
#include "chrome/browser/plugins/plugin_metadata.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/shell_integration.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/common/buildflags.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/generated_resources.h"
#include "components/google/core/browser/google_util.h"
#include "components/infobars/core/infobar.h"
#include "components/strings/grit/components_strings.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/l10n/l10n_util.h"

#if defined(OS_WIN)
#include <shellapi.h>
#include "ui/base/win/shell.h"
#endif

using base::UserMetricsAction;

// OutdatedPluginInfoBarDelegate ----------------------------------------------

void OutdatedPluginInfoBarDelegate::Create(
    InfoBarService* infobar_service,
    PluginInstaller* installer,
    std::unique_ptr<PluginMetadata> plugin_metadata) {
  // Copy the name out of |plugin_metadata| now, since the Pass() call below
  // will make it impossible to get at.
  base::string16 name(plugin_metadata->name());
  infobar_service->AddInfoBar(infobar_service->CreateConfirmInfoBar(
      std::unique_ptr<ConfirmInfoBarDelegate>(new OutdatedPluginInfoBarDelegate(
          installer, std::move(plugin_metadata),
          l10n_util::GetStringFUTF16(IDS_PLUGIN_OUTDATED_PROMPT, name)))));
}

OutdatedPluginInfoBarDelegate::OutdatedPluginInfoBarDelegate(
    PluginInstaller* installer,
    std::unique_ptr<PluginMetadata> plugin_metadata,
    const base::string16& message)
    : ConfirmInfoBarDelegate(),
      WeakPluginInstallerObserver(installer),
      identifier_(plugin_metadata->identifier()),
      plugin_metadata_(std::move(plugin_metadata)),
      message_(message) {
  base::RecordAction(UserMetricsAction("OutdatedPluginInfobar.Shown"));
  std::string name = base::UTF16ToUTF8(plugin_metadata_->name());
  if (name == PluginMetadata::kJavaGroupName) {
    base::RecordAction(UserMetricsAction("OutdatedPluginInfobar.Shown.Java"));
  } else if (name == PluginMetadata::kQuickTimeGroupName) {
    base::RecordAction(
        UserMetricsAction("OutdatedPluginInfobar.Shown.QuickTime"));
  } else if (name == PluginMetadata::kShockwaveGroupName) {
    base::RecordAction(
        UserMetricsAction("OutdatedPluginInfobar.Shown.Shockwave"));
  } else if (name == PluginMetadata::kRealPlayerGroupName) {
    base::RecordAction(
        UserMetricsAction("OutdatedPluginInfobar.Shown.RealPlayer"));
  } else if (name == PluginMetadata::kSilverlightGroupName) {
    base::RecordAction(
        UserMetricsAction("OutdatedPluginInfobar.Shown.Silverlight"));
  } else if (name == PluginMetadata::kAdobeReaderGroupName) {
    base::RecordAction(UserMetricsAction("OutdatedPluginInfobar.Shown.Reader"));
  }
}

OutdatedPluginInfoBarDelegate::~OutdatedPluginInfoBarDelegate() {
  base::RecordAction(UserMetricsAction("OutdatedPluginInfobar.Closed"));
}

infobars::InfoBarDelegate::InfoBarIdentifier
OutdatedPluginInfoBarDelegate::GetIdentifier() const {
  return OUTDATED_PLUGIN_INFOBAR_DELEGATE;
}

void OutdatedPluginInfoBarDelegate::InfoBarDismissed() {
  base::RecordAction(UserMetricsAction("OutdatedPluginInfobar.Dismissed"));
}

const gfx::VectorIcon& OutdatedPluginInfoBarDelegate::GetVectorIcon() const {
  return kExtensionIcon;
}

base::string16 OutdatedPluginInfoBarDelegate::GetMessageText() const {
  return message_;
}

base::string16 OutdatedPluginInfoBarDelegate::GetButtonLabel(
    InfoBarButton button) const {
  return l10n_util::GetStringUTF16((button == BUTTON_OK) ?
      IDS_PLUGIN_UPDATE : IDS_PLUGIN_ENABLE_TEMPORARILY);
}

bool OutdatedPluginInfoBarDelegate::Accept() {
  base::RecordAction(UserMetricsAction("OutdatedPluginInfobar.Update"));
  // A call to any of |OpenDownloadURL()| or |StartInstalling()| will
  // result in deleting ourselves. Accordingly, we make sure to
  // not pass a reference to an object that can go away.
  GURL plugin_url(plugin_metadata_->plugin_url());
  content::WebContents* web_contents =
      InfoBarService::WebContentsFromInfoBar(infobar());
  if (web_contents) {
    DCHECK(plugin_metadata_->url_for_display());
    installer()->OpenDownloadURL(plugin_url, web_contents);
  }
  return false;
}

bool OutdatedPluginInfoBarDelegate::Cancel() {
  base::RecordAction(UserMetricsAction("OutdatedPluginInfobar.AllowThisTime"));

  content::WebContents* web_contents =
      InfoBarService::WebContentsFromInfoBar(infobar());
  if (web_contents) {
    ChromePluginServiceFilter::GetInstance()->AuthorizeAllPlugins(
        web_contents, true, identifier_);
  }

  return true;
}

base::string16 OutdatedPluginInfoBarDelegate::GetLinkText() const {
  return l10n_util::GetStringUTF16(IDS_LEARN_MORE);
}

GURL OutdatedPluginInfoBarDelegate::GetLinkURL() const {
  return GURL(chrome::kOutdatedPluginLearnMoreURL);
}

void OutdatedPluginInfoBarDelegate::DownloadFinished() {
  ReplaceWithInfoBar(l10n_util::GetStringFUTF16(IDS_PLUGIN_UPDATING,
                                                plugin_metadata_->name()));
}

void OutdatedPluginInfoBarDelegate::OnlyWeakObserversLeft() {
  infobar()->RemoveSelf();
}

void OutdatedPluginInfoBarDelegate::ReplaceWithInfoBar(
    const base::string16& message) {
  // Return early if the message doesn't change. This is important in case the
  // PluginInstaller is still iterating over its observers (otherwise we would
  // keep replacing infobar delegates infinitely).
  if ((message_ == message) || !infobar()->owner())
    return;
  Replace(infobar(), installer(), plugin_metadata_->Clone(), message);
}

// static
void OutdatedPluginInfoBarDelegate::Replace(
    infobars::InfoBar* infobar,
    PluginInstaller* installer,
    std::unique_ptr<PluginMetadata> plugin_metadata,
    const base::string16& message) {
  DCHECK(infobar->owner());
  infobar->owner()->ReplaceInfoBar(
      infobar, infobar->owner()->CreateConfirmInfoBar(
                   std::unique_ptr<ConfirmInfoBarDelegate>(
                       new OutdatedPluginInfoBarDelegate(
                           installer, std::move(plugin_metadata), message))));
}

