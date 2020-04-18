// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/policy_tool_ui.h"

#include <memory>

#include "base/feature_list.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/policy_tool_ui_handler.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/browser_resources.h"
#include "components/strings/grit/components_strings.h"

namespace {

content::WebUIDataSource* CreatePolicyToolUIHtmlSource() {
  content::WebUIDataSource* source =
      content::WebUIDataSource::Create(chrome::kChromeUIPolicyToolHost);
  PolicyUIHandler::AddCommonLocalizedStringsToSource(source);

  source->AddLocalizedString("reloadPolicies", IDS_POLICY_RELOAD_POLICIES);
  source->AddLocalizedString("showUnset", IDS_POLICY_SHOW_UNSET);
  source->AddLocalizedString("noPoliciesSet", IDS_POLICY_NO_POLICIES_SET);
  source->AddLocalizedString("showExpandedValue",
                             IDS_POLICY_SHOW_EXPANDED_VALUE);
  source->AddLocalizedString("exportLinux", IDS_EXPORT_POLICIES_LINUX);
  source->AddLocalizedString("exportMac", IDS_EXPORT_POLICIES_MAC);
  source->AddLocalizedString("hideExpandedValue",
                             IDS_POLICY_HIDE_EXPANDED_VALUE);
  source->AddLocalizedString("loadSession", IDS_POLICY_TOOL_LOAD_SESSION);
  source->AddLocalizedString("removeSession", IDS_POLICY_TOOL_REMOVE_SESSION);
  source->AddLocalizedString("renameSession", IDS_POLICY_TOOL_RENAME_SESSION);
  source->AddLocalizedString("sessionNamePlaceholder",
                             IDS_POLICY_TOOL_SESSION_NAME_PLACEHOLDER);
  source->AddLocalizedString("filterPlaceholder",
                             IDS_POLICY_FILTER_PLACEHOLDER);
  source->AddLocalizedString("cancelRename", IDS_POLICY_TOOL_CANCEL_RENAME);
  source->AddLocalizedString("confirmRename", IDS_POLICY_TOOL_CONFIRM_RENAME);
  source->AddLocalizedString("edit", IDS_POLICY_TOOL_EDIT);
  source->AddLocalizedString("save", IDS_POLICY_TOOL_SAVE);
  source->AddLocalizedString("errorSavingDisabled",
                             IDS_POLICY_TOOL_SAVING_DISABLED);
  source->AddLocalizedString("errorInvalidSessionName",
                             IDS_POLICY_TOOL_INVALID_SESSION_NAME);
  source->AddLocalizedString("errorFileCorrupted",
                             IDS_POLICY_TOOL_CORRUPTED_FILE);
  source->AddLocalizedString("enableEditing", IDS_POLICY_TOOL_ENABLE_EDITING);
  source->AddLocalizedString("errorInvalidType", IDS_POLICY_TOOL_INVALID_TYPE);
  source->AddLocalizedString("errorRenameFailed",
                             IDS_POLICY_TOOL_RENAME_FAILED);
  source->AddLocalizedString("errorSessionExist",
                             IDS_POLICY_TOOL_SESSION_EXIST);
  source->AddLocalizedString("errorSessionNotExist",
                             IDS_POLICY_TOOL_SESSION_NOT_EXIST);
  source->AddLocalizedString("errorDeleteFailed",
                             IDS_POLICY_TOOL_DELETE_FAILED);
  // Overwrite the title value added by PolicyUIHandler.
  source->AddLocalizedString("title", IDS_POLICY_TOOL_TITLE);

  // Add required resources.
  source->AddResourcePath("policy_common.css", IDR_POLICY_COMMON_CSS);
  source->AddResourcePath("policy_tool.css", IDR_POLICY_TOOL_CSS);
  source->AddResourcePath("policy_base.js", IDR_POLICY_BASE_JS);
  source->AddResourcePath("policy_tool.js", IDR_POLICY_TOOL_JS);

  source->SetDefaultResource(IDR_POLICY_TOOL_HTML);
  source->UseGzip();
  return source;
}

}  // namespace

PolicyToolUI::PolicyToolUI(content::WebUI* web_ui) : WebUIController(web_ui) {
  web_ui->AddMessageHandler(std::make_unique<PolicyToolUIHandler>());
  content::WebUIDataSource::Add(Profile::FromWebUI(web_ui),
                                CreatePolicyToolUIHtmlSource());
}

PolicyToolUI::~PolicyToolUI() {}

// static
bool PolicyToolUI::IsEnabled() {
  return base::FeatureList::IsEnabled(features::kPolicyTool);
}
