// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/webrtc/webrtc_event_log_manager_remote.h"

#include <algorithm>
#include <iterator>
#include <limits>

#include "base/big_endian.h"
#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/rand_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "chrome/common/chrome_switches.h"
#include "content/public/browser/browser_thread.h"

const size_t kMaxRemoteLogFileMetadataSizeBytes = 0xffffu;  // 65535
static_assert(kMaxRemoteLogFileMetadataSizeBytes <= 0xFFFFFFu,
              "Only 24 bits available for encoding the metadata's length.");

// TODO(crbug.com/775415): Change back to (1u << 29) after resolving the issue
// where we read the entire file into memory.
const size_t kMaxRemoteLogFileSizeBytes = 50000000u;

namespace {
const base::TimeDelta kDefaultProactivePruningDelta =
    base::TimeDelta::FromMinutes(5);

const base::FilePath::CharType kRemoteBoundLogSubDirectory[] =
    FILE_PATH_LITERAL("webrtc_event_logs");

// Purge from local disk a log file which could not be properly started
// (e.g. error encountered when attempting to write the log header).
void DiscardLogFile(base::File* file, const base::FilePath& file_path) {
  file->Close();
  if (!base::DeleteFile(file_path, /*recursive=*/false)) {
    LOG(ERROR) << "Failed to delete " << file_path << ".";
  }
}

bool AreLogParametersValid(size_t max_file_size_bytes,
                           const std::string& metadata,
                           std::string* error_message) {
  if (max_file_size_bytes == kWebRtcEventLogManagerUnlimitedFileSize) {
    LOG(WARNING) << "Unlimited file sizes not allowed for remote-bound logs.";
    *error_message = kStartRemoteLoggingFailureUnlimitedSizeDisallowed;
    return false;
  }

  if (max_file_size_bytes > kMaxRemoteLogFileSizeBytes) {
    LOG(WARNING) << "File size exceeds maximum allowed.";
    *error_message = kStartRemoteLoggingFailureMaxSizeTooLarge;
    return false;
  }

  if (metadata.length() > kMaxRemoteLogFileMetadataSizeBytes) {
    LOG(ERROR) << "Excessively long metadata.";
    *error_message = kStartRemoteLoggingFailureMetadaTooLong;
    return false;
  }

  if (metadata.size() + kRemoteBoundLogFileHeaderSizeBytes >=
      max_file_size_bytes) {
    LOG(ERROR) << "Max file size and metadata must leave room for event log.";
    *error_message = kStartRemoteLoggingFailureMaxSizeTooSmall;
    return false;
  }

  return true;
}

base::Optional<base::TimeDelta> GetProactivePruningDelta() {
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          ::switches::kWebRtcRemoteEventLogProactivePruningDelta)) {
    const std::string delta_seconds_str =
        base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
            ::switches::kWebRtcRemoteEventLogProactivePruningDelta);
    int64_t seconds;
    if (base::StringToInt64(delta_seconds_str, &seconds) && seconds >= 0) {
      // A delta of 0 seconds is used to signal the intention of disabling
      // proactive pruning altogether. (From the command line. Past the command
      // line, we use an unset optional to signal that.)
      return (seconds == 0) ? base::Optional<base::TimeDelta>()
                            : base::TimeDelta::FromSeconds(seconds);
    } else {
      LOG(WARNING) << "Proactive pruning delta could not be parsed.";
    }
  }

  return kDefaultProactivePruningDelta;
}
}  // namespace

const size_t kMaxActiveRemoteBoundWebRtcEventLogs = 3;
const size_t kMaxPendingRemoteBoundWebRtcEventLogs = 5;
static_assert(kMaxActiveRemoteBoundWebRtcEventLogs <=
                  kMaxPendingRemoteBoundWebRtcEventLogs,
              "This assumption affects unit test coverage.");

const base::TimeDelta kRemoteBoundWebRtcEventLogsMaxRetention =
    base::TimeDelta::FromDays(3);

const base::FilePath::CharType kRemoteBoundLogExtension[] =
    FILE_PATH_LITERAL("log");

const uint8_t kRemoteBoundWebRtcEventLogFileVersion = 0;

const size_t kRemoteBoundLogFileHeaderSizeBytes = sizeof(uint32_t);

WebRtcRemoteEventLogManager::WebRtcRemoteEventLogManager(
    WebRtcRemoteEventLogsObserver* observer)
    : upload_suppression_disabled_(
          base::CommandLine::ForCurrentProcess()->HasSwitch(
              ::switches::kWebRtcRemoteEventLogUploadNoSuppression)),
      proactive_prune_scheduling_delta_(GetProactivePruningDelta()),
      proactive_prune_scheduling_started_(false),
      observer_(observer),
      uploader_factory_(new WebRtcEventLogUploaderImpl::Factory) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  DETACH_FROM_SEQUENCE(io_task_sequence_checker_);
  // Proactive pruning would not do anything at the moment; it will be started
  // with the first enabled browser context. This will all have the benefit
  // of doing so on io_task_sequence_checker_ rather than the UI thread.
}

WebRtcRemoteEventLogManager::~WebRtcRemoteEventLogManager() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  // TODO(crbug.com/775415): Purge from disk files which were being uploaded
  // while destruction took place, thereby avoiding endless attempts to upload
  // the same file.
}

void WebRtcRemoteEventLogManager::EnableForBrowserContext(
    BrowserContextId browser_context_id,
    const base::FilePath& browser_context_dir) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_task_sequence_checker_);
  DCHECK(!BrowserContextEnabled(browser_context_id)) << "Already enabled.";

  const base::FilePath remote_bound_logs_dir =
      GetLogsDirectoryPath(browser_context_dir);
  if (!MaybeCreateLogsDirectory(remote_bound_logs_dir)) {
    LOG(WARNING)
        << "WebRtcRemoteEventLogManager couldn't create logs directory.";
    return;
  }

  AddPendingLogs(browser_context_id, remote_bound_logs_dir);

  enabled_browser_contexts_.insert(browser_context_id);

  if (proactive_prune_scheduling_delta_.has_value() &&
      !proactive_prune_scheduling_started_) {
    proactive_prune_scheduling_started_ = true;
    RecurringPendingLogsPrune();
  }
}

// TODO(crbug.com/775415): Add unit tests.
void WebRtcRemoteEventLogManager::DisableForBrowserContext(
    BrowserContextId browser_context_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_task_sequence_checker_);

  if (!BrowserContextEnabled(browser_context_id)) {
    return;  // Enabling may have failed due to lacking permissions.
  }

  enabled_browser_contexts_.erase(browser_context_id);

#if DCHECK_IS_ON()
  // All of the RPHs associated with this BrowserContext must already have
  // exited, which should have implicitly stopped all active logs.
  auto pred = [browser_context_id](decltype(active_logs_)::value_type& log) {
    return log.first.browser_context_id == browser_context_id;
  };
  DCHECK(std::count_if(active_logs_.begin(), active_logs_.end(), pred) == 0u);
#endif

  // Pending logs for this BrowserContext are no longer eligible for upload.
  // (Active uploads, if any, are not affected.)
  for (auto it = pending_logs_.begin(); it != pending_logs_.end();) {
    if (it->browser_context_id == browser_context_id) {
      it = pending_logs_.erase(it);
    } else {
      ++it;
    }
  }
}

bool WebRtcRemoteEventLogManager::PeerConnectionAdded(
    const PeerConnectionKey& key,
    const std::string& peer_connection_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_task_sequence_checker_);
  PrunePendingLogs();  // Infrequent event - good opportunity to prune.
  const auto result = active_peer_connections_.emplace(key, peer_connection_id);
  return result.second;
}

bool WebRtcRemoteEventLogManager::PeerConnectionRemoved(
    const PeerConnectionKey& key) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_task_sequence_checker_);

  PrunePendingLogs();  // Infrequent event - good opportunity to prune.

  const auto peer_connection = active_peer_connections_.find(key);
  if (peer_connection == active_peer_connections_.end()) {
    return false;
  }

  MaybeStopRemoteLogging(key);

  active_peer_connections_.erase(peer_connection);

  MaybeStartUploading();

  return true;
}

bool WebRtcRemoteEventLogManager::StartRemoteLogging(
    int render_process_id,
    BrowserContextId browser_context_id,
    const std::string& peer_connection_id,
    const base::FilePath& browser_context_dir,
    size_t max_file_size_bytes,
    const std::string& metadata,
    std::string* error_message) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_task_sequence_checker_);
  DCHECK(error_message);
  DCHECK(error_message->empty());

  if (!AreLogParametersValid(max_file_size_bytes, metadata, error_message)) {
    // |error_message| will have been set by AreLogParametersValid().
    DCHECK(!error_message->empty()) << "AreLogParametersValid() reported an "
                                       "error without an error message.";
    return false;
  }

  if (!BrowserContextEnabled(browser_context_id)) {
    *error_message = kStartRemoteLoggingFailureGeneric;
    return false;
  }

  PeerConnectionKey key;
  if (!FindPeerConnection(render_process_id, peer_connection_id, &key)) {
    *error_message = kStartRemoteLoggingFailureUnknownOrInactivePeerConnection;
    return false;
  }

  // May not restart active remote logs.
  auto it = active_logs_.find(key);
  if (it != active_logs_.end()) {
    LOG(ERROR) << "Remote logging already underway for ("
               << key.render_process_id << ", " << key.lid << ").";
    *error_message = kStartRemoteLoggingFailureAlreadyLogging;
    return false;
  }

  // This is a good opportunity to prune the list of pending logs, potentially
  // making room for this file.
  PrunePendingLogs();

  if (!AdditionalActiveLogAllowed(key.browser_context_id)) {
    // Intentionally use a generic error, so as to not leak information such
    // as this being an incognito session (rejected elsewhere with the same
    // error), or there being too many other peer connections on other tabs
    // that might also be logging.
    *error_message = kStartRemoteLoggingFailureGeneric;
    return false;
  }

  return StartWritingLog(key, browser_context_dir, max_file_size_bytes,
                         metadata, error_message);
}

bool WebRtcRemoteEventLogManager::EventLogWrite(const PeerConnectionKey& key,
                                                const std::string& message) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_task_sequence_checker_);

  auto it = active_logs_.find(key);
  if (it == active_logs_.end()) {
    return false;
  }
  return WriteToLogFile(it, message);
}

void WebRtcRemoteEventLogManager::ClearCacheForBrowserContext(
    BrowserContextId browser_context_id,
    const base::Time& delete_begin,
    const base::Time& delete_end) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_task_sequence_checker_);
  RemovePendingLogs(delete_begin, delete_end, browser_context_id);
}

void WebRtcRemoteEventLogManager::RenderProcessHostExitedDestroyed(
    int render_process_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_task_sequence_checker_);

  // Closing files will call MaybeStartUploading(). Avoid letting that upload
  // any recently expired files.
  PrunePendingLogs();

  // Remove all of the peer connections associated with this render process.
  // It's important to do this before closing the actual files, because closing
  // files can trigger a new upload if no active peer connections are present.
  auto pc_it = active_peer_connections_.begin();
  while (pc_it != active_peer_connections_.end()) {
    if (pc_it->first.render_process_id == render_process_id) {
      pc_it = active_peer_connections_.erase(pc_it);
    } else {
      ++pc_it;
    }
  }

  // Close all of the files that were associated with peer connections which
  // belonged to this render process.
  auto log_it = active_logs_.begin();
  while (log_it != active_logs_.end()) {
    if (log_it->first.render_process_id == render_process_id) {
      log_it = CloseLogFile(log_it);
    } else {
      ++log_it;
    }
  }

  // Though CloseLogFile() calls this, it's important to also do this
  // explicitly, since it could be that no files were closed, but some
  // active PeerConnections that were suppressing uploading are now gone.
  MaybeStartUploading();
}

void WebRtcRemoteEventLogManager::OnWebRtcEventLogUploadComplete(
    const base::FilePath& file_path,
    bool upload_successful) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_task_sequence_checker_);

  // Post a task to deallocate the uploader (can't do this directly,
  // because this function is a callback from the uploader), potentially
  // starting a new upload for the next file.
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(
          &WebRtcRemoteEventLogManager::OnWebRtcEventLogUploadCompleteInternal,
          base::Unretained(this)));
}

void WebRtcRemoteEventLogManager::SetWebRtcEventLogUploaderFactoryForTesting(
    std::unique_ptr<WebRtcEventLogUploader::Factory> uploader_factory) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_task_sequence_checker_);

  uploader_factory_ = std::move(uploader_factory);

  // Unit tests would initially set a null uploader factory, so that files would
  // be kept around. Some tests would later change to a different factory
  // (e.g. one that always simulates upload failure); in that case, we should
  // get rid of the null uploader, since it never terminates.
  uploader_.reset();
  MaybeStartUploading();
}

base::FilePath WebRtcRemoteEventLogManager::GetLogsDirectoryPath(
    const base::FilePath& browser_context_dir) {
  return browser_context_dir.Append(kRemoteBoundLogSubDirectory);
}

bool WebRtcRemoteEventLogManager::BrowserContextEnabled(
    BrowserContextId browser_context_id) const {
  const auto it = enabled_browser_contexts_.find(browser_context_id);
  return it != enabled_browser_contexts_.cend();
}

WebRtcRemoteEventLogManager::LogFilesMap::iterator
WebRtcRemoteEventLogManager::CloseLogFile(LogFilesMap::iterator it) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_task_sequence_checker_);

  const PeerConnectionKey peer_connection = it->first;

  it->second.file.Flush();
  it = active_logs_.erase(it);  // file.Close() called by destructor.

  if (observer_) {
    observer_->OnRemoteLogStopped(peer_connection);
  }

  MaybeStartUploading();

  return it;
}

bool WebRtcRemoteEventLogManager::MaybeCreateLogsDirectory(
    const base::FilePath& remote_bound_logs_dir) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_task_sequence_checker_);

  if (base::PathExists(remote_bound_logs_dir)) {
    if (!base::DirectoryExists(remote_bound_logs_dir)) {
      LOG(ERROR) << "Path for remote-bound logs is taken by a non-directory.";
      return false;
    }
  } else if (!base::CreateDirectory(remote_bound_logs_dir)) {
    LOG(ERROR) << "Failed to create the local directory for remote-bound logs.";
    return false;
  }

  // TODO(crbug.com/775415): Test for appropriate permissions.

  return true;
}

void WebRtcRemoteEventLogManager::AddPendingLogs(
    BrowserContextId browser_context_id,
    const base::FilePath& remote_bound_logs_dir) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_task_sequence_checker_);

  base::FilePath::StringType pattern =
      base::FilePath::StringType(FILE_PATH_LITERAL("*")) +
      base::FilePath::kExtensionSeparator + kRemoteBoundLogExtension;
  base::FileEnumerator enumerator(remote_bound_logs_dir,
                                  /*recursive=*/false,
                                  base::FileEnumerator::FILES, pattern);

  for (auto path = enumerator.Next(); !path.empty(); path = enumerator.Next()) {
    const auto last_modified = enumerator.GetInfo().GetLastModifiedTime();
    auto it = pending_logs_.emplace(browser_context_id, path, last_modified);
    DCHECK(it.second);  // No pre-existing entry.
  }

  MaybeStartUploading();
}

bool WebRtcRemoteEventLogManager::StartWritingLog(
    const PeerConnectionKey& key,
    const base::FilePath& browser_context_dir,
    size_t max_file_size_bytes,
    const std::string& metadata,
    std::string* error_message) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_task_sequence_checker_);

  // WriteAtCurrentPos() only allows writing up to max-int at a time. We could
  // iterate to do more, but we don't expect to ever need to, so it's easier
  // to disallow it.
  if (metadata.length() >
      static_cast<size_t>(std::numeric_limits<int>::max())) {
    LOG(WARNING) << "Metadata too long to be written in one go.";
    *error_message = kStartRemoteLoggingFailureMetadaTooLong;
    return false;
  }

  // Randomize a new filename. In the highly unlikely event that this filename
  // is already taken, it will be treated the same way as any other failure
  // to start the log file.
  // TODO(crbug.com/775415): Add a unit test for above comment.
  const std::string unique_filename =
      "event_log_" + std::to_string(base::RandUint64());
  const base::FilePath base_path = GetLogsDirectoryPath(browser_context_dir);
  const base::FilePath file_path = base_path.AppendASCII(unique_filename)
                                       .AddExtension(kRemoteBoundLogExtension);

  // Attempt to create the file.
  constexpr int file_flags = base::File::FLAG_CREATE | base::File::FLAG_WRITE |
                             base::File::FLAG_EXCLUSIVE_WRITE;
  base::File file(file_path, file_flags);
  if (!file.IsValid() || !file.created()) {
    LOG(WARNING) << "Couldn't create and/or open remote WebRTC event log file.";
    // Intentionally using a generic error; look for other places where it's
    // set for an explanation why.
    *error_message = kStartRemoteLoggingFailureGeneric;
    return false;
  }

  const uint32_t header_host_order =
      static_cast<uint32_t>(metadata.length()) |
      (kRemoteBoundWebRtcEventLogFileVersion << 24);
  static_assert(kRemoteBoundLogFileHeaderSizeBytes == sizeof(uint32_t),
                "Restructure this otherwise.");
  char header[sizeof(uint32_t)];
  base::WriteBigEndian<uint32_t>(header, header_host_order);
  int written = file.WriteAtCurrentPos(header, sizeof(header));
  if (written != arraysize(header)) {
    LOG(WARNING) << "Failed to write header to log file.";
    DiscardLogFile(&file, file_path);
    // Intentionally using a generic error; look for other places where it's
    // set for an explanation why.
    *error_message = kStartRemoteLoggingFailureGeneric;
    return false;
  }

  const int metadata_length = static_cast<int>(metadata.length());
  written = file.WriteAtCurrentPos(metadata.c_str(), metadata_length);
  if (written != metadata_length) {
    LOG(WARNING) << "Failed to write metadata to log file.";
    DiscardLogFile(&file, file_path);
    // Intentionally using a generic error; look for other places where it's
    // set for an explanation why.
    *error_message = kStartRemoteLoggingFailureGeneric;
    return false;
  }

  // Record that we're now writing this remote-bound log to this file.
  const size_t header_and_metadata_size_bytes =
      kRemoteBoundLogFileHeaderSizeBytes + metadata_length;
  const auto it = active_logs_.emplace(
      key, LogFile(file_path, std::move(file), max_file_size_bytes,
                   header_and_metadata_size_bytes));
  DCHECK(it.second);

  observer_->OnRemoteLogStarted(key, file_path);

  return true;
}

void WebRtcRemoteEventLogManager::MaybeStopRemoteLogging(
    const PeerConnectionKey& key) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_task_sequence_checker_);

  const auto it = active_logs_.find(key);
  if (it == active_logs_.end()) {
    return;
  }

  it->second.file.Flush();
  it->second.file.Close();

  // The current time is a good enough approximation of the file's last
  // modification time.
  const base::Time last_modified = base::Time::Now();

  // The stopped log becomes a pending log. It is no longer an active log.
  const auto emplace_result = pending_logs_.emplace(
      key.browser_context_id, it->second.path, last_modified);
  DCHECK(emplace_result.second);  // No pre-existing entry.
  active_logs_.erase(it);

  observer_->OnRemoteLogStopped(key);

  MaybeStartUploading();
}

void WebRtcRemoteEventLogManager::PrunePendingLogs() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_task_sequence_checker_);
  RemovePendingLogs(
      base::Time::Min(),
      base::Time::Now() - kRemoteBoundWebRtcEventLogsMaxRetention);
}

void WebRtcRemoteEventLogManager::RecurringPendingLogsPrune() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_task_sequence_checker_);
  DCHECK(proactive_prune_scheduling_delta_.has_value());
  DCHECK_GT(*proactive_prune_scheduling_delta_, base::TimeDelta());
  DCHECK(proactive_prune_scheduling_started_);

  PrunePendingLogs();

  base::SequencedTaskRunnerHandle::Get()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&WebRtcRemoteEventLogManager::RecurringPendingLogsPrune,
                     base::Unretained(this)),
      *proactive_prune_scheduling_delta_);
}

void WebRtcRemoteEventLogManager::RemovePendingLogs(
    const base::Time& delete_begin,
    const base::Time& delete_end,
    base::Optional<BrowserContextId> browser_context_id) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_task_sequence_checker_);
  for (auto it = pending_logs_.begin(); it != pending_logs_.end();) {
    const bool relevant_browser_content =
        !browser_context_id || it->browser_context_id == browser_context_id;
    if (relevant_browser_content &&
        (delete_begin.is_null() || delete_begin <= it->last_modified) &&
        (delete_end.is_null() || it->last_modified < delete_end)) {
      DVLOG(1) << "Removing " << it->path << ".";
      if (!base::DeleteFile(it->path, /*recursive=*/false)) {
        LOG(ERROR) << "Failed to delete " << it->path << ".";
      }
      it = pending_logs_.erase(it);
    } else {
      DVLOG(1) << "Keeping " << it->path << " on disk.";
      ++it;
    }
  }
}

bool WebRtcRemoteEventLogManager::AdditionalActiveLogAllowed(
    BrowserContextId browser_context_id) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_task_sequence_checker_);

  // Limit over concurrently active logs (across BrowserContext-s).
  if (active_logs_.size() >= kMaxActiveRemoteBoundWebRtcEventLogs) {
    return false;
  }

  // Limit over the number of pending logs (per BrowserContext). We count active
  // logs too, since they become pending logs once completed.
  const size_t active_count = std::count_if(
      active_logs_.begin(), active_logs_.end(),
      [browser_context_id](const decltype(active_logs_)::value_type& log) {
        return log.first.browser_context_id == browser_context_id;
      });
  const size_t pending_count = std::count_if(
      pending_logs_.begin(), pending_logs_.end(),
      [browser_context_id](const decltype(pending_logs_)::value_type& log) {
        return log.browser_context_id == browser_context_id;
      });
  return active_count + pending_count < kMaxPendingRemoteBoundWebRtcEventLogs;
}

bool WebRtcRemoteEventLogManager::UploadingAllowed() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_task_sequence_checker_);
  return upload_suppression_disabled_ || active_peer_connections_.empty();
}

void WebRtcRemoteEventLogManager::MaybeStartUploading() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_task_sequence_checker_);

  PrunePendingLogs();  // Avoid uploading freshly expired files.

  if (uploader_) {
    return;  // Upload already underway.
  }

  if (pending_logs_.empty()) {
    return;  // Nothing to upload.
  }

  if (!UploadingAllowed()) {
    return;
  }

  // The uploader takes ownership of the file; it's no longer considered to be
  // pending. (If the upload fails, the log will be deleted.)
  // TODO(crbug.com/775415): Add more refined retry behavior, so that we would
  // not delete the log permanently if the network is just down, on the one
  // hand, but also would not be uploading unlimited data on endless retries on
  // the other hand.
  // TODO(crbug.com/814362): Delay the upload's start.
  // TODO(crbug.com/775415): Rename the file before uploading, so that we would
  // not retry the upload after restarting Chrome, if the upload is interrupted.
  uploader_ = uploader_factory_->Create(pending_logs_.begin()->path, this);
  pending_logs_.erase(pending_logs_.begin());
}

void WebRtcRemoteEventLogManager::OnWebRtcEventLogUploadCompleteInternal() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_task_sequence_checker_);
  uploader_.reset();
  MaybeStartUploading();
}

bool WebRtcRemoteEventLogManager::FindPeerConnection(
    int render_process_id,
    const std::string& peer_connection_id,
    PeerConnectionKey* key) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_task_sequence_checker_);

  const auto it = FindNextPeerConnection(active_peer_connections_.cbegin(),
                                         render_process_id, peer_connection_id);
  if (it == active_peer_connections_.cend()) {
    return false;
  }

  // Make sure that the peer connection ID is unique for the renderer process
  // in which it exists, though not necessarily between renderer processes.
  // (The helper exists just to allow this DCHECK.)
  DCHECK(FindNextPeerConnection(std::next(it), render_process_id,
                                peer_connection_id) ==
         active_peer_connections_.cend());

  *key = it->first;
  return true;
}

std::map<WebRtcEventLogPeerConnectionKey, const std::string>::const_iterator
WebRtcRemoteEventLogManager::FindNextPeerConnection(
    std::map<PeerConnectionKey, const std::string>::const_iterator begin,
    int render_process_id,
    const std::string& peer_connection_id) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(io_task_sequence_checker_);
  const auto end = active_peer_connections_.cend();
  for (auto it = begin; it != end; it++) {
    if (it->first.render_process_id == render_process_id &&
        it->second == peer_connection_id) {
      return it;
    }
  }
  return end;
}
