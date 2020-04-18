// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>
#include <unistd.h>

#include <memory>

#include "base/files/file_util.h"
#include "base/files/platform_file.h"
#include "base/macros.h"
#include "base/posix/eintr_wrapper.h"
#include "base/test/scoped_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/gpu_fence.h"
#include "ui/gfx/gpu_fence_handle.h"
#include "ui/ozone/platform/drm/gpu/crtc_controller.h"
#include "ui/ozone/platform/drm/gpu/fake_plane_info.h"
#include "ui/ozone/platform/drm/gpu/hardware_display_plane_atomic.h"
#include "ui/ozone/platform/drm/gpu/hardware_display_plane_manager_atomic.h"
#include "ui/ozone/platform/drm/gpu/hardware_display_plane_manager_legacy.h"
#include "ui/ozone/platform/drm/gpu/mock_drm_device.h"
#include "ui/ozone/platform/drm/gpu/mock_hardware_display_plane_manager.h"
#include "ui/ozone/platform/drm/gpu/mock_scanout_buffer.h"

namespace {

const ui::FakePlaneInfo kOnePlanePerCrtc[] = {{10, 1}, {20, 2}};
const ui::FakePlaneInfo kTwoPlanesPerCrtc[] = {{10, 1},
                                               {11, 1},
                                               {20, 2},
                                               {21, 2}};
const ui::FakePlaneInfo kOnePlanePerCrtcWithShared[] = {{10, 1},
                                                        {20, 2},
                                                        {50, 3}};
const uint32_t kDummyFormat = 0;
const gfx::Size kDefaultBufferSize(2, 2);

class HardwareDisplayPlaneManagerTest : public testing::Test {
 public:
  HardwareDisplayPlaneManagerTest() {}

  void SetUp() override;

 protected:
  std::unique_ptr<ui::MockHardwareDisplayPlaneManager> plane_manager_;
  ui::HardwareDisplayPlaneList state_;
  std::vector<uint32_t> default_crtcs_;
  scoped_refptr<ui::ScanoutBuffer> fake_buffer_;
  scoped_refptr<ui::MockDrmDevice> fake_drm_;

 private:
  DISALLOW_COPY_AND_ASSIGN(HardwareDisplayPlaneManagerTest);
};

void HardwareDisplayPlaneManagerTest::SetUp() {
  fake_buffer_ = new ui::MockScanoutBuffer(kDefaultBufferSize);
  default_crtcs_.push_back(100);
  default_crtcs_.push_back(200);

  fake_drm_ = new ui::MockDrmDevice(false, default_crtcs_, 2);
  plane_manager_.reset(
      new ui::MockHardwareDisplayPlaneManager(fake_drm_.get()));
}

TEST_F(HardwareDisplayPlaneManagerTest, SinglePlaneAssignment) {
  ui::OverlayPlaneList assigns;
  assigns.push_back(ui::OverlayPlane(fake_buffer_, nullptr));
  plane_manager_->InitForTest(kOnePlanePerCrtc, arraysize(kOnePlanePerCrtc),
                              default_crtcs_);
  EXPECT_TRUE(plane_manager_->AssignOverlayPlanes(&state_, assigns,
                                                  default_crtcs_[0], nullptr));
  EXPECT_EQ(1, plane_manager_->plane_count());
}

TEST_F(HardwareDisplayPlaneManagerTest, BadCrtc) {
  ui::OverlayPlaneList assigns;
  assigns.push_back(ui::OverlayPlane(fake_buffer_, nullptr));
  plane_manager_->InitForTest(kOnePlanePerCrtc, arraysize(kOnePlanePerCrtc),
                              default_crtcs_);
  EXPECT_FALSE(
      plane_manager_->AssignOverlayPlanes(&state_, assigns, 1, nullptr));
}

TEST_F(HardwareDisplayPlaneManagerTest, MultiplePlaneAssignment) {
  ui::OverlayPlaneList assigns;
  assigns.push_back(ui::OverlayPlane(fake_buffer_, nullptr));
  assigns.push_back(ui::OverlayPlane(fake_buffer_, nullptr));
  plane_manager_->InitForTest(kTwoPlanesPerCrtc, arraysize(kTwoPlanesPerCrtc),
                              default_crtcs_);
  EXPECT_TRUE(plane_manager_->AssignOverlayPlanes(&state_, assigns,
                                                  default_crtcs_[0], nullptr));
  EXPECT_EQ(2, plane_manager_->plane_count());
}

TEST_F(HardwareDisplayPlaneManagerTest, NotEnoughPlanes) {
  ui::OverlayPlaneList assigns;
  assigns.push_back(ui::OverlayPlane(fake_buffer_, nullptr));
  assigns.push_back(ui::OverlayPlane(fake_buffer_, nullptr));
  plane_manager_->InitForTest(kOnePlanePerCrtc, arraysize(kOnePlanePerCrtc),
                              default_crtcs_);

  EXPECT_FALSE(plane_manager_->AssignOverlayPlanes(&state_, assigns,
                                                   default_crtcs_[0], nullptr));
}

TEST_F(HardwareDisplayPlaneManagerTest, MultipleCrtcs) {
  ui::OverlayPlaneList assigns;
  assigns.push_back(ui::OverlayPlane(fake_buffer_, nullptr));
  plane_manager_->InitForTest(kOnePlanePerCrtc, arraysize(kOnePlanePerCrtc),
                              default_crtcs_);

  EXPECT_TRUE(plane_manager_->AssignOverlayPlanes(&state_, assigns,
                                                  default_crtcs_[0], nullptr));
  EXPECT_TRUE(plane_manager_->AssignOverlayPlanes(&state_, assigns,
                                                  default_crtcs_[1], nullptr));
  EXPECT_EQ(2, plane_manager_->plane_count());
}

TEST_F(HardwareDisplayPlaneManagerTest, MultiplePlanesAndCrtcs) {
  ui::OverlayPlaneList assigns;
  assigns.push_back(ui::OverlayPlane(fake_buffer_, nullptr));
  assigns.push_back(ui::OverlayPlane(fake_buffer_, nullptr));
  plane_manager_->InitForTest(kTwoPlanesPerCrtc, arraysize(kTwoPlanesPerCrtc),
                              default_crtcs_);
  EXPECT_TRUE(plane_manager_->AssignOverlayPlanes(&state_, assigns,
                                                  default_crtcs_[0], nullptr));
  EXPECT_TRUE(plane_manager_->AssignOverlayPlanes(&state_, assigns,
                                                  default_crtcs_[1], nullptr));
  EXPECT_EQ(4, plane_manager_->plane_count());
}

TEST_F(HardwareDisplayPlaneManagerTest, MultipleFrames) {
  ui::OverlayPlaneList assigns;
  assigns.push_back(ui::OverlayPlane(fake_buffer_, nullptr));
  plane_manager_->InitForTest(kTwoPlanesPerCrtc, arraysize(kTwoPlanesPerCrtc),
                              default_crtcs_);

  EXPECT_TRUE(plane_manager_->AssignOverlayPlanes(&state_, assigns,
                                                  default_crtcs_[0], nullptr));
  EXPECT_EQ(1, plane_manager_->plane_count());
  // Pretend we committed the frame.
  state_.plane_list.swap(state_.old_plane_list);
  plane_manager_->BeginFrame(&state_);
  ui::HardwareDisplayPlane* old_plane = state_.old_plane_list[0];
  // The same plane should be used.
  EXPECT_TRUE(plane_manager_->AssignOverlayPlanes(&state_, assigns,
                                                  default_crtcs_[0], nullptr));
  EXPECT_EQ(2, plane_manager_->plane_count());
  EXPECT_EQ(state_.plane_list[0], old_plane);
}

TEST_F(HardwareDisplayPlaneManagerTest, MultipleFramesDifferentPlanes) {
  ui::OverlayPlaneList assigns;
  assigns.push_back(ui::OverlayPlane(fake_buffer_, nullptr));
  plane_manager_->InitForTest(kTwoPlanesPerCrtc, arraysize(kTwoPlanesPerCrtc),
                              default_crtcs_);

  EXPECT_TRUE(plane_manager_->AssignOverlayPlanes(&state_, assigns,
                                                  default_crtcs_[0], nullptr));
  EXPECT_EQ(1, plane_manager_->plane_count());
  // The other plane should be used.
  EXPECT_TRUE(plane_manager_->AssignOverlayPlanes(&state_, assigns,
                                                  default_crtcs_[0], nullptr));
  EXPECT_EQ(2, plane_manager_->plane_count());
  EXPECT_NE(state_.plane_list[0], state_.plane_list[1]);
}

TEST_F(HardwareDisplayPlaneManagerTest, SharedPlanes) {
  ui::OverlayPlaneList assigns;
  scoped_refptr<ui::MockScanoutBuffer> buffer =
      new ui::MockScanoutBuffer(gfx::Size(1, 1));

  assigns.push_back(ui::OverlayPlane(fake_buffer_, nullptr));
  assigns.push_back(ui::OverlayPlane(buffer, nullptr));
  plane_manager_->InitForTest(kOnePlanePerCrtcWithShared,
                              arraysize(kOnePlanePerCrtcWithShared),
                              default_crtcs_);

  EXPECT_TRUE(plane_manager_->AssignOverlayPlanes(&state_, assigns,
                                                  default_crtcs_[1], nullptr));
  EXPECT_EQ(2, plane_manager_->plane_count());
  // The shared plane is now unavailable for use by the other CRTC.
  EXPECT_FALSE(plane_manager_->AssignOverlayPlanes(&state_, assigns,
                                                   default_crtcs_[0], nullptr));
}

TEST_F(HardwareDisplayPlaneManagerTest, CheckFramebufferFormatMatch) {
  ui::OverlayPlaneList assigns;
  scoped_refptr<ui::MockScanoutBuffer> buffer =
      new ui::MockScanoutBuffer(kDefaultBufferSize, kDummyFormat);
  assigns.push_back(ui::OverlayPlane(buffer, nullptr));
  plane_manager_->InitForTest(kOnePlanePerCrtc, arraysize(kOnePlanePerCrtc),
                              default_crtcs_);
  plane_manager_->BeginFrame(&state_);
  // This should return false as plane manager creates planes which support
  // DRM_FORMAT_XRGB8888 while buffer returns kDummyFormat as its pixelFormat.
  EXPECT_FALSE(plane_manager_->AssignOverlayPlanes(&state_, assigns,
                                                   default_crtcs_[0], nullptr));
  assigns.clear();
  scoped_refptr<ui::MockScanoutBuffer> xrgb_buffer =
      new ui::MockScanoutBuffer(kDefaultBufferSize);
  assigns.push_back(ui::OverlayPlane(xrgb_buffer, nullptr));
  plane_manager_->BeginFrame(&state_);
  EXPECT_TRUE(plane_manager_->AssignOverlayPlanes(&state_, assigns,
                                                  default_crtcs_[0], nullptr));
  plane_manager_->BeginFrame(&state_);
  EXPECT_FALSE(plane_manager_->AssignOverlayPlanes(&state_, assigns,
                                                   default_crtcs_[0], nullptr));
}

TEST(HardwareDisplayPlaneManagerLegacyTest, UnusedPlanesAreReleased) {
  std::vector<uint32_t> crtcs;
  crtcs.push_back(100);

  scoped_refptr<ui::MockDrmDevice> drm = new ui::MockDrmDevice(false, crtcs, 2);
  ui::OverlayPlaneList assigns;
  scoped_refptr<ui::MockScanoutBuffer> primary_buffer =
      new ui::MockScanoutBuffer(kDefaultBufferSize);
  scoped_refptr<ui::MockScanoutBuffer> overlay_buffer =
      new ui::MockScanoutBuffer(gfx::Size(1, 1));
  assigns.push_back(ui::OverlayPlane(primary_buffer, nullptr));
  assigns.push_back(ui::OverlayPlane(overlay_buffer, nullptr));
  ui::HardwareDisplayPlaneList hdpl;
  ui::CrtcController crtc(drm, crtcs[0], 0);
  drm->plane_manager()->BeginFrame(&hdpl);
  EXPECT_TRUE(drm->plane_manager()->AssignOverlayPlanes(&hdpl, assigns,
                                                        crtcs[0], &crtc));
  EXPECT_TRUE(drm->plane_manager()->Commit(&hdpl, false));
  assigns.clear();
  assigns.push_back(ui::OverlayPlane(primary_buffer, nullptr));
  drm->plane_manager()->BeginFrame(&hdpl);
  EXPECT_TRUE(drm->plane_manager()->AssignOverlayPlanes(&hdpl, assigns,
                                                        crtcs[0], &crtc));
  EXPECT_EQ(0, drm->get_overlay_clear_call_count());
  EXPECT_TRUE(drm->plane_manager()->Commit(&hdpl, false));
  EXPECT_EQ(1, drm->get_overlay_clear_call_count());
}

class FakeFenceFD {
 public:
  FakeFenceFD();

  gfx::GpuFence* GetGpuFence() const;
  void Signal() const;

 private:
  base::ScopedFD read_fd;
  base::ScopedFD write_fd;
  std::unique_ptr<gfx::GpuFence> gpu_fence;
};

FakeFenceFD::FakeFenceFD() {
  int fds[2];
  base::CreateLocalNonBlockingPipe(fds);
  read_fd = base::ScopedFD(fds[0]);
  write_fd = base::ScopedFD(fds[1]);

  gfx::GpuFenceHandle handle;
  handle.type = gfx::GpuFenceHandleType::kAndroidNativeFenceSync;
  handle.native_fd =
      base::FileDescriptor(HANDLE_EINTR(dup(read_fd.get())), true);
  gpu_fence = std::make_unique<gfx::GpuFence>(handle);
}

gfx::GpuFence* FakeFenceFD::GetGpuFence() const {
  return gpu_fence.get();
}

void FakeFenceFD::Signal() const {
  base::WriteFileDescriptor(write_fd.get(), "a", 1);
}

class HardwareDisplayPlaneManagerPlanesReadyTest : public testing::Test {
 protected:
  HardwareDisplayPlaneManagerPlanesReadyTest() = default;

  void UseLegacyManager();
  void UseAtomicManager();
  void RequestPlanesReady(const ui::OverlayPlaneList& planes);

  std::unique_ptr<ui::HardwareDisplayPlaneManager> plane_manager_;
  bool callback_called = false;
  base::test::ScopedTaskEnvironment task_env_{
      base::test::ScopedTaskEnvironment::MainThreadType::DEFAULT,
      base::test::ScopedTaskEnvironment::ExecutionMode::QUEUED};
  const scoped_refptr<ui::ScanoutBuffer> scanout_buffer{
      new ui::MockScanoutBuffer(kDefaultBufferSize)};
  const FakeFenceFD fake_fence_fd1;
  const FakeFenceFD fake_fence_fd2;

  const ui::OverlayPlaneList planes_without_fences_{
      ui::OverlayPlane(scanout_buffer, nullptr),
      ui::OverlayPlane(scanout_buffer, nullptr)};
  const ui::OverlayPlaneList planes_with_fences_{
      ui::OverlayPlane(scanout_buffer, fake_fence_fd1.GetGpuFence()),
      ui::OverlayPlane(scanout_buffer, fake_fence_fd2.GetGpuFence())};

 private:
  DISALLOW_COPY_AND_ASSIGN(HardwareDisplayPlaneManagerPlanesReadyTest);
};

void HardwareDisplayPlaneManagerPlanesReadyTest::RequestPlanesReady(
    const ui::OverlayPlaneList& planes) {
  auto set_true = [](bool* b) { *b = true; };
  plane_manager_->RequestPlanesReadyCallback(
      planes, base::BindOnce(set_true, &callback_called));
}

void HardwareDisplayPlaneManagerPlanesReadyTest::UseLegacyManager() {
  plane_manager_ = std::make_unique<ui::HardwareDisplayPlaneManagerLegacy>();
}

void HardwareDisplayPlaneManagerPlanesReadyTest::UseAtomicManager() {
  plane_manager_ = std::make_unique<ui::HardwareDisplayPlaneManagerAtomic>();
}

TEST_F(HardwareDisplayPlaneManagerPlanesReadyTest,
       LegacyWithoutFencesIsAsynchronousWithoutFenceWait) {
  UseLegacyManager();
  RequestPlanesReady(planes_without_fences_);

  EXPECT_FALSE(callback_called);

  task_env_.RunUntilIdle();

  EXPECT_TRUE(callback_called);
}

TEST_F(HardwareDisplayPlaneManagerPlanesReadyTest,
       LegacyWithFencesIsAsynchronousWithFenceWait) {
  UseLegacyManager();
  RequestPlanesReady(planes_with_fences_);

  EXPECT_FALSE(callback_called);

  fake_fence_fd1.Signal();
  fake_fence_fd2.Signal();

  EXPECT_FALSE(callback_called);

  task_env_.RunUntilIdle();

  EXPECT_TRUE(callback_called);
}

TEST_F(HardwareDisplayPlaneManagerPlanesReadyTest,
       AtomicWithoutFencesIsAsynchronousWithoutFenceWait) {
  UseAtomicManager();
  RequestPlanesReady(planes_without_fences_);

  EXPECT_FALSE(callback_called);

  task_env_.RunUntilIdle();

  EXPECT_TRUE(callback_called);
}

TEST_F(HardwareDisplayPlaneManagerPlanesReadyTest,
       AtomicWithFencesIsAsynchronousWithoutFenceWait) {
  UseAtomicManager();
  RequestPlanesReady(planes_with_fences_);

  EXPECT_FALSE(callback_called);

  task_env_.RunUntilIdle();

  EXPECT_TRUE(callback_called);
}

class HardwareDisplayPlaneAtomicMock : public ui::HardwareDisplayPlaneAtomic {
 public:
  HardwareDisplayPlaneAtomicMock() : ui::HardwareDisplayPlaneAtomic(0, ~0) {}
  ~HardwareDisplayPlaneAtomicMock() override {}

  bool SetPlaneData(drmModeAtomicReq* property_set,
                    uint32_t crtc_id,
                    uint32_t framebuffer,
                    const gfx::Rect& crtc_rect,
                    const gfx::Rect& src_rect,
                    const gfx::OverlayTransform transform,
                    int in_fence_fd) override {
    framebuffer_ = framebuffer;
    return true;
  }
  uint32_t framebuffer() const { return framebuffer_; }

 private:
  uint32_t framebuffer_ = 0;
};

TEST(HardwareDisplayPlaneManagerAtomic, EnableBlend) {
  auto plane_manager =
      std::make_unique<ui::HardwareDisplayPlaneManagerAtomic>();
  ui::HardwareDisplayPlaneList plane_list;
  HardwareDisplayPlaneAtomicMock hw_plane;
  scoped_refptr<ui::ScanoutBuffer> buffer =
      new ui::MockScanoutBuffer(kDefaultBufferSize);
  ui::OverlayPlane overlay(buffer, nullptr);
  overlay.enable_blend = true;
  plane_manager->SetPlaneData(&plane_list, &hw_plane, overlay, 1, gfx::Rect(),
                              nullptr);
  EXPECT_EQ(hw_plane.framebuffer(), buffer->GetFramebufferId());

  overlay.enable_blend = false;
  plane_manager->SetPlaneData(&plane_list, &hw_plane, overlay, 1, gfx::Rect(),
                              nullptr);
  EXPECT_EQ(hw_plane.framebuffer(), buffer->GetOpaqueFramebufferId());
}

}  // namespace
