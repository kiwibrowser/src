// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/version_handler.h"

#include <stddef.h>

#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/metrics/field_trial.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task_scheduler/post_task.h"
#include "base/threading/thread_restrictions.h"
#include "chrome/browser/plugins/plugin_prefs.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/grit/generated_resources.h"
#include "components/strings/grit/components_strings.h"
#include "components/variations/active_field_trials.h"
#include "components/version_ui/version_handler_helper.h"
#include "components/version_ui/version_ui_constants.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/plugin_service.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/common/content_constants.h"
#include "ppapi/buildflags/buildflags.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/gurl.h"

namespace {

// Retrieves the executable and profile paths on the FILE thread.
void GetFilePaths(const base::FilePath& profile_path,
                  base::string16* exec_path_out,
                  base::string16* profile_path_out) {
  base::AssertBlockingAllowed();

  base::FilePath executable_path = base::MakeAbsoluteFilePath(
      base::CommandLine::ForCurrentProcess()->GetProgram());
  if (!executable_path.empty())
    *exec_path_out = executable_path.LossyDisplayName();
  else
    *exec_path_out = l10n_util::GetStringUTF16(IDS_VERSION_UI_PATH_NOTFOUND);

  base::FilePath profile_path_copy(base::MakeAbsoluteFilePath(profile_path));
  if (!profile_path.empty() && !profile_path_copy.empty())
    *profile_path_out = profile_path.LossyDisplayName();
  else
    *profile_path_out = l10n_util::GetStringUTF16(IDS_VERSION_UI_PATH_NOTFOUND);
}

}  // namespace

VersionHandler::VersionHandler() : weak_ptr_factory_(this) {}

VersionHandler::~VersionHandler() {
}

void VersionHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      version_ui::kRequestVersionInfo,
      base::BindRepeating(&VersionHandler::HandleRequestVersionInfo,
                          base::Unretained(this)));
}

void VersionHandler::HandleRequestVersionInfo(const base::ListValue* args) {
  AllowJavascript();
#if BUILDFLAG(ENABLE_PLUGINS)
  // The Flash version information is needed in the response, so make sure
  // the plugins are loaded.
  content::PluginService::GetInstance()->GetPlugins(
      base::Bind(&VersionHandler::OnGotPlugins,
          weak_ptr_factory_.GetWeakPtr()));
#endif

  // Grab the executable path on the FILE thread. It is returned in
  // OnGotFilePaths.
  base::string16* exec_path_buffer = new base::string16;
  base::string16* profile_path_buffer = new base::string16;
  base::PostTaskWithTraitsAndReply(
      FROM_HERE, {base::TaskPriority::USER_VISIBLE, base::MayBlock()},
      base::BindOnce(&GetFilePaths, Profile::FromWebUI(web_ui())->GetPath(),
                     base::Unretained(exec_path_buffer),
                     base::Unretained(profile_path_buffer)),
      base::BindOnce(
          &VersionHandler::OnGotFilePaths, weak_ptr_factory_.GetWeakPtr(),
          base::Owned(exec_path_buffer), base::Owned(profile_path_buffer)));

  // Respond with the variations info immediately.
  CallJavascriptFunction(version_ui::kReturnVariationInfo,
                         *version_ui::GetVariationsList());
  GURL current_url = web_ui()->GetWebContents()->GetVisibleURL();
  if (current_url.query().find(version_ui::kVariationsShowCmdQuery) !=
      std::string::npos) {
    CallJavascriptFunction(version_ui::kReturnVariationCmd,
                           version_ui::GetVariationsCommandLineAsValue());
  }
}

void VersionHandler::OnGotFilePaths(base::string16* executable_path_data,
                                    base::string16* profile_path_data) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  base::Value exec_path(*executable_path_data);
  base::Value profile_path(*profile_path_data);
  CallJavascriptFunction(version_ui::kReturnFilePaths, exec_path, profile_path);
}

#if BUILDFLAG(ENABLE_PLUGINS)
void VersionHandler::OnGotPlugins(
    const std::vector<content::WebPluginInfo>& plugins) {
  // Obtain the version of the first enabled Flash plugin.
  std::vector<content::WebPluginInfo> info_array;
  content::PluginService::GetInstance()->GetPluginInfoArray(
      GURL(), content::kFlashPluginSwfMimeType, false, &info_array, NULL);
  std::string flash_version_and_path =
      l10n_util::GetStringUTF8(IDS_PLUGINS_DISABLED_PLUGIN);
  PluginPrefs* plugin_prefs =
      PluginPrefs::GetForProfile(Profile::FromWebUI(web_ui())).get();
  if (plugin_prefs) {
    for (size_t i = 0; i < info_array.size(); ++i) {
      if (plugin_prefs->IsPluginEnabled(info_array[i])) {
        flash_version_and_path = base::StringPrintf(
            "%s %s", base::UTF16ToUTF8(info_array[i].version).c_str(),
            base::UTF16ToUTF8(info_array[i].path.LossyDisplayName()).c_str());
        break;
      }
    }
  }

  base::Value arg(flash_version_and_path);

  CallJavascriptFunction(version_ui::kReturnFlashVersion, arg);
}
#endif  // BUILDFLAG(ENABLE_PLUGINS)
