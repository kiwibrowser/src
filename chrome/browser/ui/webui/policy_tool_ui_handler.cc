// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/policy_tool_ui_handler.h"

#include "base/files/file_enumerator.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task_scheduler/post_task.h"
#include "build/build_config.h"
#include "chrome/browser/download/download_prefs.h"
#include "chrome/browser/policy/schema_registry_service.h"
#include "chrome/browser/policy/schema_registry_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/chrome_select_file_policy.h"
#include "components/policy/core/common/plist_writer.h"
#include "components/strings/grit/components_strings.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/shell_dialogs/select_file_policy.h"

namespace {

const base::FilePath::CharType kPolicyToolSessionsDir[] =
    FILE_PATH_LITERAL("Policy sessions");

const base::FilePath::CharType kPolicyToolDefaultSessionName[] =
    FILE_PATH_LITERAL("policy");

const base::FilePath::CharType kPolicyToolSessionExtension[] =
    FILE_PATH_LITERAL("json");

const base::FilePath::CharType kPolicyToolLinuxExtension[] =
    FILE_PATH_LITERAL("json");

const base::FilePath::CharType kPolicyToolMacExtension[] =
    FILE_PATH_LITERAL("plist");

// Returns the current list of all sessions sorted by last access time in
// decreasing order.
base::ListValue GetSessionsList(const base::FilePath& sessions_dir) {
  base::FilePath::StringType sessions_pattern =
      FILE_PATH_LITERAL("*.") +
      base::FilePath::StringType(kPolicyToolSessionExtension);
  base::FileEnumerator enumerator(sessions_dir, /*recursive=*/false,
                                  base::FileEnumerator::FILES,
                                  sessions_pattern);
  // A vector of session names and their last access times.
  using Session = std::pair<base::Time, base::FilePath::StringType>;
  std::vector<Session> sessions;
  for (base::FilePath name = enumerator.Next(); !name.empty();
       name = enumerator.Next()) {
    base::File::Info info;
    base::GetFileInfo(name, &info);
    sessions.push_back(
        {info.last_accessed, name.BaseName().RemoveExtension().value()});
  }
  // Sort the sessions by the the time of last access in decreasing order.
  std::sort(sessions.begin(), sessions.end(), std::greater<Session>());

  // Convert sessions to the list containing only names.
  base::ListValue session_names;
  for (const Session& session : sessions)
    session_names.GetList().push_back(base::Value(session.second));
  return session_names;
}

// Tries to parse the value if it is necessary. If parsing was necessary, but
// not successful, returns nullptr.
std::unique_ptr<base::Value> ParseSinglePolicyType(
    const policy::Schema& policy_schema,
    const std::string& policy_name,
    base::Value* policy_value) {
  if (!policy_schema.valid())
    return nullptr;
  if (policy_value->type() == base::Value::Type::STRING &&
      policy_schema.type() != base::Value::Type::STRING) {
    return base::JSONReader::Read(policy_value->GetString());
  }
  return base::Value::ToUniquePtrValue(policy_value->Clone());
}

// Checks if the value matches the actual policy type.
bool CheckSinglePolicyType(const policy::Schema& policy_schema,
                           const std::string& policy_name,
                           base::Value* policy_value) {
  // If the schema is invalid, this means that the policy is unknown, and so
  // considered valid.
  if (!policy_schema.valid())
    return true;
  std::string error_path, error;
  return policy_schema.Validate(*policy_value, policy::SCHEMA_STRICT,
                                &error_path, &error);
}

// Parses and checks policy types for a single source (e.g. chrome policies
// or policies for one extension). For each policy, if parsing was successful
// and the parsed value matches its expected schema, replaces the policy value
// with the parsed value. Also, sets the 'valid' field to indicate whether the
// value is valid for its policy.
void ParseSourcePolicyTypes(const policy::Schema* source_schema,
                            base::Value* policies) {
  for (const auto& policy : policies->DictItems()) {
    const std::string policy_name = policy.first;
    policy::Schema policy_schema = source_schema->GetKnownProperty(policy_name);
    std::unique_ptr<base::Value> parsed_value = ParseSinglePolicyType(
        policy_schema, policy_name, policy.second.FindKey("value"));
    if (parsed_value &&
        CheckSinglePolicyType(policy_schema, policy_name, parsed_value.get())) {
      policy.second.SetKey("value", std::move(*parsed_value));
      policy.second.SetKey("valid", base::Value(true));
    } else {
      policy.second.SetKey("valid", base::Value(false));
    }
  }
}

}  // namespace

PolicyToolUIHandler::PolicyToolUIHandler() : callback_weak_ptr_factory_(this) {}

PolicyToolUIHandler::~PolicyToolUIHandler() {}

void PolicyToolUIHandler::RegisterMessages() {
  // Set directory for storing sessions.
  sessions_dir_ =
      Profile::FromWebUI(web_ui())->GetPath().Append(kPolicyToolSessionsDir);

  web_ui()->RegisterMessageCallback(
      "initialized",
      base::BindRepeating(&PolicyToolUIHandler::HandleInitializedAdmin,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "loadSession",
      base::BindRepeating(&PolicyToolUIHandler::HandleLoadSession,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "renameSession",
      base::BindRepeating(&PolicyToolUIHandler::HandleRenameSession,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "updateSession",
      base::BindRepeating(&PolicyToolUIHandler::HandleUpdateSession,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "resetSession",
      base::BindRepeating(&PolicyToolUIHandler::HandleResetSession,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "deleteSession",
      base::BindRepeating(&PolicyToolUIHandler::HandleDeleteSession,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "exportLinux",
      base::BindRepeating(&PolicyToolUIHandler::HandleExportLinux,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "exportMac", base::BindRepeating(&PolicyToolUIHandler::HandleExportMac,
                                       base::Unretained(this)));
}

void PolicyToolUIHandler::OnJavascriptDisallowed() {
  callback_weak_ptr_factory_.InvalidateWeakPtrs();
}

base::FilePath PolicyToolUIHandler::GetSessionPath(
    const base::FilePath::StringType& name) const {
  return sessions_dir_.Append(name).AddExtension(kPolicyToolSessionExtension);
}

void PolicyToolUIHandler::SetDefaultSessionName() {
  base::ListValue sessions = GetSessionsList(sessions_dir_);
  if (sessions.empty()) {
    // If there are no sessions, fallback to the default session name.
    session_name_ = kPolicyToolDefaultSessionName;
  } else {
    session_name_ =
        base::FilePath::FromUTF8Unsafe(sessions.GetList()[0].GetString())
            .value();
  }
}

std::string PolicyToolUIHandler::ReadOrCreateFileCallback() {
  // Create sessions directory, if it doesn't exist yet.
  // If unable to create a directory, just silently return a dictionary
  // indicating that saving was unsuccessful.
  // TODO(urusant): add a possibility to disable saving to disk in similar
  // cases.
  if (!base::CreateDirectory(sessions_dir_))
    is_saving_enabled_ = false;

  // Initialize session name if it is not initialized yet.
  if (session_name_.empty())
    SetDefaultSessionName();
  const base::FilePath session_path = GetSessionPath(session_name_);

  // Check if the file for the current session already exists. If not, create it
  // and put an empty dictionary in it.
  base::File session_file(session_path, base::File::Flags::FLAG_CREATE |
                                            base::File::Flags::FLAG_WRITE);

  // If unable to open the file, just return an empty dictionary.
  if (session_file.created()) {
    session_file.WriteAtCurrentPos("{}", 2);
    return "{}";
  }
  session_file.Close();

  // Check that the file exists by now. If it doesn't, it means that at least
  // one of the filesystem operations wasn't successful. In this case, return
  // a dictionary indicating that saving was unsuccessful. Potentially this can
  // also be the place to disable saving to disk.
  if (!PathExists(session_path))
    is_saving_enabled_ = false;

  // Read file contents.
  std::string contents;
  base::ReadFileToString(session_path, &contents);

  // Touch the file to remember the last session.
  base::File::Info info;
  base::GetFileInfo(session_path, &info);
  base::TouchFile(session_path, base::Time::Now(), info.last_modified);

  return contents;
}

void PolicyToolUIHandler::OnSessionContentReceived(
    const std::string& contents) {
  // If the saving is disabled, send a message about that to the UI.
  if (!is_saving_enabled_) {
    CallJavascriptFunction("policy.Page.disableSaving");
    return;
  }
  std::unique_ptr<base::DictionaryValue> value =
      base::DictionaryValue::From(base::JSONReader::Read(contents));

  // If contents is not a properly formed JSON string, disable editing in the
  // UI to prevent the user from accidentally overriding it.
  if (!value) {
    CallJavascriptFunction("policy.Page.setPolicyValues",
                           base::DictionaryValue());
    CallJavascriptFunction("policy.Page.disableEditing");
  } else {
    ParsePolicyTypes(value.get());
    CallJavascriptFunction("policy.Page.setPolicyValues", *value);
    CallJavascriptFunction("policy.Page.setSessionTitle",
                           base::Value(session_name_));
    base::PostTaskWithTraitsAndReplyWithResult(
        FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_VISIBLE},
        base::BindOnce(&GetSessionsList, sessions_dir_),
        base::BindOnce(&PolicyToolUIHandler::OnSessionsListReceived,
                       callback_weak_ptr_factory_.GetWeakPtr()));
  }
}

void PolicyToolUIHandler::OnSessionsListReceived(base::ListValue list) {
  CallJavascriptFunction("policy.Page.setSessionsList", list);
}

void PolicyToolUIHandler::ImportFile() {
  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_VISIBLE},
      base::BindOnce(&PolicyToolUIHandler::ReadOrCreateFileCallback,
                     base::Unretained(this)),
      base::BindOnce(&PolicyToolUIHandler::OnSessionContentReceived,
                     callback_weak_ptr_factory_.GetWeakPtr()));
}

void PolicyToolUIHandler::HandleInitializedAdmin(const base::ListValue* args) {
  DCHECK_EQ(0U, args->GetSize());
  AllowJavascript();
  is_saving_enabled_ = true;
  SendPolicyNames();
  ImportFile();
}

bool PolicyToolUIHandler::IsValidSessionName(
    const base::FilePath::StringType& name) const {
  // Check if the session name is valid, which means that it doesn't use
  // filesystem navigation (e.g. ../ or nested folder).

  // Sanity check to avoid that GetSessionPath(|name|) crashed.
  if (base::FilePath(name).IsAbsolute())
    return false;
  base::FilePath session_path = GetSessionPath(name);
  return !session_path.empty() && session_path.DirName() == sessions_dir_ &&
         session_path.BaseName().RemoveExtension() == base::FilePath(name) &&
         !session_path.EndsWithSeparator();
}

void PolicyToolUIHandler::HandleLoadSession(const base::ListValue* args) {
  DCHECK_EQ(1U, args->GetSize());
  base::FilePath::StringType new_session_name =
      base::FilePath::FromUTF8Unsafe(args->GetList()[0].GetString()).value();
  if (!IsValidSessionName(new_session_name)) {
    CallJavascriptFunction("policy.Page.showInvalidSessionNameError");
    return;
  }
  session_name_ = new_session_name;
  ImportFile();
}

// static
PolicyToolUIHandler::SessionErrors PolicyToolUIHandler::DoRenameSession(
    const base::FilePath& old_session_path,
    const base::FilePath& new_session_path) {
  // Check if a session files with the new name exist. If |old_session_path|
  // doesn't exist, it means that is not a valid session. If |new_session_path|
  // exists, it means that we can't do the rename because that will cause a file
  // overwrite.
  if (!PathExists(old_session_path))
    return SessionErrors::kSessionNameNotExist;
  if (PathExists(new_session_path))
    return SessionErrors::kSessionNameExist;
  if (!base::Move(old_session_path, new_session_path))
    return SessionErrors::kRenamedSessionError;
  return SessionErrors::kNone;
}

void PolicyToolUIHandler::OnSessionRenamed(
    PolicyToolUIHandler::SessionErrors result) {
  if (result == SessionErrors::kNone) {
    base::PostTaskWithTraitsAndReplyWithResult(
        FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_VISIBLE},
        base::BindOnce(&GetSessionsList, sessions_dir_),
        base::BindOnce(&PolicyToolUIHandler::OnSessionsListReceived,
                       callback_weak_ptr_factory_.GetWeakPtr()));
    CallJavascriptFunction("policy.Page.closeRenameSessionDialog");
    return;
  }
  int error_message_id;
  if (result == SessionErrors::kSessionNameNotExist) {
    error_message_id = IDS_POLICY_TOOL_SESSION_NOT_EXIST;
  } else if (result == SessionErrors::kSessionNameExist) {
    error_message_id = IDS_POLICY_TOOL_SESSION_EXIST;
  } else {
    error_message_id = IDS_POLICY_TOOL_RENAME_FAILED;
  }

  CallJavascriptFunction(
      "policy.Page.showRenameSessionError",
      base::Value(l10n_util::GetStringUTF16(error_message_id)));
}

void PolicyToolUIHandler::HandleRenameSession(const base::ListValue* args) {
  DCHECK_EQ(2U, args->GetSize());
  base::FilePath::StringType old_session_name, new_session_name;
  old_session_name =
      base::FilePath::FromUTF8Unsafe(args->GetList()[0].GetString()).value();
  new_session_name =
      base::FilePath::FromUTF8Unsafe(args->GetList()[1].GetString()).value();

  if (!IsValidSessionName(new_session_name) ||
      !IsValidSessionName(old_session_name)) {
    CallJavascriptFunction("policy.Page.showRenameSessionError",
                           base::Value(l10n_util::GetStringUTF16(
                               IDS_POLICY_TOOL_INVALID_SESSION_NAME)));
    return;
  }

  // This is important in case the user renames the current active session.
  // If we don't clear the current session name, after the rename, a new file
  // will be created with the old name and with an empty dictionary in it.
  session_name_.clear();
  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::USER_BLOCKING,
       base::TaskShutdownBehavior::BLOCK_SHUTDOWN},
      base::BindOnce(&PolicyToolUIHandler::DoRenameSession,
                     GetSessionPath(old_session_name),
                     GetSessionPath(new_session_name)),
      base::BindOnce(&PolicyToolUIHandler::OnSessionRenamed,
                     callback_weak_ptr_factory_.GetWeakPtr()));
}

bool PolicyToolUIHandler::DoUpdateSession(const std::string& contents) {
  // Sanity check that contents is not too big. Otherwise, passing it to
  // WriteFile will be int overflow.
  if (contents.size() > static_cast<size_t>(std::numeric_limits<int>::max()))
    return false;
  int bytes_written = base::WriteFile(GetSessionPath(session_name_),
                                      contents.c_str(), contents.size());
  return bytes_written == static_cast<int>(contents.size());
}

void PolicyToolUIHandler::OnSessionUpdated(bool is_successful) {
  if (!is_successful) {
    is_saving_enabled_ = false;
    CallJavascriptFunction("policy.Page.disableSaving");
  }
}

void PolicyToolUIHandler::HandleUpdateSession(const base::ListValue* args) {
  DCHECK(is_saving_enabled_);
  DCHECK_EQ(1U, args->GetSize());

  const base::DictionaryValue* policy_values = nullptr;
  args->GetDictionary(0, &policy_values);
  DCHECK(policy_values);
  std::string converted_values;
  base::JSONWriter::Write(*policy_values, &converted_values);
  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::BACKGROUND},
      base::BindOnce(&PolicyToolUIHandler::DoUpdateSession,
                     base::Unretained(this), converted_values),
      base::BindOnce(&PolicyToolUIHandler::OnSessionUpdated,
                     callback_weak_ptr_factory_.GetWeakPtr()));
  OnSessionContentReceived(converted_values);
}

void PolicyToolUIHandler::HandleResetSession(const base::ListValue* args) {
  DCHECK_EQ(0U, args->GetSize());
  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::BACKGROUND},
      base::BindOnce(&PolicyToolUIHandler::DoUpdateSession,
                     base::Unretained(this), "{}"),
      base::BindOnce(&PolicyToolUIHandler::OnSessionUpdated,
                     callback_weak_ptr_factory_.GetWeakPtr()));
}

void PolicyToolUIHandler::OnSessionDeleted(bool is_successful) {
  if (is_successful) {
    ImportFile();
  }
}

void PolicyToolUIHandler::HandleDeleteSession(const base::ListValue* args) {
  DCHECK_EQ(1U, args->GetSize());
  base::FilePath::StringType name =
      base::FilePath::FromUTF8Unsafe(args->GetList()[0].GetString()).value();

  if (!IsValidSessionName(name)) {
    return;
  }

  // Clear the current session name to ensure that we force a reload of the
  // active session. This is important in case the user has deleted the current
  // active session, in which case a new one is selected.
  session_name_.clear();
  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_VISIBLE},
      base::BindOnce(&base::DeleteFile, GetSessionPath(name),
                     /*recursive=*/false),
      base::BindOnce(&PolicyToolUIHandler::OnSessionDeleted,
                     callback_weak_ptr_factory_.GetWeakPtr()));
}

void PolicyToolUIHandler::HandleExportLinux(const base::ListValue* args) {
  DCHECK_EQ(1U, args->GetSize());
  base::JSONWriter::Write(args->GetList()[0], &session_dict_for_exporting_);
  ExportSessionToFile(kPolicyToolLinuxExtension);
}

void PolicyToolUIHandler::HandleExportMac(const base::ListValue* args) {
  DCHECK_EQ(1U, args->GetSize());
  policy::PlistWrite(args->GetList()[0], &session_dict_for_exporting_);
  ExportSessionToFile(kPolicyToolMacExtension);
}

void DoWriteSessionPolicyToFile(const base::FilePath& path,
                                const std::string& data) {
  // TODO(rodmartin): Handle when WriteFile fail.
  base::WriteFile(path, data.c_str(), data.size());
}

void PolicyToolUIHandler::WriteSessionPolicyToFile(
    const base::FilePath& path) const {
  const std::string data = session_dict_for_exporting_;
  base::PostTaskWithTraits(
      FROM_HERE,
      {base::MayBlock(), base::TaskPriority::BACKGROUND,
       base::TaskShutdownBehavior::BLOCK_SHUTDOWN},
      base::BindOnce(&DoWriteSessionPolicyToFile, path, data));
}

void PolicyToolUIHandler::FileSelected(const base::FilePath& path,
                                       int index,
                                       void* params) {
  DCHECK(export_policies_select_file_dialog_);
  WriteSessionPolicyToFile(path);
  session_dict_for_exporting_.clear();
  export_policies_select_file_dialog_ = nullptr;
}

void PolicyToolUIHandler::FileSelectionCanceled(void* params) {
  DCHECK(export_policies_select_file_dialog_);
  session_dict_for_exporting_.clear();
  export_policies_select_file_dialog_ = nullptr;
}

void PolicyToolUIHandler::ExportSessionToFile(
    const base::FilePath::StringType& file_extension) {
  // If the "select file" dialog window is already opened, we don't want to open
  // it again.
  if (export_policies_select_file_dialog_)
    return;

  content::WebContents* webcontents = web_ui()->GetWebContents();

  // Building initial path based on download preferences.
  base::FilePath initial_dir =
      DownloadPrefs::FromBrowserContext(webcontents->GetBrowserContext())
          ->DownloadPath();
  base::FilePath initial_path =
      initial_dir.Append(kPolicyToolDefaultSessionName)
          .AddExtension(file_extension);

  // TODO(rodmartin): Put an error message when the user is not allowed
  // to open select file dialogs.
  export_policies_select_file_dialog_ = ui::SelectFileDialog::Create(
      this, std::make_unique<ChromeSelectFilePolicy>(webcontents));
  ui::SelectFileDialog::FileTypeInfo file_type_info;
  file_type_info.extensions = {{file_extension}};
  gfx::NativeWindow owning_window = webcontents->GetTopLevelNativeWindow();
  export_policies_select_file_dialog_->SelectFile(
      ui::SelectFileDialog::SELECT_SAVEAS_FILE, /*title=*/base::string16(),
      initial_path, &file_type_info, /*file_type_index=*/0,
      /*default_extension=*/base::FilePath::StringType(), owning_window,
      /*params=*/nullptr);
}

void PolicyToolUIHandler::ParsePolicyTypes(
    base::DictionaryValue* policies_dict) {
  // TODO(rodmartin): Is there a better way to get the
  // types for each policy?.
  Profile* profile = Profile::FromWebUI(web_ui());
  policy::SchemaRegistry* registry =
      policy::SchemaRegistryServiceFactory::GetForContext(
          profile->GetOriginalProfile())
          ->registry();
  scoped_refptr<policy::SchemaMap> schema_map = registry->schema_map();

  for (const auto& policies_dict : policies_dict->DictItems()) {
    policy::PolicyNamespace policy_domain;
    if (policies_dict.first == "chromePolicies") {
      policy_domain = policy::PolicyNamespace(policy::POLICY_DOMAIN_CHROME, "");
    } else if (policies_dict.first == "extensionPolicies") {
      policy_domain =
          policy::PolicyNamespace(policy::POLICY_DOMAIN_EXTENSIONS, "");
    } else {
      // The domains should be only chromePolicies and extensionPolicies.
      NOTREACHED();
      continue;
    }
    const policy::Schema* schema = schema_map->GetSchema(policy_domain);
    ParseSourcePolicyTypes(schema, &policies_dict.second);
  }
}
