// Copyright (c) 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/system_log_uploader.h"

#include <map>
#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/sequenced_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "base/syslog_logging.h"
#include "base/task_scheduler/post_task.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chromeos/policy/upload_job_impl.h"
#include "chrome/browser/chromeos/settings/device_oauth2_token_service.h"
#include "chrome/browser/chromeos/settings/device_oauth2_token_service_factory.h"
#include "chrome/browser/policy/policy_conversions.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/common/chrome_switches.h"
#include "components/feedback/anonymizer_tool.h"
#include "components/policy/core/browser/browser_policy_connector.h"
#include "components/user_manager/user_manager.h"
#include "net/http/http_request_headers.h"

namespace policy {

namespace {

// The maximum number of successive retries.
const int kMaxNumRetries = 1;

// String constant defining the url tail we upload system logs to.
constexpr char kSystemLogUploadUrlTail[] = "/upload";

// The cutoff point (in bytes) after which log contents are ignored.
const size_t kLogCutoffSize = 50 * 1024 * 1024;  // 50 MiB.

// Pseudo-location of policy dump file. Policy is uploaded from memory,
// there is no actual file on disk.
constexpr char kPolicyDumpFileLocation[] = "/var/log/policy_dump.json";

// The file names of the system logs to upload.
// Note: do not add anything to this list without checking for PII in the file.
const char* const kSystemLogFileNames[] = {
    "/var/log/bios_info.txt",
    "/var/log/chrome/chrome", "/var/log/chrome/chrome.PREVIOUS",
    "/var/log/eventlog.txt",  "/var/log/platform_info.txt",
    "/var/log/messages",      "/var/log/messages.1",
    "/var/log/net.log",       "/var/log/net.1.log",
    "/var/log/ui/ui.LATEST",  "/var/log/update_engine.log"};

std::string ReadAndAnonymizeLogFile(feedback::AnonymizerTool* anonymizer,
                                    const base::FilePath& file_path) {
  std::string data;
  if (!base::ReadFileToStringWithMaxSize(file_path, &data, kLogCutoffSize) &&
      data.empty()) {
    SYSLOG(ERROR) << "Failed to read the system log file from the disk "
                  << file_path.value();
  }
  // We want to remove the last line completely because PII data might be cut in
  // half (anonymizer might not recognize it).
  if (!data.empty() && data.back() != '\n') {
    size_t pos = data.find_last_of('\n');
    data.erase(pos != std::string::npos ? pos + 1 : 0);
    data += "... [truncated]\n";
  }
  return SystemLogUploader::RemoveSensitiveData(anonymizer, data);
}

// Reads the system log files as binary files, anonymizes data, stores the files
// as pairs (file name, data) and returns. Called on blocking thread.
std::unique_ptr<SystemLogUploader::SystemLogs> ReadFiles() {
  auto system_logs = std::make_unique<SystemLogUploader::SystemLogs>();
  feedback::AnonymizerTool anonymizer;
  for (const char* file_path : kSystemLogFileNames) {
    if (!base::PathExists(base::FilePath(file_path)))
      continue;
    system_logs->push_back(std::make_pair(
        file_path,
        ReadAndAnonymizeLogFile(&anonymizer, base::FilePath(file_path))));
  }
  return system_logs;
}

// An implementation of the |SystemLogUploader::Delegate|, that is used to
// create an upload job and load system logs from the disk.
class SystemLogDelegate : public SystemLogUploader::Delegate {
 public:
  explicit SystemLogDelegate(
      scoped_refptr<base::SequencedTaskRunner> task_runner);
  ~SystemLogDelegate() override;

  // SystemLogUploader::Delegate:
  std::string GetPolicyAsJSON() override;
  void LoadSystemLogs(LogUploadCallback upload_callback) override;

  std::unique_ptr<UploadJob> CreateUploadJob(
      const GURL& upload_url,
      UploadJob::Delegate* delegate) override;

 private:
  // TaskRunner used for scheduling upload the upload task.
  const scoped_refptr<base::SequencedTaskRunner> task_runner_;

  DISALLOW_COPY_AND_ASSIGN(SystemLogDelegate);
};

SystemLogDelegate::SystemLogDelegate(
    scoped_refptr<base::SequencedTaskRunner> task_runner)
    : task_runner_(task_runner) {}

SystemLogDelegate::~SystemLogDelegate() {}

std::string SystemLogDelegate::GetPolicyAsJSON() {
  bool include_user_policies = false;
  if (user_manager::UserManager::IsInitialized()) {
    if (user_manager::UserManager::Get()->GetPrimaryUser()) {
      include_user_policies =
          user_manager::UserManager::Get()->GetPrimaryUser()->IsAffiliated();
    }
  }
  return policy::GetAllPolicyValuesAsJSON(
      ProfileManager::GetActiveUserProfile(), include_user_policies);
}

void SystemLogDelegate::LoadSystemLogs(LogUploadCallback upload_callback) {
  // Run ReadFiles() in the thread that interacts with the file system and
  // return system logs to |upload_callback| on the current thread.
  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::BACKGROUND},
      base::BindOnce(&ReadFiles), std::move(upload_callback));
}

std::unique_ptr<UploadJob> SystemLogDelegate::CreateUploadJob(
    const GURL& upload_url,
    UploadJob::Delegate* delegate) {
  chromeos::DeviceOAuth2TokenService* device_oauth2_token_service =
      chromeos::DeviceOAuth2TokenServiceFactory::Get();

  scoped_refptr<net::URLRequestContextGetter> system_request_context =
      g_browser_process->system_request_context();
  std::string robot_account_id =
      device_oauth2_token_service->GetRobotAccountId();

  SYSLOG(INFO) << "Creating upload job for system log";
  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("policy_system_logs", R"(
        semantics {
          sender: "Chrome OS system log uploader"
          description:
              "Admins can ask that their devices regularly upload their system "
              "logs."
          trigger: "After reboot and every 12 hours."
          data: "Non-user specific, anonymized system logs from /var/log/."
          destination: GOOGLE_OWNED_SERVICE
        }
        policy {
          cookies_allowed: NO
          setting: "This feature cannot be disabled in settings."
          chrome_policy {
            LogUploadEnabled {
                LogUploadEnabled: false
            }
          }
        }
      )");
  return std::make_unique<UploadJobImpl>(
      upload_url, robot_account_id, device_oauth2_token_service,
      system_request_context, delegate,
      std::make_unique<UploadJobImpl::RandomMimeBoundaryGenerator>(),
      traffic_annotation, task_runner_);
}

// Returns the system log upload frequency.
base::TimeDelta GetUploadFrequency() {
  base::TimeDelta upload_frequency(base::TimeDelta::FromMilliseconds(
      SystemLogUploader::kDefaultUploadDelayMs));
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kSystemLogUploadFrequency)) {
    std::string string_value =
        base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
            switches::kSystemLogUploadFrequency);
    int frequency;
    if (base::StringToInt(string_value, &frequency)) {
      upload_frequency = base::TimeDelta::FromMilliseconds(frequency);
    }
  }
  return upload_frequency;
}

std::string GetUploadUrl() {
  return BrowserPolicyConnector::GetDeviceManagementUrl() +
         kSystemLogUploadUrlTail;
}

}  // namespace

// Determines the time between log uploads.
const int64_t SystemLogUploader::kDefaultUploadDelayMs =
    12 * 60 * 60 * 1000;  // 12 hours

// Determines the time, measured from the time of last failed upload,
// after which the log upload is retried.
const int64_t SystemLogUploader::kErrorUploadDelayMs =
    120 * 1000;  // 120 seconds

// String constant identifying the header field which stores the file type.
const char* const SystemLogUploader::kFileTypeHeaderName = "File-Type";

// String constant signalling that the data segment contains log files.
const char* const SystemLogUploader::kFileTypeLogFile = "log_file";

// String constant signalling that the segment contains a plain text.
const char* const SystemLogUploader::kContentTypePlainText = "text/plain";

// Template string constant for populating the name field.
const char* const SystemLogUploader::kNameFieldTemplate = "file%d";

SystemLogUploader::SystemLogUploader(
    std::unique_ptr<Delegate> syslog_delegate,
    const scoped_refptr<base::SequencedTaskRunner>& task_runner)
    : retry_count_(0),
      upload_frequency_(GetUploadFrequency()),
      task_runner_(task_runner),
      syslog_delegate_(std::move(syslog_delegate)),
      upload_enabled_(false),
      weak_factory_(this) {
  if (!syslog_delegate_)
    syslog_delegate_ = std::make_unique<SystemLogDelegate>(task_runner);
  DCHECK(syslog_delegate_);
  SYSLOG(INFO) << "Creating system log uploader.";

  // Watch for policy changes.
  upload_enabled_observer_ = chromeos::CrosSettings::Get()->AddSettingsObserver(
      chromeos::kSystemLogUploadEnabled,
      base::Bind(&SystemLogUploader::RefreshUploadSettings,
                 base::Unretained(this)));

  // Fetch the current value of the policy.
  RefreshUploadSettings();

  // Immediately schedule the next system log upload (last_upload_attempt_ is
  // set to the start of the epoch, so this will trigger an update upload in the
  // immediate future).
  ScheduleNextSystemLogUpload(upload_frequency_);
}

SystemLogUploader::~SystemLogUploader() {}

void SystemLogUploader::OnSuccess() {
  SYSLOG(INFO) << "Upload successful.";
  upload_job_.reset();
  last_upload_attempt_ = base::Time::NowFromSystemTime();
  log_upload_in_progress_ = false;
  retry_count_ = 0;

  // On successful log upload schedule the next log upload after
  // upload_frequency_ time from now.
  ScheduleNextSystemLogUpload(upload_frequency_);
}

void SystemLogUploader::OnFailure(UploadJob::ErrorCode error_code) {
  upload_job_.reset();
  last_upload_attempt_ = base::Time::NowFromSystemTime();
  log_upload_in_progress_ = false;

  //  If we have hit the maximum number of retries, terminate this upload
  //  attempt and schedule the next one using the normal delay. Otherwise, retry
  //  uploading after kErrorUploadDelayMs milliseconds.
  if (retry_count_++ < kMaxNumRetries) {
    SYSLOG(ERROR) << "Upload failed with error code " << error_code
                  << ", retrying later.";
    ScheduleNextSystemLogUpload(
        base::TimeDelta::FromMilliseconds(kErrorUploadDelayMs));
  } else {
    // No more retries.
    SYSLOG(ERROR) << "Upload failed with error code " << error_code
                  << ", no more retries.";
    retry_count_ = 0;
    ScheduleNextSystemLogUpload(upload_frequency_);
  }
}

// static
std::string SystemLogUploader::RemoveSensitiveData(
    feedback::AnonymizerTool* anonymizer,
    const std::string& data) {
  return anonymizer->Anonymize(data);
}

void SystemLogUploader::ScheduleNextSystemLogUploadImmediately() {
  ScheduleNextSystemLogUpload(base::TimeDelta());
}

void SystemLogUploader::RefreshUploadSettings() {
  // Attempt to fetch the current value of the reporting settings.
  // If trusted values are not available, register this function to be called
  // back when they are available.
  chromeos::CrosSettings* settings = chromeos::CrosSettings::Get();
  auto trust_status = settings->PrepareTrustedValues(base::Bind(
      &SystemLogUploader::RefreshUploadSettings, weak_factory_.GetWeakPtr()));
  if (trust_status != chromeos::CrosSettingsProvider::TRUSTED)
    return;

  // CrosSettings are trusted - we want to use the last trusted values, by
  // default do not upload system logs.
  if (!settings->GetBoolean(chromeos::kSystemLogUploadEnabled,
                            &upload_enabled_)) {
    upload_enabled_ = false;
  }
}

void SystemLogUploader::UploadSystemLogs(
    std::unique_ptr<SystemLogs> system_logs) {
  // Must be called on the main thread.
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(!upload_job_);

  SYSLOG(INFO) << "Uploading system logs.";

  GURL upload_url(GetUploadUrl());
  DCHECK(upload_url.is_valid());
  upload_job_ = syslog_delegate_->CreateUploadJob(upload_url, this);

  // Start a system log upload.
  int file_number = 1;
  for (const auto& syslog_entry : *system_logs) {
    std::map<std::string, std::string> header_fields;
    std::unique_ptr<std::string> data =
        std::make_unique<std::string>(syslog_entry.second);
    header_fields.insert(std::make_pair(kFileTypeHeaderName, kFileTypeLogFile));
    header_fields.insert(std::make_pair(net::HttpRequestHeaders::kContentType,
                                        kContentTypePlainText));
    upload_job_->AddDataSegment(
        base::StringPrintf(kNameFieldTemplate, file_number), syslog_entry.first,
        header_fields, std::move(data));
    ++file_number;
  }
  upload_job_->Start();
}

void SystemLogUploader::StartLogUpload() {
  // Must be called on the main thread.
  DCHECK(thread_checker_.CalledOnValidThread());

  if (upload_enabled_) {
    SYSLOG(INFO) << "Reading system logs for upload.";
    log_upload_in_progress_ = true;
    syslog_delegate_->LoadSystemLogs(base::BindOnce(
        &SystemLogUploader::OnSystemLogsLoaded, weak_factory_.GetWeakPtr()));
  } else {
    // If upload is disabled, schedule the next attempt after 12h.
    SYSLOG(INFO) << "System log upload is disabled, rescheduling.";
    retry_count_ = 0;
    last_upload_attempt_ = base::Time::NowFromSystemTime();
    ScheduleNextSystemLogUpload(upload_frequency_);
  }
}

void SystemLogUploader::OnSystemLogsLoaded(
    std::unique_ptr<SystemLogs> system_logs) {
  // Must be called on the main thread.
  DCHECK(thread_checker_.CalledOnValidThread());
  system_logs->push_back(std::make_pair(kPolicyDumpFileLocation,
                                        syslog_delegate_->GetPolicyAsJSON()));
  SYSLOG(INFO) << "Starting system log upload.";
  UploadSystemLogs(std::move(system_logs));
}

void SystemLogUploader::ScheduleNextSystemLogUpload(base::TimeDelta frequency) {
  // Don't schedule a new system log upload if there's a log upload in progress
  // (it will be scheduled once the current one completes).
  if (log_upload_in_progress_) {
    SYSLOG(INFO) << "In the middle of a system log upload, not scheduling the "
                 << "next one until this one finishes.";
    return;
  }

  // Calculate when to fire off the next update.
  base::TimeDelta delay = std::max(
      (last_upload_attempt_ + frequency) - base::Time::NowFromSystemTime(),
      base::TimeDelta());
  SYSLOG(INFO) << "Scheduling next system log upload " << delay << " from now.";
  // Ensure that we never have more than one pending delayed task
  // (InvalidateWeakPtrs() will cancel any pending log uploads).
  weak_factory_.InvalidateWeakPtrs();
  task_runner_->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&SystemLogUploader::StartLogUpload,
                     weak_factory_.GetWeakPtr()),
      delay);
}

}  // namespace policy
