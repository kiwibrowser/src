// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/views/first_run_dialog.h"

#include <string>

#include "base/bind.h"
#include "base/run_loop.h"
#include "chrome/browser/first_run/first_run.h"
#include "chrome/browser/metrics/metrics_reporting_state.h"
#include "chrome/browser/platform_util.h"
#include "chrome/browser/shell_integration.h"
#include "chrome/browser/ui/browser_dialogs.h"
#include "chrome/browser/ui/views/harmony/chrome_layout_provider.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/chromium_strings.h"
#include "chrome/grit/generated_resources.h"
#include "components/strings/grit/components_strings.h"
#include "ui/aura/env.h"
#include "ui/aura/window.h"
#include "ui/aura/window_event_dispatcher.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/views/controls/button/checkbox.h"
#include "ui/views/controls/link.h"
#include "ui/views/layout/grid_layout.h"
#include "ui/views/widget/widget.h"
#include "ui/views/window/dialog_delegate.h"

#if defined(OS_WIN)
#include "components/crash/content/app/breakpad_win.h"
#elif defined(OS_LINUX)
#include "components/crash/content/app/breakpad_linux.h"
#endif

using views::GridLayout;

namespace {

void InitCrashReporterIfEnabled(bool enabled) {
  if (enabled)
    breakpad::InitCrashReporter(std::string());
}

}  // namespace

namespace first_run {

void ShowFirstRunDialog(Profile* profile) {
  FirstRunDialog::Show(profile);
}

}  // namespace first_run

// static
void FirstRunDialog::Show(Profile* profile) {
  FirstRunDialog* dialog = new FirstRunDialog(profile);
  views::DialogDelegate::CreateDialogWidget(dialog, NULL, NULL)->Show();

  base::RunLoop run_loop(base::RunLoop::Type::kNestableTasksAllowed);
  dialog->quit_runloop_ = run_loop.QuitClosure();
  run_loop.Run();
}

FirstRunDialog::FirstRunDialog(Profile* profile)
    : profile_(profile),
      make_default_(NULL),
      report_crashes_(NULL) {
  set_margins(ChromeLayoutProvider::Get()->GetDialogInsetsForContentType(
      views::TEXT, views::TEXT));
  GridLayout* layout =
      SetLayoutManager(std::make_unique<views::GridLayout>(this));

  views::ColumnSet* column_set = layout->AddColumnSet(0);
  column_set->AddColumn(GridLayout::FILL, GridLayout::CENTER, 0,
                        GridLayout::USE_PREF, 0, 0);

  layout->StartRow(0, 0);
  make_default_ = new views::Checkbox(l10n_util::GetStringUTF16(
      IDS_FR_CUSTOMIZE_DEFAULT_BROWSER));
  make_default_->SetChecked(true);
  layout->AddView(make_default_);

  layout->StartRowWithPadding(0, 0, 0,
                              ChromeLayoutProvider::Get()->GetDistanceMetric(
                                  views::DISTANCE_RELATED_CONTROL_VERTICAL));
  report_crashes_ = new views::Checkbox(
      l10n_util::GetStringUTF16(IDS_SETTINGS_ENABLE_LOGGING));
  // Having this box checked means the user has to opt-out of metrics recording.
  report_crashes_->SetChecked(!first_run::IsMetricsReportingOptIn());
  layout->AddView(report_crashes_);
  chrome::RecordDialogCreation(chrome::DialogIdentifier::FIRST_RUN_DIALOG);
}

FirstRunDialog::~FirstRunDialog() {
}

void FirstRunDialog::Done() {
  CHECK(!quit_runloop_.is_null());
  quit_runloop_.Run();
}

views::View* FirstRunDialog::CreateExtraView() {
  views::Link* link = new views::Link(l10n_util::GetStringUTF16(
      IDS_LEARN_MORE));
  link->set_listener(this);
  return link;
}

bool FirstRunDialog::Accept() {
  GetWidget()->Hide();

  ChangeMetricsReportingStateWithReply(report_crashes_->checked(),
                                       base::Bind(&InitCrashReporterIfEnabled));

  if (make_default_->checked())
    shell_integration::SetAsDefaultBrowser();

  Done();
  return true;
}

int FirstRunDialog::GetDialogButtons() const {
  return ui::DIALOG_BUTTON_OK;
}

void FirstRunDialog::WindowClosing() {
  first_run::SetShouldShowWelcomePage();
  Done();
}

void FirstRunDialog::LinkClicked(views::Link* source, int event_flags) {
  platform_util::OpenExternal(profile_, GURL(chrome::kLearnMoreReportingURL));
}
