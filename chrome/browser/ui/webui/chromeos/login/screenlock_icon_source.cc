// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/chromeos/login/screenlock_icon_source.h"

#include "chrome/browser/chromeos/login/lock/screen_locker.h"
#include "chrome/browser/ui/webui/chromeos/login/screenlock_icon_provider.h"
#include "chrome/common/url_constants.h"
#include "net/base/escape.h"

namespace {

gfx::Image GetDefaultIcon() {
  return gfx::Image();
}

}  // namespace

namespace chromeos {

////////////////////////////////////////////////////////////////////////////////
// ScreenlockIconSource

ScreenlockIconSource::ScreenlockIconSource(
    base::WeakPtr<ScreenlockIconProvider> icon_provider)
    : icon_provider_(icon_provider) {
}

ScreenlockIconSource::~ScreenlockIconSource() {}

std::string ScreenlockIconSource::GetSource() const {
  return std::string(chrome::kChromeUIScreenlockIconHost);
}

void ScreenlockIconSource::StartDataRequest(
    const std::string& path,
    const content::ResourceRequestInfo::WebContentsGetter& wc_getter,
    const content::URLDataSource::GotDataCallback& callback) {
  if (!icon_provider_) {
    callback.Run(GetDefaultIcon().As1xPNGBytes().get());
    return;
  }

  GURL url(chrome::kChromeUIScreenlockIconURL + path);
  std::string username = net::UnescapeURLComponent(
      url.path().substr(1),
      net::UnescapeRule::URL_SPECIAL_CHARS_EXCEPT_PATH_SEPARATORS |
          net::UnescapeRule::PATH_SEPARATORS | net::UnescapeRule::SPACES);

  gfx::Image image = icon_provider_->GetIcon(username);
  if (image.IsEmpty()) {
    callback.Run(GetDefaultIcon().As1xPNGBytes().get());
    return;
  }

  callback.Run(image.As1xPNGBytes().get());
}

std::string ScreenlockIconSource::GetMimeType(const std::string&) const {
  return "image/png";
}

// static.
std::string ScreenlockIconSource::GetIconURLForUser(
    const std::string& username) {
  return std::string(chrome::kChromeUIScreenlockIconURL) +
      net::EscapePath(username);
}

}  // namespace chromeos
