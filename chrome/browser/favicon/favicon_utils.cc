// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/favicon/favicon_utils.h"

#include "chrome/browser/favicon/favicon_service_factory.h"
#include "chrome/browser/history/history_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search/search.h"
#include "chrome/common/url_constants.h"
#include "components/favicon/content/content_favicon_driver.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/common/favicon_url.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/image/image_skia_operations.h"

namespace favicon {

namespace {

// Desaturate favicon HSL shift values.
const double kDesaturateHue = -1.0;
const double kDesaturateSaturation = 0.0;
const double kDesaturateLightness = 0.6;
}

void CreateContentFaviconDriverForWebContents(
    content::WebContents* web_contents) {
  DCHECK(web_contents);
  if (ContentFaviconDriver::FromWebContents(web_contents))
    return;

  Profile* original_profile =
      Profile::FromBrowserContext(web_contents->GetBrowserContext())
          ->GetOriginalProfile();
  return ContentFaviconDriver::CreateForWebContents(
      web_contents,
      FaviconServiceFactory::GetForProfile(original_profile,
                                           ServiceAccessType::IMPLICIT_ACCESS),
      HistoryServiceFactory::GetForProfile(original_profile,
                                           ServiceAccessType::IMPLICIT_ACCESS));
}

bool ShouldDisplayFavicon(content::WebContents* web_contents) {
  // No favicon on interstitials. This check must be done first since
  // interstitial navigations don't commit and always have a pending entry.
  if (web_contents->ShowingInterstitialPage())
    return false;

  // Always display a throbber during pending loads.
  const content::NavigationController& controller =
      web_contents->GetController();
  if (controller.GetLastCommittedEntry() && controller.GetPendingEntry())
    return true;

  GURL url = web_contents->GetURL();
  if (url.SchemeIs(content::kChromeUIScheme) &&
      url.host_piece() == chrome::kChromeUINewTabHost) {
    return false;
  }

  // No favicon on Instant New Tab Pages.
  if (search::IsInstantNTP(web_contents))
    return false;

  return true;
}

gfx::Image TabFaviconFromWebContents(content::WebContents* contents) {
  DCHECK(contents);

  favicon::FaviconDriver* favicon_driver =
      favicon::ContentFaviconDriver::FromWebContents(contents);
  gfx::Image favicon = favicon_driver->GetFavicon();

  // Desaturate the favicon if the navigation entry contains a network error.
  if (!contents->IsLoadingToDifferentDocument()) {
    const content::NavigationController& controller = contents->GetController();

    content::NavigationEntry* entry = controller.GetLastCommittedEntry();
    if (entry && (entry->GetPageType() == content::PAGE_TYPE_ERROR)) {
      color_utils::HSL shift = {kDesaturateHue, kDesaturateSaturation,
                                kDesaturateLightness};
      return gfx::Image(gfx::ImageSkiaOperations::CreateHSLShiftedImage(
          *favicon.ToImageSkia(), shift));
    }
  }

  return favicon;
}

}  // namespace favicon
