// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_METRICS_LOG_STORE_H_
#define COMPONENTS_METRICS_LOG_STORE_H_

#include <string>

namespace metrics {

// Interface for local storage of serialized logs to be reported.
// It allows consumers to check if there are logs to consume, consume them one
// at a time by staging and discarding logs, and persist/load the whole set.
class LogStore {
 public:
  // Returns true if there are any logs waiting to be uploaded.
  virtual bool has_unsent_logs() const = 0;

  // Returns true if there is a log that needs to be, or is being, uploaded.
  virtual bool has_staged_log() const = 0;

  // The text of the staged log, as a serialized protobuf.
  // Will trigger a DCHECK if there is no staged log.
  virtual const std::string& staged_log() const = 0;

  // The SHA1 hash of the staged log.
  // Will trigger a DCHECK if there is no staged log.
  virtual const std::string& staged_log_hash() const = 0;

  // Populates staged_log() with the next stored log to send.
  // The order in which logs are staged is up to the implementor.
  // The staged_log must remain the same even if additional logs are added.
  // Should only be called if has_unsent_logs() is true.
  virtual void StageNextLog() = 0;

  // Discards the staged log.
  virtual void DiscardStagedLog() = 0;

  // Saves any unsent logs to persistent storage.
  virtual void PersistUnsentLogs() const = 0;

  // Loads unsent logs from persistent storage.
  virtual void LoadPersistedUnsentLogs() = 0;
};

}  // namespace metrics

#endif  // COMPONENTS_METRICS_LOG_STORE_H_
