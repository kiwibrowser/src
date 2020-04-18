// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/conflicts_handler.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/strings/string16.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "chrome/grit/generated_resources.h"
#include "content/public/browser/web_ui.h"
#include "ui/base/l10n/l10n_util.h"

ConflictsHandler::ConflictsHandler() : observer_(this) {}
ConflictsHandler::~ConflictsHandler() = default;

void ConflictsHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "requestModuleList",
      base::BindRepeating(&ConflictsHandler::HandleRequestModuleList,
                          base::Unretained(this)));
}

void ConflictsHandler::OnScanCompleted() {
  SendModuleList();
  observer_.Remove(EnumerateModulesModel::GetInstance());
}

void ConflictsHandler::HandleRequestModuleList(const base::ListValue* args) {
  auto* model = EnumerateModulesModel::GetInstance();
  // Make sure the JS doesn't call 'requestModuleList' more than once.
  // TODO(739291): It would be better to kill the renderer instead of the
  // browser for malformed messages.
  CHECK(!observer_.IsObserving(model));

  CHECK_EQ(1U, args->GetSize());
  CHECK(args->GetString(0, &module_list_callback_id_));

  // The request is handled asynchronously, and will callback via
  // OnScanCompleted on completion.
  observer_.Add(model);

  // Ask the scan to be performed immediately, and not in background mode.
  // This ensures the results are available ASAP for the UI.
  model->ScanNow(false);
}

void ConflictsHandler::SendModuleList() {
  auto* loaded_modules = EnumerateModulesModel::GetInstance();
  std::unique_ptr<base::ListValue> list = loaded_modules->GetModuleList();

  // Add the section title and the total count for bad modules found.
  int confirmed_bad = loaded_modules->confirmed_bad_modules_detected();
  int suspected_bad = loaded_modules->suspected_bad_modules_detected();
  base::string16 table_title;
  if (!confirmed_bad && !suspected_bad) {
    table_title += l10n_util::GetStringFUTF16(
        IDS_CONFLICTS_CHECK_PAGE_TABLE_TITLE_SUFFIX_ONE,
        base::IntToString16(list->GetSize()));
  } else {
    table_title += l10n_util::GetStringFUTF16(
        IDS_CONFLICTS_CHECK_PAGE_TABLE_TITLE_SUFFIX_TWO,
        base::IntToString16(list->GetSize()),
        base::IntToString16(confirmed_bad), base::IntToString16(suspected_bad));
  }
  base::DictionaryValue results;
  results.Set("moduleList", std::move(list));
  results.SetString("modulesTableTitle", table_title);

  AllowJavascript();
  ResolveJavascriptCallback(base::Value(module_list_callback_id_), results);
}
