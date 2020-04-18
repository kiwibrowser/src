// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PREVIEWS_PREVIEWS_INFOBAR_DELEGATE_H_
#define CHROME_BROWSER_PREVIEWS_PREVIEWS_INFOBAR_DELEGATE_H_

#include "base/callback.h"
#include "base/strings/string16.h"
#include "base/time/time.h"
#include "components/infobars/core/confirm_infobar_delegate.h"
#include "components/previews/core/previews_experiments.h"

class PreviewsInfoBarTabHelper;

namespace content {
class WebContents;
}

namespace previews {
class PreviewsUIService;
}

// Shows an infobar that lets the user know that a preview page has been loaded,
// and gives the user a link to reload the original page. This infobar will only
// be shown once per page load. Records UMA data for user interactions with the
// infobar.
class PreviewsInfoBarDelegate : public ConfirmInfoBarDelegate {
 public:
  typedef base::OnceCallback<void(bool opt_out)>
      OnDismissPreviewsInfobarCallback;

  // Actions on the previews infobar. This enum must remain synchronized with
  // the enum of the same name in metrics/histograms/histograms.xml.
  enum PreviewsInfoBarAction {
    INFOBAR_SHOWN = 0,
    INFOBAR_LOAD_ORIGINAL_CLICKED = 1,
    INFOBAR_DISMISSED_BY_USER = 2,
    INFOBAR_DISMISSED_BY_NAVIGATION = 3,
    INFOBAR_DISMISSED_BY_RELOAD = 4,
    INFOBAR_DISMISSED_BY_TAB_CLOSURE = 5,
    INFOBAR_INDEX_BOUNDARY
  };

  // Values of the UMA Previews.InfoBarTimestamp histogram. This enum must
  // remain synchronized with the enum of the same name in
  // metrics/histograms/histograms.xml.
  enum PreviewsInfoBarTimestamp {
    TIMESTAMP_SHOWN = 0,
    TIMESTAMP_NOT_SHOWN_PREVIEW_NOT_STALE = 1,
    TIMESTAMP_NOT_SHOWN_STALENESS_NEGATIVE = 2,
    TIMESTAMP_NOT_SHOWN_STALENESS_GREATER_THAN_MAX = 3,
    TIMESTAMP_UPDATED_NOW_SHOWN = 4,
    TIMESTAMP_INDEX_BOUNDARY
  };

  ~PreviewsInfoBarDelegate() override;

  // Creates a preview infobar and corresponding delegate and adds the infobar
  // to InfoBarService. |on_dismiss_callback| is called when the InfoBar is
  // dismissed.
  static void Create(
      content::WebContents* web_contents,
      previews::PreviewsType previews_type,
      base::Time previews_freshness,
      bool is_data_saver_user,
      bool is_reload,
      // TODO(ryansturm): Replace |on_dismiss_callback| with direct call to
      // |previews_ui_service|.
      OnDismissPreviewsInfobarCallback on_dismiss_callback,
      previews::PreviewsUIService* previews_ui_service);

  // ConfirmInfoBarDelegate overrides:
  int GetIconId() const override;
  base::string16 GetMessageText() const override;
  base::string16 GetLinkText() const override;

  base::string16 GetTimestampText() const;

  // A key to identify opt out events.
  static const void* OptOutEventKey();

 private:
  PreviewsInfoBarDelegate(PreviewsInfoBarTabHelper* infobar_tab_helper,
                          previews::PreviewsType previews_type,
                          base::Time previews_freshness,
                          bool is_data_saver_user,
                          bool is_reload,
                          OnDismissPreviewsInfobarCallback on_dismiss_callback);

  // ConfirmInfoBarDelegate overrides:
  infobars::InfoBarDelegate::InfoBarIdentifier GetIdentifier() const override;
  bool ShouldExpire(const NavigationDetails& details) const override;
  void InfoBarDismissed() override;
  int GetButtons() const override;
  bool LinkClicked(WindowOpenDisposition disposition) override;

  PreviewsInfoBarTabHelper* infobar_tab_helper_;
  previews::PreviewsType previews_type_;
  // The time at which the preview associated with this infobar was created. A
  // value of zero means that the creation time is unknown.
  const base::Time previews_freshness_;
  const bool is_reload_;
  mutable PreviewsInfoBarAction infobar_dismissed_action_;

  const base::string16 message_text_;

  OnDismissPreviewsInfobarCallback on_dismiss_callback_;

  DISALLOW_COPY_AND_ASSIGN(PreviewsInfoBarDelegate);
};

#endif  // CHROME_BROWSER_PREVIEWS_PREVIEWS_INFOBAR_DELEGATE_H_
