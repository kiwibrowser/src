// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SECURITY_INTERSTITIALS_CONTENT_SECURITY_INTERSTITIAL_PAGE_H_
#define COMPONENTS_SECURITY_INTERSTITIALS_CONTENT_SECURITY_INTERSTITIAL_PAGE_H_

#include <memory>

#include "base/macros.h"
#include "base/strings/string16.h"
#include "content/public/browser/interstitial_page_delegate.h"
#include "url/gurl.h"

namespace base {
class DictionaryValue;
}

namespace content {
class InterstitialPage;
class WebContents;
}

namespace security_interstitials {
class SecurityInterstitialControllerClient;

class SecurityInterstitialPage : public content::InterstitialPageDelegate {
 public:
  // |request_url| is the URL which triggered the interstitial page. For
  // SafeBrowsing interstitials, it can be a main frame or a subresource URL.
  // For SSL interstitials, it's always the main frame URL.
  SecurityInterstitialPage(
      content::WebContents* web_contents,
      const GURL& request_url,
      std::unique_ptr<SecurityInterstitialControllerClient> controller);
  ~SecurityInterstitialPage() override;

  // Creates an interstitial and shows it. This is used for the pre-committed
  // interstitials code path, when an interstitial is generated as an
  // overlay.
  virtual void Show();

  // Prevents creating the actual interstitial view for testing.
  void DontCreateViewForTesting();

  // InterstitialPageDelegate method:
  std::string GetHTMLContents() override;

  // Must be called when the interstitial is closed, to give subclasses a chance
  // to e.g. update metrics.
  virtual void OnInterstitialClosing() = 0;

 protected:
  // Returns true if the interstitial should create a new navigation entry.
  virtual bool ShouldCreateNewNavigation() const = 0;

  // Populates the strings used to generate the HTML from the template.
  virtual void PopulateInterstitialStrings(
      base::DictionaryValue* load_time_data) = 0;

  virtual int GetHTMLTemplateId();

  // Returns the formatted host name for the request url.
  base::string16 GetFormattedHostName() const;

  content::InterstitialPage* interstitial_page() const;
  content::WebContents* web_contents() const;
  GURL request_url() const;

  SecurityInterstitialControllerClient* controller() const;

  // Update metrics when the interstitial is closed.
  void UpdateMetricsAfterSecurityInterstitial();

 private:
  void SetUpMetrics();

  // The WebContents with which this interstitial page is
  // associated. Not available in ~SecurityInterstitialPage, since it
  // can be destroyed before this class is destroyed.
  content::WebContents* web_contents_;
  const GURL request_url_;
  // Once shown, |interstitial_page| takes ownership of this
  // SecurityInterstitialPage instance.
  content::InterstitialPage* interstitial_page_;
  // Whether the interstitial should create a view.
  bool create_view_;

  // Store some data about the initial state of extended reporting opt-in.
  bool on_show_extended_reporting_pref_exists_;
  bool on_show_extended_reporting_pref_value_;

  // For subclasses that don't have their own ControllerClients yet.
  std::unique_ptr<SecurityInterstitialControllerClient> controller_;

  DISALLOW_COPY_AND_ASSIGN(SecurityInterstitialPage);
};

}  // security_interstitials

#endif  // COMPONENTS_SECURITY_INTERSTITIALS_CONTENT_SECURITY_INTERSTITIAL_PAGE_H_
