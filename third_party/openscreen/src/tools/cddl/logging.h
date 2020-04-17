// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_CDDL_LOGGING_H_
#define TOOLS_CDDL_LOGGING_H_

#include <fstream>
#include <iostream>
#include <string>

#include "platform/api/logging.h"

// A wrapper around all logging methods, so that the underlying logging
// implementation can easily be swapped out, and to that loggers only ever need
// to call static Log(...) or Error(...) methods
// NOTES:
// Template methods are fully defined in the header file here instead of the cc
//   to get around needing explicit instantiation for the template, since we
//   don't have that information at this point.
class Logger {
 public:
  // Writes a log to the global singleton instance of Logger.
  template <typename... Args>
  static void Log(const std::string& message, Args&&... args) {
    Logger::Get()->WriteLog(message, std::forward<Args>(args)...);
  }

  // Writes an error to the global singleton instance of Logger.
  template <typename... Args>
  static void Error(const std::string& message, Args&&... args) {
    Logger::Get()->WriteError(message, std::forward<Args>(args)...);
  }

  // Returns the singleton instance of Logger.
  static Logger* Get();

 private:
  // Creates and initializes the logging file associated with this logger.
  void InitializeInstance();

  // Limit calling the constructor/destructor to from within this same class.
  Logger();

  // Represents whether this instance has been initialized.
  bool is_initialized_;

  // Singleton instance of logger. At the beginning of runtime it's initiated to
  // nullptr due to zero initialization.
  static Logger* singleton_;

  // Exits the program if initialization has not occured.
  void VerifyInitialized();

  // fprintf doesn't like passing strings as parameters, so use overloads to
  // convert all C++ std::string types into C strings.
  template <class T>
  T MakePrintable(const T data) {
    return data;
  }

  const char* MakePrintable(const std::string data);

  // Writes a log message to this instance of Logger's text file.
  template <typename... Args>
  void WriteToStream(std::ostream& stream,
                     const std::string& message,
                     Args&&... args) {
    VerifyInitialized();

    // NOTE: wihout the #pragma suppressions, the below line fails. There is a
    // warning generated since the compiler is attempting to prevent a string
    // format vulnerability. This is not a risk for us since this code is only
    // used at compile time. The below #pragma commands suppress the warning for
    // just the one dprintf(...) line.
    // For more details: https://www.owasp.org/index.php/Format_string_attack
    char* str_buffer;
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-security"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-security"
#endif  // defined(__clang__)
    int byte_count = asprintf(&str_buffer, message.c_str(),
                              this->MakePrintable(std::forward<Args>(args))...);
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif  // defined(__clang__)
    OSP_CHECK_GE(byte_count, 0);
    stream << str_buffer;
    free(str_buffer);
  }

  // Writes an error message to this instance of Logger's text file.
  template <typename... Args>
  void WriteError(const std::string& message, Args&&... args) {
    this->WriteToStream(OSP_LOG_ERROR, "Error: " + message,
                        std::forward<Args>(args)...);
  }

  // Writes a log message to this instance of Logger's text file.
  template <typename... Args>
  void WriteLog(const std::string& message, Args&&... args) {
    this->WriteToStream(OSP_LOG_INFO, message, std::forward<Args>(args)...);
  }
};

#endif  // TOOLS_CDDL_LOGGING_H_
