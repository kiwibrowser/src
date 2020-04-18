// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/conflicting_module_view_win.h"

#include "base/metrics/histogram_macros.h"
#include "base/metrics/user_metrics.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_dialogs.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/views/harmony/chrome_layout_provider.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/grit/locale_settings.h"
#include "chrome/grit/theme_resources.h"
#include "components/strings/grit/components_strings.h"
#include "content/public/browser/notification_service.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/label.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/widget/widget.h"

using base::UserMetricsAction;

namespace {

// How often to show this bubble.
const int kShowConflictingModuleBubbleMax = 3;

}  // namespace

////////////////////////////////////////////////////////////////////////////////
// ConflictingModuleView

ConflictingModuleView::ConflictingModuleView(views::View* anchor_view,
                                             Browser* browser,
                                             const GURL& help_center_url)
    : BubbleDialogDelegateView(anchor_view, views::BubbleBorder::TOP_RIGHT),
      browser_(browser),
      observer_(this),
      help_center_url_(help_center_url) {
  set_close_on_deactivate(false);

  // Compensate for built-in vertical padding in the anchor view's image.
  set_anchor_view_insets(gfx::Insets(
      GetLayoutConstant(LOCATION_BAR_BUBBLE_ANCHOR_VERTICAL_INSET), 0));

  observer_.Add(EnumerateModulesModel::GetInstance());

  chrome::RecordDialogCreation(chrome::DialogIdentifier::CONFLICTING_MODULE);
}

// static
void ConflictingModuleView::MaybeShow(Browser* browser,
                                      views::View* anchor_view) {
  static bool done_checking = false;
  if (done_checking)
    return;  // Only show the bubble once per launch.

  // A pref that counts how often the Sideload Wipeout bubble has been shown.
  IntegerPrefMember bubble_shown;
  bubble_shown.Init(prefs::kModuleConflictBubbleShown,
                    browser->profile()->GetPrefs());
  if (bubble_shown.GetValue() >= kShowConflictingModuleBubbleMax) {
    done_checking = true;
    return;
  }

  // |anchor_view| must be in a widget (the browser's widget). If not, |browser|
  // may be destroyed before us, and we'll crash trying to access |browser|
  // later on. We can't DCHECK |browser|'s widget here as we may be called from
  // creation of BrowserWindow, which means browser->window() may return NULL.
  DCHECK(anchor_view);
  DCHECK(anchor_view->GetWidget());

  ConflictingModuleView* bubble_delegate = new ConflictingModuleView(
      anchor_view, browser, GURL(chrome::kChromeUIConflictsURL));
  views::BubbleDialogDelegateView::CreateBubble(bubble_delegate);
  bubble_delegate->ShowBubble();

  done_checking = true;
}

////////////////////////////////////////////////////////////////////////////////
// ConflictingModuleView - private.

ConflictingModuleView::~ConflictingModuleView() {
}

void ConflictingModuleView::ShowBubble() {
  GetWidget()->Show();

  IntegerPrefMember bubble_shown;
  bubble_shown.Init(
      prefs::kModuleConflictBubbleShown,
      browser_->profile()->GetPrefs());
  bubble_shown.SetValue(bubble_shown.GetValue() + 1);
}

void ConflictingModuleView::OnWidgetClosing(views::Widget* widget) {
  views::BubbleDialogDelegateView::OnWidgetClosing(widget);
  base::RecordAction(
      UserMetricsAction("ConflictingModuleNotificationDismissed"));
}

bool ConflictingModuleView::Accept() {
  browser_->OpenURL(
      content::OpenURLParams(help_center_url_, content::Referrer(),
                             WindowOpenDisposition::NEW_FOREGROUND_TAB,
                             ui::PAGE_TRANSITION_LINK, false));
  EnumerateModulesModel::GetInstance()->AcknowledgeConflictNotification();
  return true;
}

base::string16 ConflictingModuleView::GetDialogButtonLabel(
    ui::DialogButton button) const {
  return l10n_util::GetStringUTF16(button == ui::DIALOG_BUTTON_OK
                                       ? IDS_CONFLICTS_LEARN_MORE
                                       : IDS_NOT_NOW);
}

void ConflictingModuleView::Init() {
  ui::ResourceBundle& rb = ui::ResourceBundle::GetSharedInstance();

  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::kHorizontal, gfx::Insets(),
      ChromeLayoutProvider::Get()->GetDistanceMetric(
          views::DISTANCE_RELATED_LABEL_HORIZONTAL)));

  views::ImageView* icon = new views::ImageView();
  icon->SetImage(rb.GetImageSkiaNamed(IDR_INPUT_ALERT_MENU));
  AddChildView(icon);

  views::Label* explanation = new views::Label();
  explanation->SetMultiLine(true);
  explanation->SetHorizontalAlignment(gfx::ALIGN_LEFT);
  explanation->SetText(
      l10n_util::GetStringUTF16(IDS_OPTIONS_CONFLICTING_MODULE));
  explanation->SizeToFit(views::Widget::GetLocalizedContentsWidth(
      IDS_CONFLICTING_MODULE_BUBBLE_WIDTH_CHARS));
  AddChildView(explanation);

  base::RecordAction(UserMetricsAction("ConflictingModuleNotificationShown"));

  UMA_HISTOGRAM_ENUMERATION("ConflictingModule.UserSelection",
      EnumerateModulesModel::ACTION_BUBBLE_SHOWN,
      EnumerateModulesModel::ACTION_BOUNDARY);
}

void ConflictingModuleView::OnConflictsAcknowledged() {
  EnumerateModulesModel* model = EnumerateModulesModel::GetInstance();
  if (!model->ShouldShowConflictWarning())
    GetWidget()->Close();
}
