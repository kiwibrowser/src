// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/gpu/vaapi/vaapi_video_decode_accelerator.h"

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "base/run_loop.h"
#include "base/test/scoped_task_environment.h"
#include "media/gpu/accelerated_video_decoder.h"
#include "media/gpu/format_utils.h"
#include "media/gpu/vaapi/vaapi_picture.h"
#include "media/gpu/vaapi/vaapi_picture_factory.h"
#include "media/gpu/vaapi/vaapi_wrapper.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using ::testing::_;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::Return;
using ::testing::TestWithParam;
using ::testing::ValuesIn;
using ::testing::WithArg;

namespace media {

namespace {

ACTION_P(RunClosure, closure) {
  closure.Run();
}

constexpr VideoCodecProfile kCodecProfiles[] = {H264PROFILE_MIN, VP8PROFILE_MIN,
                                                VP9PROFILE_MIN};
constexpr int32_t kBitstreamId = 123;
constexpr size_t kInputSize = 256;

constexpr size_t kNumPictures = 2;
const gfx::Size kPictureSize(64, 48);

constexpr size_t kNewNumPictures = 3;
const gfx::Size kNewPictureSize(64, 48);

}  // namespace

class MockAcceleratedVideoDecoder : public AcceleratedVideoDecoder {
 public:
  MockAcceleratedVideoDecoder() = default;
  ~MockAcceleratedVideoDecoder() override = default;

  MOCK_METHOD4(
      SetStream,
      void(int32_t id, const uint8_t* ptr, size_t size, const DecryptConfig*));
  MOCK_METHOD0(Flush, bool());
  MOCK_METHOD0(Reset, void());
  MOCK_METHOD0(Decode, DecodeResult());
  MOCK_CONST_METHOD0(GetPicSize, gfx::Size());
  MOCK_CONST_METHOD0(GetRequiredNumOfPictures, size_t());
};

class MockVaapiWrapper : public VaapiWrapper {
 public:
  MockVaapiWrapper() = default;
  MOCK_METHOD4(
      CreateSurfaces,
      bool(unsigned int, const gfx::Size&, size_t, std::vector<VASurfaceID>*));
  MOCK_METHOD0(DestroySurfaces, void());

 private:
  ~MockVaapiWrapper() override = default;
};

class MockVaapiPicture : public VaapiPicture {
 public:
  MockVaapiPicture(const scoped_refptr<VaapiWrapper>& vaapi_wrapper,
                   const MakeGLContextCurrentCallback& make_context_current_cb,
                   const BindGLImageCallback& bind_image_cb,
                   int32_t picture_buffer_id,
                   const gfx::Size& size,
                   uint32_t texture_id,
                   uint32_t client_texture_id,
                   uint32_t texture_target)
      : VaapiPicture(vaapi_wrapper,
                     make_context_current_cb,
                     bind_image_cb,
                     picture_buffer_id,
                     size,
                     texture_id,
                     client_texture_id,
                     texture_target) {}
  ~MockVaapiPicture() override = default;

  // VaapiPicture implementation.
  bool Allocate(gfx::BufferFormat format) override { return true; }
  bool ImportGpuMemoryBufferHandle(
      gfx::BufferFormat format,
      const gfx::GpuMemoryBufferHandle& gpu_memory_buffer_handle) override {
    return true;
  }
  bool DownloadFromSurface(
      const scoped_refptr<VASurface>& va_surface) override {
    return true;
  }
  bool AllowOverlay() const override { return false; }
};

class MockVaapiPictureFactory : public VaapiPictureFactory {
 public:
  MockVaapiPictureFactory() = default;
  ~MockVaapiPictureFactory() override = default;

  MOCK_METHOD2(MockCreateVaapiPicture, void(VaapiWrapper*, const gfx::Size&));
  std::unique_ptr<VaapiPicture> Create(
      const scoped_refptr<VaapiWrapper>& vaapi_wrapper,
      const MakeGLContextCurrentCallback& make_context_current_cb,
      const BindGLImageCallback& bind_image_cb,
      int32_t picture_buffer_id,
      const gfx::Size& size,
      uint32_t texture_id,
      uint32_t client_texture_id,
      uint32_t texture_target) override {
    MockCreateVaapiPicture(vaapi_wrapper.get(), size);
    return std::make_unique<MockVaapiPicture>(
        vaapi_wrapper, make_context_current_cb, bind_image_cb,
        picture_buffer_id, size, texture_id, client_texture_id, texture_target);
  }
};

class VaapiVideoDecodeAcceleratorTest : public TestWithParam<VideoCodecProfile>,
                                        public VideoDecodeAccelerator::Client {
 public:
  VaapiVideoDecodeAcceleratorTest()
      : vda_(base::Bind([] { return true; }),
             base::Bind([](uint32_t client_texture_id,
                           uint32_t texture_target,
                           const scoped_refptr<gl::GLImage>& image,
                           bool can_bind_to_sampler) { return true; })),
        decoder_thread_("VaapiVideoDecodeAcceleratorTestThread"),
        mock_decoder_(new MockAcceleratedVideoDecoder),
        mock_vaapi_picture_factory_(new MockVaapiPictureFactory()),
        mock_vaapi_wrapper_(new MockVaapiWrapper()),
        weak_ptr_factory_(this) {
    decoder_thread_.Start();

    // Don't want to go through a vda_->Initialize() because it binds too many
    // items of the environment. Instead, just start the decoder thread.
    vda_.decoder_thread_task_runner_ = decoder_thread_.task_runner();

    // Plug in all the mocks and ourselves as the |client_|.
    vda_.decoder_.reset(mock_decoder_);
    vda_.client_ = weak_ptr_factory_.GetWeakPtr();
    vda_.vaapi_wrapper_ = mock_vaapi_wrapper_;
    vda_.vaapi_picture_factory_.reset(mock_vaapi_picture_factory_);

    vda_.state_ = VaapiVideoDecodeAccelerator::kIdle;
  }
  ~VaapiVideoDecodeAcceleratorTest() {}

  void SetUp() override {
    in_shm_.reset(new base::SharedMemory);
    ASSERT_TRUE(in_shm_->CreateAndMapAnonymous(kInputSize));
  }

  void SetVdaStateToUnitialized() {
    vda_.state_ = VaapiVideoDecodeAccelerator::kUninitialized;
  }

  void QueueInputBuffer(const BitstreamBuffer& bitstream_buffer) {
    vda_.QueueInputBuffer(bitstream_buffer);
  }

  void AssignPictureBuffers(const std::vector<PictureBuffer>& picture_buffers) {
    vda_.AssignPictureBuffers(picture_buffers);
  }

  // Reset epilogue, needed to get |vda_| worker thread out of its Wait().
  void ResetSequence() {
    base::RunLoop run_loop;
    base::Closure quit_closure = run_loop.QuitClosure();
    EXPECT_CALL(*mock_decoder_, Reset());
    EXPECT_CALL(*this, NotifyResetDone()).WillOnce(RunClosure(quit_closure));
    vda_.Reset();
    run_loop.Run();
  }

  // Try and QueueInputBuffer()s, where we pretend that |mock_decoder_| requests
  // to kAllocateNewSurfaces: |vda_| will ping us to ProvidePictureBuffers().
  // If |expect_dismiss_picture_buffers| is signalled, then we expect as well
  // that |vda_| will emit |num_picture_buffers_to_dismiss| DismissPictureBuffer
  // calls.
  void QueueInputBufferSequence(size_t num_pictures,
                                const gfx::Size& picture_size,
                                int32_t bitstream_id,
                                bool expect_dismiss_picture_buffers = false,
                                size_t num_picture_buffers_to_dismiss = 0) {
    ::testing::InSequence s;
    base::RunLoop run_loop;
    base::Closure quit_closure = run_loop.QuitClosure();
    EXPECT_CALL(*mock_decoder_, SetStream(_, _, kInputSize, nullptr));
    EXPECT_CALL(*mock_decoder_, Decode())
        .WillOnce(Return(AcceleratedVideoDecoder::kAllocateNewSurfaces));

    EXPECT_CALL(*mock_decoder_, GetRequiredNumOfPictures())
        .WillOnce(Return(num_pictures));
    EXPECT_CALL(*mock_decoder_, GetPicSize()).WillOnce(Return(picture_size));
    EXPECT_CALL(*mock_vaapi_wrapper_, DestroySurfaces());

    if (expect_dismiss_picture_buffers) {
      EXPECT_CALL(*this, DismissPictureBuffer(_))
          .Times(num_picture_buffers_to_dismiss);
    }

    EXPECT_CALL(*this,
                ProvidePictureBuffers(num_pictures, _, 1, picture_size, _))
        .WillOnce(RunClosure(quit_closure));

    base::SharedMemoryHandle handle;
    handle = base::SharedMemory::DuplicateHandle(in_shm_->handle());
    BitstreamBuffer bitstream_buffer(bitstream_id, handle, kInputSize);

    QueueInputBuffer(bitstream_buffer);
    run_loop.Run();
  }

  // Calls AssignPictureBuffers(), expecting the corresponding mock calls; we
  // pretend |mock_decoder_| has kRanOutOfStreamData (i.e. it's finished
  // decoding) and expect |vda_| to emit a NotifyEndOfBitstreamBuffer().
  // QueueInputBufferSequence() must have been called beforehand.
  void AssignPictureBuffersSequence(size_t num_pictures,
                                    const gfx::Size& picture_size,
                                    int32_t bitstream_id) {
    ASSERT_TRUE(vda_.curr_input_buffer_)
        << "QueueInputBuffer() should have been called";

    ::testing::InSequence s;
    base::RunLoop run_loop;
    base::Closure quit_closure = run_loop.QuitClosure();

    EXPECT_CALL(*mock_vaapi_wrapper_,
                CreateSurfaces(_, picture_size, num_pictures, _))
        .WillOnce(DoAll(
            WithArg<3>(Invoke(
                [num_pictures](std::vector<VASurfaceID>* va_surface_ids) {
                  va_surface_ids->resize(num_pictures);
                })),
            Return(true)));
    EXPECT_CALL(*mock_vaapi_picture_factory_,
                MockCreateVaapiPicture(mock_vaapi_wrapper_.get(), picture_size))
        .Times(num_pictures);

    EXPECT_CALL(*mock_decoder_, Decode())
        .WillOnce(Return(AcceleratedVideoDecoder::kRanOutOfStreamData));
    EXPECT_CALL(*this, NotifyEndOfBitstreamBuffer(bitstream_id))
        .WillOnce(RunClosure(quit_closure));

    const auto tex_target = mock_vaapi_picture_factory_->GetGLTextureTarget();
    int irrelevant_id = 2;
    std::vector<PictureBuffer> picture_buffers;
    for (size_t picture = 0; picture < num_pictures; ++picture) {
      // The picture buffer id, client id and service texture ids are
      // arbitrarily chosen.
      picture_buffers.push_back({irrelevant_id++, picture_size,
                                 PictureBuffer::TextureIds{irrelevant_id++},
                                 PictureBuffer::TextureIds{irrelevant_id++},
                                 tex_target, PIXEL_FORMAT_XRGB});
    }

    AssignPictureBuffers(picture_buffers);
    run_loop.Run();
  }

  // Calls QueueInputBuffer(); we instruct from |mock_decoder_| that it has
  // kRanOutOfStreamData (i.e. it's finished decoding). This is a fast method
  // because the Decode() is (almost) immediate.
  void DecodeOneFrameFast(int32_t bitstream_id) {
    base::RunLoop run_loop;
    base::Closure quit_closure = run_loop.QuitClosure();
    EXPECT_CALL(*mock_decoder_, SetStream(_, _, kInputSize, nullptr));
    EXPECT_CALL(*mock_decoder_, Decode())
        .WillOnce(Return(AcceleratedVideoDecoder::kRanOutOfStreamData));
    EXPECT_CALL(*this, NotifyEndOfBitstreamBuffer(bitstream_id))
        .WillOnce(RunClosure(quit_closure));

    base::SharedMemoryHandle handle;
    handle = base::SharedMemory::DuplicateHandle(in_shm_->handle());
    BitstreamBuffer bitstream_buffer(bitstream_id, handle, kInputSize);

    QueueInputBuffer(bitstream_buffer);
    run_loop.Run();
  }

  // VideoDecodeAccelerator::Client methods.
  MOCK_METHOD1(NotifyInitializationComplete, void(bool));
  MOCK_METHOD5(
      ProvidePictureBuffers,
      void(uint32_t, VideoPixelFormat, uint32_t, const gfx::Size&, uint32_t));
  MOCK_METHOD1(DismissPictureBuffer, void(int32_t));
  MOCK_METHOD1(PictureReady, void(const Picture&));
  MOCK_METHOD1(NotifyEndOfBitstreamBuffer, void(int32_t));
  MOCK_METHOD0(NotifyFlushDone, void());
  MOCK_METHOD0(NotifyResetDone, void());
  MOCK_METHOD1(NotifyError, void(VideoDecodeAccelerator::Error));

  base::test::ScopedTaskEnvironment scoped_task_environment_;

  // The class under test and a worker thread for it.
  VaapiVideoDecodeAccelerator vda_;
  base::Thread decoder_thread_;

  // Ownership passed to |vda_|, but we retain a pointer to it for MOCK checks.
  MockAcceleratedVideoDecoder* mock_decoder_;
  MockVaapiPictureFactory* mock_vaapi_picture_factory_;

  scoped_refptr<MockVaapiWrapper> mock_vaapi_wrapper_;

  std::unique_ptr<base::SharedMemory> in_shm_;

 private:
  base::WeakPtrFactory<VaapiVideoDecodeAcceleratorTest> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(VaapiVideoDecodeAcceleratorTest);
};

// Verify that it is possible to select DRM(egl) and TFP(glx) at runtime.
TEST_P(VaapiVideoDecodeAcceleratorTest, SupportedPlatforms) {
  EXPECT_EQ(VaapiPictureFactory::kVaapiImplementationNone,
            mock_vaapi_picture_factory_->GetVaapiImplementation(
                gl::kGLImplementationNone));
  EXPECT_EQ(VaapiPictureFactory::kVaapiImplementationDrm,
            mock_vaapi_picture_factory_->GetVaapiImplementation(
                gl::kGLImplementationEGLGLES2));

#if defined(USE_X11)
  EXPECT_EQ(VaapiPictureFactory::kVaapiImplementationX11,
            mock_vaapi_picture_factory_->GetVaapiImplementation(
                gl::kGLImplementationDesktopGL));
#endif
}

// This test checks that QueueInputBuffer() fails when state is kUnitialized.
TEST_P(VaapiVideoDecodeAcceleratorTest, QueueInputBufferAndError) {
  SetVdaStateToUnitialized();

  base::SharedMemoryHandle handle;
  handle = base::SharedMemory::DuplicateHandle(in_shm_->handle());
  BitstreamBuffer bitstream_buffer(kBitstreamId, handle, kInputSize);

  EXPECT_CALL(*this,
              NotifyError(VaapiVideoDecodeAccelerator::PLATFORM_FAILURE));
  QueueInputBuffer(bitstream_buffer);
}

// Verifies that Decode() returning kDecodeError ends up pinging NotifyError().
TEST_P(VaapiVideoDecodeAcceleratorTest, QueueInputBufferAndDecodeError) {
  base::SharedMemoryHandle handle;
  handle = base::SharedMemory::DuplicateHandle(in_shm_->handle());
  BitstreamBuffer bitstream_buffer(kBitstreamId, handle, kInputSize);

  base::RunLoop run_loop;
  base::Closure quit_closure = run_loop.QuitClosure();
  EXPECT_CALL(*mock_decoder_, SetStream(_, _, kInputSize, nullptr));
  EXPECT_CALL(*mock_decoder_, Decode())
      .WillOnce(Return(AcceleratedVideoDecoder::kDecodeError));
  EXPECT_CALL(*this, NotifyError(VaapiVideoDecodeAccelerator::PLATFORM_FAILURE))
      .WillOnce(RunClosure(quit_closure));

  QueueInputBuffer(bitstream_buffer);
  run_loop.Run();
}

// Verifies a single fast frame decoding..
TEST_P(VaapiVideoDecodeAcceleratorTest, DecodeOneFrame) {
  DecodeOneFrameFast(kBitstreamId);

  ResetSequence();
}

// Tests usual startup sequence: a BitstreamBuffer is enqueued for decode;
// |vda_| asks for PictureBuffers, that we provide via AssignPictureBuffers().
TEST_P(VaapiVideoDecodeAcceleratorTest,
       QueueInputBuffersAndAssignPictureBuffers) {
  QueueInputBufferSequence(kNumPictures, kPictureSize, kBitstreamId);

  AssignPictureBuffersSequence(kNumPictures, kPictureSize, kBitstreamId);

  ResetSequence();
}

// Tests a typical resolution change sequence: a BitstreamBuffer is enqueued;
// |vda_| asks for PictureBuffers, we them provide via AssignPictureBuffers().
// We then try to enqueue a few BitstreamBuffers of a different resolution: we
// then expect the old ones to be dismissed and new ones provided.This sequence
// is purely ingress-wise, i.e. there's no decoded output checks.
TEST_P(VaapiVideoDecodeAcceleratorTest,
       QueueInputBuffersAndAssignPictureBuffersAndReallocate) {
  QueueInputBufferSequence(kNumPictures, kPictureSize, kBitstreamId);

  AssignPictureBuffersSequence(kNumPictures, kPictureSize, kBitstreamId);

  // Decode a few frames. This step is not necessary.
  for (int i = 0; i < 5; ++i)
    DecodeOneFrameFast(kBitstreamId + i);

  QueueInputBufferSequence(kNewNumPictures, kNewPictureSize, kBitstreamId,
                           true /* expect_dismiss_picture_buffers */,
                           kNumPictures /* num_picture_buffers_to_dismiss */);

  AssignPictureBuffersSequence(kNewNumPictures, kNewPictureSize, kBitstreamId);

  ResetSequence();
}

INSTANTIATE_TEST_CASE_P(/* No prefix. */,
                        VaapiVideoDecodeAcceleratorTest,
                        ValuesIn(kCodecProfiles));

}  // namespace media
