// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_COMMAND_BUFFER_CLIENT_LOGGING_H_
#define GPU_COMMAND_BUFFER_CLIENT_LOGGING_H_

#include "base/logging.h"
#include "base/macros.h"
#include "gpu/command_buffer/client/gles2_impl_export.h"

// Macros to log information if DCHECK_IS_ON() and --enable-gpu-client-logging
// flag is set. Code is optimized out if DCHECK is disabled. Requires that a
// LogSettings named log_settings_ is in scope whenever a macro is used.
//
// Example usage:
//
// class Foo {
//  public:
//   Foo() {
//     GPU_CLIENT_LOG("[" << LogPrefix() << "] Hello world");
//     GPU_CLIENT_LOG_CODE_BLOCK({
//       for (int i = 0; i < 10; ++i) {
//         GPU_CLIENT_LOG_CODE_BLOCK("Hello again");
//       }
//     });
//   }
//
//   std::string LogPrefix() { return "Foo"; }
//
//  private:
//   LogSettings log_settings_;
// };

#if DCHECK_IS_ON() && !defined(__native_client__) && \
    !defined(GLES2_CONFORMANCE_TESTS) && !defined(GLES2_INLINE_OPTIMIZATION)
#define GPU_CLIENT_DEBUG
#endif

#if defined(GPU_CLIENT_DEBUG)
#define GPU_CLIENT_LOG(args) DLOG_IF(INFO, log_settings_.enabled()) << args;
#define GPU_CLIENT_LOG_CODE_BLOCK(code) code
#define GPU_CLIENT_DCHECK_CODE_BLOCK(code) code
#else  // !defined(GPU_CLIENT_DEBUG)
#define GPU_CLIENT_LOG(args)
#define GPU_CLIENT_LOG_CODE_BLOCK(code)
#define GPU_CLIENT_DCHECK_CODE_BLOCK(code)
#endif  // defined(GPU_CLIENT_DEBUG)

namespace gpu {

// Caches whether --enable-gpu-client-logging is set.
class GLES2_IMPL_EXPORT LogSettings {
 public:
  LogSettings();
  ~LogSettings();

  bool enabled() { return enabled_; }

 private:
  bool enabled_;

  DISALLOW_COPY_AND_ASSIGN(LogSettings);
};

}  // namespace gpu

#endif  // GPU_COMMAND_BUFFER_CLIENT_LOGGING_H_
