// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_BASE_WIN_MF_HELPERS_H_
#define MEDIA_BASE_WIN_MF_HELPERS_H_

#include <mfapi.h>
#include <stdint.h>
#include <wrl/client.h>

#include "base/logging.h"
#include "base/macros.h"
#include "media/base/win/mf_initializer_export.h"

namespace media {

namespace mf {

#define RETURN_ON_FAILURE(result, log, ret) \
  do {                                      \
    if (!(result)) {                        \
      DLOG(ERROR) << log;                   \
      mf::LogDXVAError(__LINE__);           \
      return ret;                           \
    }                                       \
  } while (0)

#define RETURN_ON_HR_FAILURE(result, log, ret) \
  RETURN_ON_FAILURE(SUCCEEDED(result),         \
                    log << ", HRESULT: 0x" << std::hex << result, ret);

#define RETURN_AND_NOTIFY_ON_FAILURE(result, log, error_code, ret) \
  do {                                                             \
    if (!(result)) {                                               \
      DVLOG(1) << log;                                             \
      mf::LogDXVAError(__LINE__);                                  \
      StopOnError(error_code);                                     \
      return ret;                                                  \
    }                                                              \
  } while (0)

#define RETURN_AND_NOTIFY_ON_HR_FAILURE(result, log, error_code, ret)        \
  RETURN_AND_NOTIFY_ON_FAILURE(SUCCEEDED(result),                            \
                               log << ", HRESULT: 0x" << std::hex << result, \
                               error_code, ret);

MF_INITIALIZER_EXPORT void LogDXVAError(int line);

// Creates a Media Foundation sample with one buffer of length |buffer_length|
// on a |align|-byte boundary. Alignment must be a perfect power of 2 or 0.
MF_INITIALIZER_EXPORT Microsoft::WRL::ComPtr<IMFSample>
CreateEmptySampleWithBuffer(uint32_t buffer_length, int align);

// Provides scoped access to the underlying buffer in an IMFMediaBuffer
// instance.
class MF_INITIALIZER_EXPORT MediaBufferScopedPointer {
 public:
  MediaBufferScopedPointer(IMFMediaBuffer* media_buffer);
  ~MediaBufferScopedPointer();

  uint8_t* get() { return buffer_; }
  DWORD current_length() const { return current_length_; }

 private:
  Microsoft::WRL::ComPtr<IMFMediaBuffer> media_buffer_;
  uint8_t* buffer_;
  DWORD max_length_;
  DWORD current_length_;

  DISALLOW_COPY_AND_ASSIGN(MediaBufferScopedPointer);
};

}  // namespace mf

}  // namespace media

#endif  // MEDIA_BASE_WIN_MF_HELPERS_H_
