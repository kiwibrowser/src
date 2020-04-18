// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_WEBRTC_WEBRTC_EVENT_LOG_MANAGER_COMMON_H_
#define CHROME_BROWSER_MEDIA_WEBRTC_WEBRTC_EVENT_LOG_MANAGER_COMMON_H_

#include <map>
#include <string>
#include <tuple>

#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/time/time.h"
#include "build/build_config.h"

// This file is intended for:
// 1. Code shared between WebRtcEventLogManager, WebRtcLocalEventLogManager
//    and WebRtcRemoteEventLogManager.
// 2. Code specific to either of the above classes, but which also needs
//    to be seen by unit tests (such as constants).

extern const size_t kWebRtcEventLogManagerUnlimitedFileSize;

extern const size_t kDefaultMaxLocalLogFileSizeBytes;
extern const size_t kMaxNumberLocalWebRtcEventLogFiles;

extern const size_t kMaxRemoteLogFileMetadataSizeBytes;
extern const size_t kMaxRemoteLogFileSizeBytes;

// Limit over the number of concurrently active (currently being written to
// disk) remote-bound log files. This limits IO operations, and so it is
// applied globally (all browser contexts are limited together).
extern const size_t kMaxActiveRemoteBoundWebRtcEventLogs;

// Limit over the number of pending logs (logs stored on disk and awaiting to
// be uploaded to a remote server). This limit avoids excessive storage. If a
// user chooses to have multiple profiles (and hence browser contexts) on a
// system, it is assumed that the user has enough storage to accommodate
// the increased storage consumption that comes with it. Therefore, this
// limit is applied per browser context.
extern const size_t kMaxPendingRemoteBoundWebRtcEventLogs;

// The file extension to be associated with remote-bound logs while they are
// kept on local disk.
extern const base::FilePath::CharType kRemoteBoundLogExtension[];

// Version of the remote-bound log. Refers to the version of the event logs'
// encoding, method for separation of metadata from the WebRTC event log, etc.
extern const uint8_t kRemoteBoundWebRtcEventLogFileVersion;

// Remote-bound log headers are composed of:
// * One byte for the version (for the encoding, metadata format, etc.)
// * Three bytes encoding the length of the metadata, in bytes.
// The metadata, which immediately follows the header, is not counted as part
// of the header size.
extern const size_t kRemoteBoundLogFileHeaderSizeBytes;

// Remote-bound event logs will not be uploaded if the time since their last
// modification (meaning the time when they were completed) exceeds this value.
// Such expired files will be purged from disk when examined.
extern const base::TimeDelta kRemoteBoundWebRtcEventLogsMaxRetention;

// StartRemoteLogging could fail for several reasons, but we only report
// individually those failures that relate to either bad parameters, or calls
// at a time that makes no sense. Anything else  would leak information to
// the JS application (too many pending logs, etc.), and is not actionable
// anyhow.
// These are made globally visible so that unit tests may check for them.
extern const char kStartRemoteLoggingFailureFeatureDisabled[];
extern const char kStartRemoteLoggingFailureUnlimitedSizeDisallowed[];
extern const char kStartRemoteLoggingFailureMaxSizeTooLarge[];
extern const char kStartRemoteLoggingFailureMetadaTooLong[];
extern const char kStartRemoteLoggingFailureMaxSizeTooSmall[];
extern const char kStartRemoteLoggingFailureUnknownOrInactivePeerConnection[];
extern const char kStartRemoteLoggingFailureAlreadyLogging[];
extern const char kStartRemoteLoggingFailureGeneric[];

// For a given Chrome session, this is a unique key for PeerConnections.
// It's not, however, unique between sessions (after Chrome is restarted).
struct WebRtcEventLogPeerConnectionKey {
  using BrowserContextId = uintptr_t;

  constexpr WebRtcEventLogPeerConnectionKey()
      : WebRtcEventLogPeerConnectionKey(
            /* render_process_id = */ 0,
            /* lid = */ 0,
            reinterpret_cast<BrowserContextId>(nullptr)) {}

  constexpr WebRtcEventLogPeerConnectionKey(int render_process_id,
                                            int lid,
                                            BrowserContextId browser_context_id)
      : render_process_id(render_process_id),
        lid(lid),
        browser_context_id(browser_context_id) {}

  bool operator==(const WebRtcEventLogPeerConnectionKey& other) const {
    // Each RPH is associated with exactly one BrowserContext.
    DCHECK(render_process_id != other.render_process_id ||
           browser_context_id == other.browser_context_id);

    const bool equal = std::tie(render_process_id, lid) ==
                       std::tie(other.render_process_id, other.lid);
    return equal;
  }

  bool operator<(const WebRtcEventLogPeerConnectionKey& other) const {
    // Each RPH is associated with exactly one BrowserContext.
    DCHECK(render_process_id != other.render_process_id ||
           browser_context_id == other.browser_context_id);

    return std::tie(render_process_id, lid) <
           std::tie(other.render_process_id, other.lid);
  }

  // These two fields are the actual key; any peer connection is uniquely
  // identifiable by the renderer process in which it lives, and its ID within
  // that process.
  int render_process_id;
  int lid;  // Renderer-local PeerConnection ID.

  // The BrowserContext is not actually part of the key, but each PeerConnection
  // is associated with a BrowserContext, and that BrowserContext is almost
  // always necessary, so it makes sense to remember it along with the key.
  BrowserContextId browser_context_id;
};

// An observer for notifications of local log files being started/stopped, and
// the paths which will be used for these logs.
class WebRtcLocalEventLogsObserver {
 public:
  virtual void OnLocalLogStarted(WebRtcEventLogPeerConnectionKey key,
                                 const base::FilePath& file_path) = 0;
  virtual void OnLocalLogStopped(WebRtcEventLogPeerConnectionKey key) = 0;

 protected:
  virtual ~WebRtcLocalEventLogsObserver() = default;
};

// An observer for notifications of remote-bound log files being
// started/stopped. The start event would likely only interest unit tests
// (because it exposes the randomized filename to them). The stop event is of
// general interest, because it would often mean that WebRTC can stop sending
// us event logs for this peer connection.
// Some cases where OnRemoteLogStopped would be called include:
// 1. The PeerConnection has become inactive.
// 2. The file's maximum size has been reached.
// 3. Any type of error while writing to the file.
class WebRtcRemoteEventLogsObserver {
 public:
  virtual void OnRemoteLogStarted(WebRtcEventLogPeerConnectionKey key,
                                  const base::FilePath& file_path) = 0;
  virtual void OnRemoteLogStopped(WebRtcEventLogPeerConnectionKey key) = 0;

 protected:
  virtual ~WebRtcRemoteEventLogsObserver() = default;
};

struct LogFile {
  LogFile(const base::FilePath& path,
          base::File file,
          size_t max_file_size_bytes,
          size_t file_size_bytes = 0)
      : path(path),
        file(std::move(file)),
        max_file_size_bytes(max_file_size_bytes),
        file_size_bytes(file_size_bytes) {}
  const base::FilePath path;
  base::File file;
  const size_t max_file_size_bytes;
  size_t file_size_bytes;
};

// WebRtcLocalEventLogManager and WebRtcRemoteEventLogManager share some logic
// when it comes to handling of files on disk.
class LogFileWriter {
 protected:
  using PeerConnectionKey = WebRtcEventLogPeerConnectionKey;
  using BrowserContextId = PeerConnectionKey::BrowserContextId;
  using LogFilesMap = std::map<PeerConnectionKey, LogFile>;

  virtual ~LogFileWriter() = default;

  // Given a peer connection and its associated log file, and given a log
  // fragment that should be written to the log file, attempt to write to
  // the log file (return value indicates success/failure).
  // If an error occurs, or if the file reaches its capacity, CloseLogFile()
  // will be called, closing the file.
  bool WriteToLogFile(LogFilesMap::iterator it, const std::string& message);

  // Called when WriteToLogFile() either encounters an error, or if the file's
  // intended capacity is reached. It indicates to the inheriting class that
  // the file should also be purged from its set of active log files.
  // The function should return an iterator to the next element in the set
  // of active logs. This makes the function more useful, allowing it to be
  // used when iterating and closing several log files.
  virtual LogFilesMap::iterator CloseLogFile(LogFilesMap::iterator it) = 0;
};

#endif  // CHROME_BROWSER_MEDIA_WEBRTC_WEBRTC_EVENT_LOG_MANAGER_COMMON_H_
