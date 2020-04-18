// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_TRACING_TEST_FAKE_SCATTERED_BUFFER_H_
#define COMPONENTS_TRACING_TEST_FAKE_SCATTERED_BUFFER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/macros.h"
#include "components/tracing/core/scattered_stream_writer.h"
#include "components/tracing/tracing_export.h"

namespace tracing {
namespace v2 {

// A simple ScatteredStreamWriter::Delegate implementation which just allocates
// chunks of a fixed size.
class FakeScatteredBuffer : public ScatteredStreamWriter::Delegate {
 public:
  explicit FakeScatteredBuffer(size_t chunk_size);
  ~FakeScatteredBuffer() override;

  // ScatteredStreamWriter::Delegate implementation.
  ContiguousMemoryRange GetNewBuffer() override;

  std::string GetChunkAsString(int chunk_index);

  void GetBytes(size_t start, size_t length, uint8_t* buf);
  std::string GetBytesAsString(size_t start, size_t length);

  const std::vector<std::unique_ptr<uint8_t[]>>& chunks() const {
    return chunks_;
  }

 private:
  const size_t chunk_size_;
  std::vector<std::unique_ptr<uint8_t[]>> chunks_;

  DISALLOW_COPY_AND_ASSIGN(FakeScatteredBuffer);
};

}  // namespace v2
}  // namespace tracing

#endif  // COMPONENTS_TRACING_TEST_FAKE_SCATTERED_BUFFER_H_
