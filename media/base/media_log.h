// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_MEDIA_LOG_H_
#define MEDIA_BASE_MEDIA_LOG_H_

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <sstream>
#include <string>

#include "base/logging.h"
#include "base/macros.h"
#include "media/base/buffering_state.h"
#include "media/base/media_export.h"
#include "media/base/media_log_event.h"
#include "media/base/pipeline_impl.h"
#include "media/base/pipeline_status.h"
#include "url/gurl.h"

namespace media {

// Interface for media components to log to chrome://media-internals log.
//
// Implementations only need to implement AddEvent(), which must be thread-safe.
// AddEvent() is expected to be called from multiple threads.
class MEDIA_EXPORT MediaLog {
 public:
  enum MediaLogLevel {
    // Fatal error, e.g. cause of playback failure. Since this is also used to
    // form MediaError.message, do NOT use this for non-fatal errors to avoid
    // contaminating MediaError.message.
    MEDIALOG_ERROR,

    // Warning about non-fatal issues, e.g. quality of playback issues such as
    // audio/video out of sync.
    MEDIALOG_WARNING,

    // General info useful for Chromium and/or web developers, testers and even
    // users, e.g. audio/video codecs used in a playback instance.
    MEDIALOG_INFO,

    // Misc debug info for Chromium developers.
    MEDIALOG_DEBUG,
  };

  // Convert various enums to strings.
  static std::string MediaLogLevelToString(MediaLogLevel level);
  static MediaLogEvent::Type MediaLogLevelToEventType(MediaLogLevel level);
  static std::string EventTypeToString(MediaLogEvent::Type type);

  // Returns a string version of the status, unique to each PipelineStatus, and
  // not including any ':'. This makes it suitable for usage in
  // MediaError.message as the UA-specific-error-code.
  static std::string PipelineStatusToString(PipelineStatus status);

  static std::string BufferingStateToString(BufferingState state);

  static std::string MediaEventToLogString(const MediaLogEvent& event);

  // Returns a string usable as part of a MediaError.message, for only
  // PIPELINE_ERROR or MEDIA_ERROR_LOG_ENTRY events, with any newlines replaced
  // with whitespace in the latter kind of events.
  static std::string MediaEventToMessageString(const MediaLogEvent& event);

  MediaLog();
  virtual ~MediaLog();

  // Add an event to this log. Overriden by inheritors to actually do something
  // with it.
  virtual void AddEvent(std::unique_ptr<MediaLogEvent> event);

  // Returns a string usable as the contents of a MediaError.message.
  // This method returns an incomplete message if it is called before the
  // pertinent events for the error have been added to the log.
  // Note: The base class definition only produces empty messages. See
  // RenderMediaLog for where this method is meaningful.
  virtual std::string GetErrorMessage();

  // Records the domain and registry of the current frame security origin to a
  // Rappor privacy-preserving metric. See:
  //   https://www.chromium.org/developers/design-documents/rappor
  virtual void RecordRapporWithSecurityOrigin(const std::string& metric);

  // Helper methods to create events and their parameters.
  std::unique_ptr<MediaLogEvent> CreateEvent(MediaLogEvent::Type type);
  std::unique_ptr<MediaLogEvent> CreateBooleanEvent(MediaLogEvent::Type type,
                                                    const std::string& property,
                                                    bool value);
  std::unique_ptr<MediaLogEvent> CreateCreatedEvent(
      const std::string& origin_url);
  std::unique_ptr<MediaLogEvent> CreateStringEvent(MediaLogEvent::Type type,
                                                   const std::string& property,
                                                   const std::string& value);
  std::unique_ptr<MediaLogEvent> CreateTimeEvent(MediaLogEvent::Type type,
                                                 const std::string& property,
                                                 base::TimeDelta value);
  std::unique_ptr<MediaLogEvent> CreateLoadEvent(const std::string& url);
  std::unique_ptr<MediaLogEvent> CreateSeekEvent(double seconds);
  std::unique_ptr<MediaLogEvent> CreatePipelineStateChangedEvent(
      PipelineImpl::State state);
  std::unique_ptr<MediaLogEvent> CreatePipelineErrorEvent(PipelineStatus error);
  std::unique_ptr<MediaLogEvent> CreateVideoSizeSetEvent(size_t width,
                                                         size_t height);
  std::unique_ptr<MediaLogEvent> CreateBufferingStateChangedEvent(
      const std::string& property,
      BufferingState state);

  // Report a log message at the specified log level.
  void AddLogEvent(MediaLogLevel level, const std::string& message);

  // Report a property change without an accompanying event.
  void SetStringProperty(const std::string& key, const std::string& value);
  void SetDoubleProperty(const std::string& key, double value);
  void SetBooleanProperty(const std::string& key, bool value);

  // Getter for |id_|. Used by MojoMediaLogService to construct MediaLogEvents
  // to log into this MediaLog. Also used in trace events to associate each
  // event with a specific media playback.
  int32_t id() const { return id_; }

 private:
  // A unique (to this process) id for this MediaLog.
  int32_t id_;

  DISALLOW_COPY_AND_ASSIGN(MediaLog);
};

// Helper class to make it easier to use MediaLog like DVLOG().
class MEDIA_EXPORT LogHelper {
 public:
  LogHelper(MediaLog::MediaLogLevel level, MediaLog* media_log);
  ~LogHelper();

  std::ostream& stream() { return stream_; }

 private:
  const MediaLog::MediaLogLevel level_;
  MediaLog* const media_log_;
  std::stringstream stream_;
};

// Provides a stringstream to collect a log entry to pass to the provided
// MediaLog at the requested level.
#define MEDIA_LOG(level, media_log) \
  LogHelper((MediaLog::MEDIALOG_##level), (media_log)).stream()

// Logs only while |count| < |max|, increments |count| for each log, and warns
// in the log if |count| has just reached |max|.
// Multiple short-circuit evaluations are involved in this macro:
// 1) LAZY_STREAM avoids wasteful MEDIA_LOG and evaluation of subsequent stream
//    arguments if |count| is >= |max|, and
// 2) the |condition| given to LAZY_STREAM itself short-circuits to prevent
//    incrementing |count| beyond |max|.
// Note that LAZY_STREAM guarantees exactly one evaluation of |condition|, so
// |count| will be incremented at most once each time this macro runs.
// The "|| true" portion of |condition| lets logging occur correctly when
// |count| < |max| and |count|++ is 0.
// TODO(wolenetz,chcunningham): Consider using a helper class instead of a macro
// to improve readability.
#define LIMITED_MEDIA_LOG(level, media_log, count, max)                       \
  LAZY_STREAM(MEDIA_LOG(level, media_log),                                    \
              (count) < (max) && ((count)++ || true))                         \
      << (((count) == (max)) ? "(Log limit reached. Further similar entries " \
                               "may be suppressed): "                         \
                             : "")

}  // namespace media

#endif  // MEDIA_BASE_MEDIA_LOG_H_
