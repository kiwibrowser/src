/*
 *  Copyright 2004 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

// RTC_LOG(...) an ostream target that can be used to send formatted
// output to a variety of logging targets, such as debugger console, stderr,
// or any LogSink.
// The severity level passed as the first argument to the logging
// functions is used as a filter, to limit the verbosity of the logging.
// Static members of LogMessage documented below are used to control the
// verbosity and target of the output.
// There are several variations on the RTC_LOG macro which facilitate logging
// of common error conditions, detailed below.

// RTC_LOG(sev) logs the given stream at severity "sev", which must be a
//     compile-time constant of the LoggingSeverity type, without the namespace
//     prefix.
// RTC_LOG_V(sev) Like RTC_LOG(), but sev is a run-time variable of the
//     LoggingSeverity type (basically, it just doesn't prepend the namespace).
// RTC_LOG_F(sev) Like RTC_LOG(), but includes the name of the current function.
// RTC_LOG_T(sev) Like RTC_LOG(), but includes the this pointer.
// RTC_LOG_T_F(sev) Like RTC_LOG_F(), but includes the this pointer.
// RTC_LOG_GLE(sev [, mod]) attempt to add a string description of the
//     HRESULT returned by GetLastError.
// RTC_LOG_ERRNO(sev) attempts to add a string description of an errno-derived
//     error. errno and associated facilities exist on both Windows and POSIX,
//     but on Windows they only apply to the C/C++ runtime.
// RTC_LOG_ERR(sev) is an alias for the platform's normal error system, i.e.
//     _GLE on Windows and _ERRNO on POSIX.
// (The above three also all have _EX versions that let you specify the error
// code, rather than using the last one.)
// RTC_LOG_E(sev, ctx, err, ...) logs a detailed error interpreted using the
//     specified context.
// RTC_LOG_CHECK_LEVEL(sev) (and RTC_LOG_CHECK_LEVEL_V(sev)) can be used as a
//     test before performing expensive or sensitive operations whose sole
//     purpose is to output logging data at the desired level.

#ifndef RTC_BASE_LOGGING_H_
#define RTC_BASE_LOGGING_H_

#include <errno.h>

#include <list>
#include <sstream>
#include <string>
#include <utility>

#if defined(WEBRTC_MAC) && !defined(WEBRTC_IOS)
#include <CoreServices/CoreServices.h>
#endif

#include "rtc_base/constructormagic.h"
#include "rtc_base/deprecation.h"
#include "rtc_base/system/no_inline.h"
#include "rtc_base/thread_annotations.h"

#if !defined(NDEBUG) || defined(DLOG_ALWAYS_ON)
#define RTC_DLOG_IS_ON 1
#else
#define RTC_DLOG_IS_ON 0
#endif

namespace rtc {

#if defined(WEBRTC_MAC) && !defined(WEBRTC_IOS)
// Returns a UTF8 description from an OS X Status error.
std::string DescriptionFromOSStatus(OSStatus err);
#endif

//////////////////////////////////////////////////////////////////////

// Note that the non-standard LoggingSeverity aliases exist because they are
// still in broad use.  The meanings of the levels are:
//  LS_SENSITIVE: Information which should only be logged with the consent
//   of the user, due to privacy concerns.
//  LS_VERBOSE: This level is for data which we do not want to appear in the
//   normal debug log, but should appear in diagnostic logs.
//  LS_INFO: Chatty level used in debugging for all sorts of things, the default
//   in debug builds.
//  LS_WARNING: Something that may warrant investigation.
//  LS_ERROR: Something that should not have occurred.
//  LS_NONE: Don't log.
enum LoggingSeverity {
  LS_SENSITIVE,
  LS_VERBOSE,
  LS_INFO,
  LS_WARNING,
  LS_ERROR,
  LS_NONE,
  INFO = LS_INFO,
  WARNING = LS_WARNING,
  LERROR = LS_ERROR
};

// LogErrorContext assists in interpreting the meaning of an error value.
enum LogErrorContext {
  ERRCTX_NONE,
  ERRCTX_ERRNO,     // System-local errno
  ERRCTX_HRESULT,   // Windows HRESULT
  ERRCTX_OSSTATUS,  // MacOS OSStatus

  // Abbreviations for LOG_E macro
  ERRCTX_EN = ERRCTX_ERRNO,     // LOG_E(sev, EN, x)
  ERRCTX_HR = ERRCTX_HRESULT,   // LOG_E(sev, HR, x)
  ERRCTX_OS = ERRCTX_OSSTATUS,  // LOG_E(sev, OS, x)
};

// Virtual sink interface that can receive log messages.
class LogSink {
 public:
  LogSink() {}
  virtual ~LogSink() {}
  virtual void OnLogMessage(const std::string& msg,
                            LoggingSeverity severity,
                            const char* tag);
  virtual void OnLogMessage(const std::string& message) = 0;
};

class LogMessage {
 public:
  LogMessage(const char* file, int line, LoggingSeverity sev);

  // Same as the above, but using a compile-time constant for the logging
  // severity. This saves space at the call site, since passing an empty struct
  // is generally the same as not passing an argument at all.
  template <LoggingSeverity S>
  RTC_NO_INLINE LogMessage(const char* file,
                           int line,
                           std::integral_constant<LoggingSeverity, S>)
      : LogMessage(file, line, S) {}

  LogMessage(const char* file,
             int line,
             LoggingSeverity sev,
             LogErrorContext err_ctx,
             int err);

#if defined(WEBRTC_ANDROID)
  LogMessage(const char* file, int line, LoggingSeverity sev, const char* tag);
#endif

  // DEPRECATED - DO NOT USE - PLEASE USE THE MACROS INSTEAD OF THE CLASS.
  // Android code should use the 'const char*' version since tags are static
  // and we want to avoid allocating a std::string copy per log line.
  RTC_DEPRECATED
  LogMessage(const char* file, int line, LoggingSeverity sev,
             const std::string& tag);

  ~LogMessage();

  static bool Loggable(LoggingSeverity sev);

  // Same as the above, but using a template argument instead of a function
  // argument. (When the logging severity is statically known, passing it as a
  // template argument instead of as a function argument saves space at the
  // call site.)
  template <LoggingSeverity S>
  RTC_NO_INLINE static bool Loggable() {
    return Loggable(S);
  }

  std::ostream& stream();

  // Returns the time at which this function was called for the first time.
  // The time will be used as the logging start time.
  // If this is not called externally, the LogMessage ctor also calls it, in
  // which case the logging start time will be the time of the first LogMessage
  // instance is created.
  static int64_t LogStartTime();

  // Returns the wall clock equivalent of |LogStartTime|, in seconds from the
  // epoch.
  static uint32_t WallClockStartTime();

  //  LogThreads: Display the thread identifier of the current thread
  static void LogThreads(bool on = true);

  //  LogTimestamps: Display the elapsed time of the program
  static void LogTimestamps(bool on = true);

  // These are the available logging channels
  //  Debug: Debug console on Windows, otherwise stderr
  static void LogToDebug(LoggingSeverity min_sev);
  static LoggingSeverity GetLogToDebug();

  // Sets whether logs will be directed to stderr in debug mode.
  static void SetLogToStderr(bool log_to_stderr);

  //  Stream: Any non-blocking stream interface.  LogMessage takes ownership of
  //   the stream. Multiple streams may be specified by using AddLogToStream.
  //   LogToStream is retained for backwards compatibility; when invoked, it
  //   will discard any previously set streams and install the specified stream.
  //   GetLogToStream gets the severity for the specified stream, of if none
  //   is specified, the minimum stream severity.
  //   RemoveLogToStream removes the specified stream, without destroying it.
  static int GetLogToStream(LogSink* stream = nullptr);
  static void AddLogToStream(LogSink* stream, LoggingSeverity min_sev);
  static void RemoveLogToStream(LogSink* stream);

  // Testing against MinLogSeverity allows code to avoid potentially expensive
  // logging operations by pre-checking the logging level.
  static int GetMinLogSeverity();

  // Parses the provided parameter stream to configure the options above.
  // Useful for configuring logging from the command line.
  static void ConfigureLogging(const char* params);

 private:
  friend class LogMessageForTesting;
  typedef std::pair<LogSink*, LoggingSeverity> StreamAndSeverity;
  typedef std::list<StreamAndSeverity> StreamList;

  // Updates min_sev_ appropriately when debug sinks change.
  static void UpdateMinLogSeverity();

  // These write out the actual log messages.
#if defined(WEBRTC_ANDROID)
  static void OutputToDebug(const std::string& msg,
                            LoggingSeverity severity,
                            const char* tag);
#else
  static void OutputToDebug(const std::string& msg, LoggingSeverity severity);
#endif

  // Checks the current global debug severity and if the |streams_| collection
  // is empty. If |severity| is smaller than the global severity and if the
  // |streams_| collection is empty, the LogMessage will be considered a noop
  // LogMessage.
  static bool IsNoop(LoggingSeverity severity);

  // Called from the dtor (or from a test) to append optional extra error
  // information to the log stream and a newline character.
  void FinishPrintStream();

  // The ostream that buffers the formatted message before output
  std::ostringstream print_stream_;

  // The severity level of this message
  LoggingSeverity severity_;

#if defined(WEBRTC_ANDROID)
  // The default Android debug output tag.
  const char* tag_ = "libjingle";
#endif

  // String data generated in the constructor, that should be appended to
  // the message before output.
  std::string extra_;

  const bool is_noop_;

  // The output streams and their associated severities
  static StreamList streams_;

  // Flags for formatting options
  static bool thread_, timestamp_;

  // Determines if logs will be directed to stderr in debug mode.
  static bool log_to_stderr_;

  RTC_DISALLOW_COPY_AND_ASSIGN(LogMessage);
};

//////////////////////////////////////////////////////////////////////
// Logging Helpers
//////////////////////////////////////////////////////////////////////

// The following non-obvious technique for implementation of a
// conditional log stream was stolen from google3/base/logging.h.

// This class is used to explicitly ignore values in the conditional
// logging macros.  This avoids compiler warnings like "value computed
// is not used" and "statement has no effect".

class LogMessageVoidify {
 public:
  LogMessageVoidify() { }
  // This has to be an operator with a precedence lower than << but
  // higher than ?:
  void operator&(std::ostream&) { }
};

#define RTC_LOG_SEVERITY_PRECONDITION(sev) \
  !(rtc::LogMessage::Loggable(sev)) \
    ? (void) 0 \
    : rtc::LogMessageVoidify() &

#define RTC_LOG_SEVERITY_PRECONDITION_C(sev) \
  !(rtc::LogMessage::Loggable<rtc::sev>()) ? (void)0 : rtc::LogMessageVoidify()&
#define RTC_LOG(sev)                                                        \
  RTC_LOG_SEVERITY_PRECONDITION_C(sev)                                      \
  rtc::LogMessage(__FILE__, __LINE__,                                       \
                  std::integral_constant<rtc::LoggingSeverity, rtc::sev>()) \
      .stream()

// The _V version is for when a variable is passed in.  It doesn't do the
// namespace concatenation.
#define RTC_LOG_V(sev) \
  RTC_LOG_SEVERITY_PRECONDITION(sev) \
    rtc::LogMessage(__FILE__, __LINE__, sev).stream()

// The _F version prefixes the message with the current function name.
#if (defined(__GNUC__) && !defined(NDEBUG)) || defined(WANT_PRETTY_LOG_F)
#define RTC_LOG_F(sev) RTC_LOG(sev) << __PRETTY_FUNCTION__ << ": "
#define RTC_LOG_T_F(sev) RTC_LOG(sev) << this << ": " \
  << __PRETTY_FUNCTION__ << ": "
#else
#define RTC_LOG_F(sev) RTC_LOG(sev) << __FUNCTION__ << ": "
#define RTC_LOG_T_F(sev) RTC_LOG(sev) << this << ": " << __FUNCTION__ << ": "
#endif

#define RTC_LOG_CHECK_LEVEL(sev) \
  rtc::LogCheckLevel(rtc::sev)
#define RTC_LOG_CHECK_LEVEL_V(sev) \
  rtc::LogCheckLevel(sev)

inline bool LogCheckLevel(LoggingSeverity sev) {
  return (LogMessage::GetMinLogSeverity() <= sev);
}

#define RTC_LOG_E(sev, ctx, err, ...)                                   \
  RTC_LOG_SEVERITY_PRECONDITION_C(sev)                                  \
  rtc::LogMessage(__FILE__, __LINE__, rtc::sev, rtc::ERRCTX_##ctx, err, \
                  ##__VA_ARGS__)                                        \
      .stream()

#define RTC_LOG_T(sev) RTC_LOG(sev) << this << ": "

#define RTC_LOG_ERRNO_EX(sev, err) \
  RTC_LOG_E(sev, ERRNO, err)
#define RTC_LOG_ERRNO(sev) \
  RTC_LOG_ERRNO_EX(sev, errno)

#if defined(WEBRTC_WIN)
#define RTC_LOG_GLE_EX(sev, err) \
  RTC_LOG_E(sev, HRESULT, err)
#define RTC_LOG_GLE(sev) \
  RTC_LOG_GLE_EX(sev, GetLastError())
#define RTC_LOG_ERR_EX(sev, err) \
  RTC_LOG_GLE_EX(sev, err)
#define RTC_LOG_ERR(sev) \
  RTC_LOG_GLE(sev)
#elif defined(__native_client__) && __native_client__
#define RTC_LOG_ERR_EX(sev, err) \
  RTC_LOG(sev)
#define RTC_LOG_ERR(sev) \
  RTC_LOG(sev)
#elif defined(WEBRTC_POSIX)
#define RTC_LOG_ERR_EX(sev, err) \
  RTC_LOG_ERRNO_EX(sev, err)
#define RTC_LOG_ERR(sev) \
  RTC_LOG_ERRNO(sev)
#endif  // WEBRTC_WIN

#if defined(WEBRTC_ANDROID)
namespace internal {
// Inline adapters provided for backwards compatibility for downstream projects.
inline const char* AdaptString(const char* str) { return str; }
inline const char* AdaptString(const std::string& str) { return str.c_str(); }
}  // namespace internal
#define RTC_LOG_TAG(sev, tag)        \
  RTC_LOG_SEVERITY_PRECONDITION(sev) \
  rtc::LogMessage(nullptr, 0, sev, rtc::internal::AdaptString(tag)).stream()
#else
// DEPRECATED. This macro is only intended for Android.
#define RTC_LOG_TAG(sev, tag)        \
  RTC_LOG_SEVERITY_PRECONDITION(sev) \
  rtc::LogMessage(nullptr, 0, sev).stream()
#endif

// The RTC_DLOG macros are equivalent to their RTC_LOG counterparts except that
// they only generate code in debug builds.
#if RTC_DLOG_IS_ON
#define RTC_DLOG(sev) RTC_LOG(sev)
#define RTC_DLOG_V(sev) RTC_LOG_V(sev)
#define RTC_DLOG_F(sev) RTC_LOG_F(sev)
#else
#define RTC_DLOG_EAT_STREAM_PARAMS(sev) \
  (true ? true : ((void)(sev), true))   \
      ? static_cast<void>(0)            \
      : rtc::LogMessageVoidify() &      \
            rtc::LogMessage(__FILE__, __LINE__, sev).stream()
#define RTC_DLOG(sev) RTC_DLOG_EAT_STREAM_PARAMS(rtc::sev)
#define RTC_DLOG_V(sev) RTC_DLOG_EAT_STREAM_PARAMS(sev)
#define RTC_DLOG_F(sev) RTC_DLOG_EAT_STREAM_PARAMS(rtc::sev)
#endif

}  // namespace rtc

#endif  // RTC_BASE_LOGGING_H_
