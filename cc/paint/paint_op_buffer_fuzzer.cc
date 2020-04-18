// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>

#include "base/command_line.h"
#include "cc/paint/paint_op_buffer.h"
#include "cc/test/transfer_cache_test_helper.h"
#include "components/viz/test/test_context_provider.h"
#include "gpu/command_buffer/common/buffer.h"
#include "gpu/command_buffer/service/service_font_manager.h"
#include "third_party/skia/include/core/SkSurface.h"
#include "third_party/skia/include/gpu/GrContext.h"

struct Environment {
  Environment() {
    // Disable noisy logging as per "libFuzzer in Chrome" documentation:
    // testing/libfuzzer/getting_started.md#Disable-noisy-error-message-logging.
    logging::SetMinLogLevel(logging::LOG_FATAL);
  }
};

class FontSupport : public gpu::ServiceFontManager::Client {
 public:
  FontSupport() = default;
  ~FontSupport() override = default;

  // gpu::ServiceFontManager::Client implementation.
  scoped_refptr<gpu::Buffer> GetShmBuffer(uint32_t shm_id) override {
    auto it = buffers_.find(shm_id);
    if (it != buffers_.end())
      return it->second;
    return CreateBuffer(shm_id);
  }

 private:
  scoped_refptr<gpu::Buffer> CreateBuffer(uint32_t shm_id) {
    std::unique_ptr<base::SharedMemory> shared_memory(new base::SharedMemory());
    static const size_t kBufferSize = 2048u;
    shared_memory->CreateAndMapAnonymous(kBufferSize);
    auto buffer =
        gpu::MakeBufferFromSharedMemory(std::move(shared_memory), kBufferSize);
    buffers_[shm_id] = buffer;
    return buffer;
  }

  base::flat_map<uint32_t, scoped_refptr<gpu::Buffer>> buffers_;
};

// Deserialize an arbitrary number of cc::PaintOps and raster them
// using gpu raster into an SkCanvas.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  if (size <= sizeof(size_t))
    return 0;

  static Environment* env = new Environment();
  ALLOW_UNUSED_LOCAL(env);
  base::CommandLine::Init(0, nullptr);
  const size_t kMaxSerializedSize = 1000000;
  const size_t kRasterDimension = 32;

  // Partition the data to use some bytes for populating the font cache.
  size_t bytes_for_fonts = data[0];
  if (bytes_for_fonts > size)
    bytes_for_fonts = size / 2;

  FontSupport font_support;
  gpu::ServiceFontManager font_manager(&font_support);
  std::vector<SkDiscardableHandleId> locked_handles;
  if (bytes_for_fonts > 0u) {
    font_manager.Deserialize(reinterpret_cast<const char*>(data),
                             bytes_for_fonts, &locked_handles);
    data += bytes_for_fonts;
    size -= bytes_for_fonts;
  }

  SkImageInfo image_info = SkImageInfo::MakeN32(
      kRasterDimension, kRasterDimension, kOpaque_SkAlphaType);
  scoped_refptr<viz::TestContextProvider> context_provider =
      viz::TestContextProvider::Create();
  context_provider->BindToCurrentThread();
  sk_sp<SkSurface> surface = SkSurface::MakeRenderTarget(
      context_provider->GrContext(), SkBudgeted::kYes, image_info);
  SkCanvas* canvas = surface->getCanvas();

  cc::PlaybackParams params(nullptr, canvas->getTotalMatrix());
  cc::TransferCacheTestHelper transfer_cache_helper;
  cc::PaintOp::DeserializeOptions deserialize_options(
      &transfer_cache_helper, font_manager.strike_client());

  // Need 4 bytes to be able to read the type/skip.
  while (size >= 4) {
    const cc::PaintOp* serialized = reinterpret_cast<const cc::PaintOp*>(data);
    if (serialized->skip > kMaxSerializedSize)
      break;

    std::unique_ptr<char, base::AlignedFreeDeleter> deserialized(
        static_cast<char*>(base::AlignedAlloc(
            sizeof(cc::LargestPaintOp), cc::PaintOpBuffer::PaintOpAlign)));
    size_t bytes_read = 0;
    cc::PaintOp* deserialized_op = cc::PaintOp::Deserialize(
        data, size, deserialized.get(), sizeof(cc::LargestPaintOp), &bytes_read,
        deserialize_options);

    if (!deserialized_op)
      break;

    deserialized_op->Raster(canvas, params);

    deserialized_op->DestroyThis();

    if (serialized->skip >= size)
      break;

    size -= bytes_read;
    data += bytes_read;
  }

  font_manager.Unlock(locked_handles);
  return 0;
}
