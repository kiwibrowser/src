// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/services/mojo_jpeg_decode_accelerator_service.h"

#include "base/bind.h"
#include "base/command_line.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "base/threading/thread.h"
#include "media/base/media_switches.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace media {

static const int32_t kArbitraryBitstreamBufferId = 123;

// Test fixture for the unit that is created via the mojom interface for
// class MojoJpegDecodeAcceleratorService. Uses a FakeJpegDecodeAccelerator to
// simulate the actual decoding without the need for special hardware.
class MojoJpegDecodeAcceleratorServiceTest : public ::testing::Test {
 public:
 MojoJpegDecodeAcceleratorServiceTest() = default;
  ~MojoJpegDecodeAcceleratorServiceTest() override = default;

  void SetUp() override {
    base::CommandLine::ForCurrentProcess()->AppendSwitch(
        switches::kUseFakeJpegDecodeAccelerator);
  }

  void OnInitializeDone(const base::Closure& continuation, bool success) {
    EXPECT_TRUE(success);
    continuation.Run();
  }

  void OnDecodeAck(const base::Closure& continuation,
                   int32_t bitstream_buffer_id,
                   JpegDecodeAccelerator::Error error) {
    EXPECT_EQ(kArbitraryBitstreamBufferId, bitstream_buffer_id);
    continuation.Run();
  }

 private:
  // This is required to allow base::ThreadTaskRunnerHandle::Get() from the
  // test execution thread.
  base::test::ScopedTaskEnvironment scoped_task_environment_;
};

TEST_F(MojoJpegDecodeAcceleratorServiceTest, InitializeAndDecode) {
  mojom::JpegDecodeAcceleratorPtr jpeg_decoder;
  MojoJpegDecodeAcceleratorService::Create(mojo::MakeRequest(&jpeg_decoder));

  base::RunLoop run_loop;
  jpeg_decoder->Initialize(
      base::Bind(&MojoJpegDecodeAcceleratorServiceTest::OnInitializeDone,
                 base::Unretained(this), run_loop.QuitClosure()));
  run_loop.Run();

  const size_t kInputBufferSizeInBytes = 512;
  const size_t kOutputFrameSizeInBytes = 1024;
  const gfx::Size kDummyFrameCodedSize(10, 10);
  const char kKeyId[] = "key id";
  const char kIv[] = "0123456789abcdef";
  std::vector<SubsampleEntry> subsamples;
  subsamples.push_back(SubsampleEntry(10, 5));
  subsamples.push_back(SubsampleEntry(15, 7));

  base::RunLoop run_loop2;
  base::SharedMemory shm;
  ASSERT_TRUE(shm.CreateAndMapAnonymous(kInputBufferSizeInBytes));

  mojo::ScopedSharedBufferHandle output_frame_handle =
      mojo::SharedBufferHandle::Create(kOutputFrameSizeInBytes);

  BitstreamBuffer bitstream_buffer(
      kArbitraryBitstreamBufferId,
      base::SharedMemory::DuplicateHandle(shm.handle()),
      kInputBufferSizeInBytes);
  bitstream_buffer.SetDecryptionSettings(kKeyId, kIv, subsamples);

  jpeg_decoder->Decode(
      bitstream_buffer, kDummyFrameCodedSize, std::move(output_frame_handle),
      base::checked_cast<uint32_t>(kOutputFrameSizeInBytes),
      base::Bind(&MojoJpegDecodeAcceleratorServiceTest::OnDecodeAck,
                 base::Unretained(this), run_loop2.QuitClosure()));
  run_loop2.Run();
}

}  // namespace media
