// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_POLICY_TOOL_UI_HANDLER_H_
#define CHROME_BROWSER_UI_WEBUI_POLICY_TOOL_UI_HANDLER_H_

#include "base/files/file_path.h"
#include "chrome/browser/ui/webui/policy_ui_handler.h"

class PolicyToolUIHandler : public PolicyUIHandler {
 public:
  PolicyToolUIHandler();
  ~PolicyToolUIHandler() override;

  // content::WebUIMessageHandler implementation.
  void RegisterMessages() override;
  void OnJavascriptDisallowed() override;

 private:
  friend class PolicyToolUITest;

  enum class SessionErrors {
    kNone = 0,
    kInvalidSessionName,
    kSessionNameExist,
    kSessionNameNotExist,
    kRenamedSessionError,
  };

  // Reads the current session file (based on the session_name_) and sends the
  // contents to the UI.
  void ImportFile();

  void HandleInitializedAdmin(const base::ListValue* args);

  void HandleLoadSession(const base::ListValue* args);

  // Rename a session if the new session name doesn't exist.
  void HandleRenameSession(const base::ListValue* args);

  void HandleUpdateSession(const base::ListValue* args);

  void HandleResetSession(const base::ListValue* args);

  void HandleDeleteSession(const base::ListValue* args);

  void HandleExportLinux(const base::ListValue* args);

  void HandleExportMac(const base::ListValue* args);

  void OnSessionDeleted(bool is_successful);

  std::string ReadOrCreateFileCallback();
  void OnSessionContentReceived(const std::string& contents);

  static SessionErrors DoRenameSession(const base::FilePath& old_session_path,
                                       const base::FilePath& new_session_path);

  void OnSessionRenamed(SessionErrors result);

  bool DoUpdateSession(const std::string& contents);

  void OnSessionUpdated(bool is_successful);

  bool IsValidSessionName(const base::FilePath::StringType& name) const;

  base::FilePath GetSessionPath(const base::FilePath::StringType& name) const;

  void OnSessionsListReceived(base::ListValue list);

  void SetDefaultSessionName();

  // ui::SelectFileDialog::Listener implementation.
  void FileSelected(const base::FilePath& path,
                    int index,
                    void* params) override;

  void FileSelectionCanceled(void* params) override;

  void WriteSessionPolicyToFile(const base::FilePath& path) const;

  void ExportSessionToFile(const base::FilePath::StringType& file_extension);

  bool is_saving_enabled_ = true;

  // Parses and checks policy types for all sources.
  void ParsePolicyTypes(base::DictionaryValue* values);

  // This string is filled when an export action occurs, it contains the current
  // session dictionary in a specific format. This format will be JSON, PLIST,
  // or REG; depending on the kind of export.
  std::string session_dict_for_exporting_;
  base::FilePath sessions_dir_;
  base::FilePath::StringType session_name_;

  scoped_refptr<ui::SelectFileDialog> export_policies_select_file_dialog_;

  base::WeakPtrFactory<PolicyToolUIHandler> callback_weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(PolicyToolUIHandler);
};

#endif  // CHROME_BROWSER_UI_WEBUI_POLICY_TOOL_UI_HANDLER_H_
