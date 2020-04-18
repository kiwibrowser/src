// Copyright (c) 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/webrtc/webrtc_event_log_manager_common.h"

#include <limits>

const char kStartRemoteLoggingFailureFeatureDisabled[] = "Feature disabled.";
const char kStartRemoteLoggingFailureUnlimitedSizeDisallowed[] =
    "Unlimited size disallowed.";
const char kStartRemoteLoggingFailureMaxSizeTooLarge[] =
    "Excessively large max log size.";
const char kStartRemoteLoggingFailureMetadaTooLong[] =
    "Excessively long metadata.";
const char kStartRemoteLoggingFailureMaxSizeTooSmall[] = "Max size too small.";
const char kStartRemoteLoggingFailureUnknownOrInactivePeerConnection[] =
    "Unknown or inactive peer connection.";
const char kStartRemoteLoggingFailureAlreadyLogging[] = "Already logging.";
const char kStartRemoteLoggingFailureGeneric[] = "Unspecified error.";

bool LogFileWriter::WriteToLogFile(LogFilesMap::iterator it,
                                   const std::string& message) {
  DCHECK_LE(message.length(),
            static_cast<size_t>(std::numeric_limits<int>::max()));

  // Observe the file size limit, if any. Note that base::File's interface does
  // not allow writing more than numeric_limits<int>::max() bytes at a time.
  int message_len = static_cast<int>(message.length());  // DCHECKed above.
  LogFile& log_file = it->second;
  if (log_file.max_file_size_bytes != kWebRtcEventLogManagerUnlimitedFileSize) {
    DCHECK_LT(log_file.file_size_bytes, log_file.max_file_size_bytes);
    const bool size_will_wrap_around =
        log_file.file_size_bytes + message.length() < log_file.file_size_bytes;
    const bool size_limit_will_be_exceeded =
        log_file.file_size_bytes + message.length() >
        log_file.max_file_size_bytes;
    if (size_will_wrap_around || size_limit_will_be_exceeded) {
      CloseLogFile(it);
      return false;
    }
  }

  int written = log_file.file.WriteAtCurrentPos(message.c_str(), message_len);
  if (written != message_len) {
    LOG(WARNING) << "WebRTC event log message couldn't be written to the "
                    "locally stored file in its entirety.";
    CloseLogFile(it);
    return false;
  }

  log_file.file_size_bytes += static_cast<size_t>(written);
  if (log_file.max_file_size_bytes != kWebRtcEventLogManagerUnlimitedFileSize) {
    DCHECK_LE(log_file.file_size_bytes, log_file.max_file_size_bytes);
    if (log_file.file_size_bytes >= log_file.max_file_size_bytes) {
      CloseLogFile(it);
    }
  }

  return (static_cast<size_t>(written) == message.length());
}
