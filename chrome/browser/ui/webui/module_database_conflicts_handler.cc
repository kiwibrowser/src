// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/module_database_conflicts_handler.h"

#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/strings/string16.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "chrome/browser/conflicts/module_database_win.h"
#include "chrome/browser/conflicts/module_info_win.h"
#include "chrome/grit/generated_resources.h"
#include "content/public/browser/web_ui.h"
#include "ui/base/l10n/l10n_util.h"

ModuleDatabaseConflictsHandler::ModuleDatabaseConflictsHandler() = default;

ModuleDatabaseConflictsHandler::~ModuleDatabaseConflictsHandler() {
  if (module_list_)
    ModuleDatabase::GetInstance()->RemoveObserver(this);
}

void ModuleDatabaseConflictsHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "requestModuleList",
      base::BindRepeating(
          &ModuleDatabaseConflictsHandler::HandleRequestModuleList,
          base::Unretained(this)));
}

void ModuleDatabaseConflictsHandler::OnNewModuleFound(
    const ModuleInfoKey& module_key,
    const ModuleInfoData& module_data) {
  DCHECK(module_list_);

  auto data = std::make_unique<base::DictionaryValue>();

  // TODO(pmonette): Set the status when conflicting module detection is added.
  constexpr int kGoodStatus = 1;
  data->SetInteger("status", kGoodStatus);

  base::string16 type_string;
  if (module_data.module_types & ModuleInfoData::kTypeShellExtension)
    type_string = L"Shell extension";
  data->SetString("type_description", type_string);

  const auto& inspection_result = *module_data.inspection_result;
  data->SetString("location", inspection_result.location);
  data->SetString("name", inspection_result.basename);
  data->SetString("product_name", inspection_result.product_name);
  data->SetString("description", inspection_result.description);
  data->SetString("version", inspection_result.version);
  data->SetString("digital_signer", inspection_result.certificate_info.subject);

  module_list_->Append(std::move(data));
}

void ModuleDatabaseConflictsHandler::OnModuleDatabaseIdle() {
  DCHECK(module_list_);
  DCHECK(!module_list_callback_id_.empty());

  ModuleDatabase::GetInstance()->RemoveObserver(this);

  // Add the section title and the total count for bad modules found.
  // TODO(pmonette): Add the number of conflicts when conflicting module
  // detection is added.
  base::string16 table_title = l10n_util::GetStringFUTF16(
      IDS_CONFLICTS_CHECK_PAGE_TABLE_TITLE_SUFFIX_ONE,
      base::IntToString16(module_list_->GetSize()));

  base::DictionaryValue results;
  results.SetString("modulesTableTitle", table_title);
  results.Set("moduleList", std::move(module_list_));

  AllowJavascript();
  ResolveJavascriptCallback(base::Value(module_list_callback_id_), results);
}

void ModuleDatabaseConflictsHandler::HandleRequestModuleList(
    const base::ListValue* args) {
  // Make sure the JS doesn't call 'requestModuleList' more than once.
  // TODO(739291): It would be better to kill the renderer instead of the
  // browser for malformed messages.
  CHECK(!module_list_);

  CHECK_EQ(1U, args->GetSize());
  CHECK(args->GetString(0, &module_list_callback_id_));

  // The request is handled asynchronously, filling up the |module_list_|,
  // and will callback via OnModuleDatabaseIdle() on completion.
  module_list_ = std::make_unique<base::ListValue>();

  auto* module_database = ModuleDatabase::GetInstance();
  module_database->IncreaseInspectionPriority();
  module_database->AddObserver(this);
}
