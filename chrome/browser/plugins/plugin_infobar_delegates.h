// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PLUGINS_PLUGIN_INFOBAR_DELEGATES_H_
#define CHROME_BROWSER_PLUGINS_PLUGIN_INFOBAR_DELEGATES_H_

#include "base/callback.h"
#include "base/macros.h"
#include "build/build_config.h"
#include "chrome/browser/plugins/plugin_installer_observer.h"
#include "chrome/common/buildflags.h"
#include "components/infobars/core/confirm_infobar_delegate.h"
#include "url/gurl.h"


class InfoBarService;
class PluginInstaller;
class PluginMetadata;

// Infobar that's shown when a plugin is out of date.
class OutdatedPluginInfoBarDelegate : public ConfirmInfoBarDelegate,
                                      public WeakPluginInstallerObserver {
 public:
  // Creates an outdated plugin infobar and delegate and adds the infobar to
  // |infobar_service|.
  static void Create(InfoBarService* infobar_service,
                     PluginInstaller* installer,
                     std::unique_ptr<PluginMetadata> metadata);

  // Replaces |infobar|, which must currently be owned, with an infobar asking
  // the user to update a particular plugin.
  static void Replace(infobars::InfoBar* infobar,
                      PluginInstaller* installer,
                      std::unique_ptr<PluginMetadata> plugin_metadata,
                      const base::string16& message);

 private:
  OutdatedPluginInfoBarDelegate(PluginInstaller* installer,
                                std::unique_ptr<PluginMetadata> metadata,
                                const base::string16& message);
  ~OutdatedPluginInfoBarDelegate() override;

  // ConfirmInfoBarDelegate:
  infobars::InfoBarDelegate::InfoBarIdentifier GetIdentifier() const override;
  void InfoBarDismissed() override;
  const gfx::VectorIcon& GetVectorIcon() const override;
  base::string16 GetMessageText() const override;
  base::string16 GetButtonLabel(InfoBarButton button) const override;
  bool Accept() override;
  bool Cancel() override;
  base::string16 GetLinkText() const override;
  GURL GetLinkURL() const override;

  // PluginInstallerObserver:
  void DownloadFinished() override;

  // WeakPluginInstallerObserver:
  void OnlyWeakObserversLeft() override;

  // Replaces this infobar with one showing |message|. The new infobar will
  // not have any buttons (and not call the callback).
  void ReplaceWithInfoBar(const base::string16& message);

  std::string identifier_;

  std::unique_ptr<PluginMetadata> plugin_metadata_;

  base::string16 message_;

  DISALLOW_COPY_AND_ASSIGN(OutdatedPluginInfoBarDelegate);
};

#endif  // CHROME_BROWSER_PLUGINS_PLUGIN_INFOBAR_DELEGATES_H_
