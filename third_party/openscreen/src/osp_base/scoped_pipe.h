// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef OSP_BASE_SCOPED_PIPE_H_
#define OSP_BASE_SCOPED_PIPE_H_

#include <unistd.h>

#include <utility>

namespace openscreen {

struct IntFdTraits {
  using PipeType = int;
  static constexpr int kInvalidValue = -1;

  static void Close(PipeType pipe) { close(pipe); }
};

// This class wraps file descriptor and uses RAII to ensure it is closed
// properly when control leaves its scope.  It is parameterized by a traits type
// which defines the value type of the file descriptor, an invalid value, and a
// closing function.
//
// This class is move-only as it represents ownership of the wrapped file
// descriptor.  It is not thread-safe.
template <typename Traits>
class ScopedPipe {
 public:
  using PipeType = typename Traits::PipeType;

  ScopedPipe() : pipe_(Traits::kInvalidValue) {}
  explicit ScopedPipe(PipeType pipe) : pipe_(pipe) {}
  ScopedPipe(const ScopedPipe&) = delete;
  ScopedPipe(ScopedPipe&& other) : pipe_(other.release()) {}
  ~ScopedPipe() {
    if (pipe_ != Traits::kInvalidValue)
      Traits::Close(release());
  }

  ScopedPipe& operator=(ScopedPipe&& other) {
    if (pipe_ != Traits::kInvalidValue)
      Traits::Close(release());
    pipe_ = other.release();
    return *this;
  }

  PipeType get() const { return pipe_; }
  PipeType release() {
    PipeType pipe = pipe_;
    pipe_ = Traits::kInvalidValue;
    return pipe;
  }

  bool operator==(const ScopedPipe& other) const {
    return pipe_ == other.pipe_;
  }
  bool operator!=(const ScopedPipe& other) const { return !(*this == other); }

  explicit operator bool() const { return pipe_ != Traits::kInvalidValue; }

 private:
  PipeType pipe_;
};

using ScopedFd = ScopedPipe<IntFdTraits>;

}  // namespace openscreen

#endif  // OSP_BASE_SCOPED_PIPE_H_
