// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>
#include <stdint.h>

#include "base/bind.h"
#include "base/macros.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "ui/gfx/native_pixmap.h"
#include "ui/gfx/presentation_feedback.h"
#include "ui/ozone/platform/drm/gpu/crtc_controller.h"
#include "ui/ozone/platform/drm/gpu/hardware_display_controller.h"
#include "ui/ozone/platform/drm/gpu/mock_drm_device.h"
#include "ui/ozone/platform/drm/gpu/mock_scanout_buffer.h"

namespace {

// Create a basic mode for a 6x4 screen.
const drmModeModeInfo kDefaultMode =
    {0, 6, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, {'\0'}};

const uint32_t kPrimaryCrtc = 1;
const uint32_t kPrimaryConnector = 2;
const uint32_t kSecondaryCrtc = 3;
const uint32_t kSecondaryConnector = 4;
const size_t kPlanesPerCrtc = 2;

const gfx::Size kDefaultModeSize(kDefaultMode.hdisplay, kDefaultMode.vdisplay);
const gfx::Size kOverlaySize(kDefaultMode.hdisplay / 2,
                             kDefaultMode.vdisplay / 2);
const gfx::SizeF kDefaultModeSizeF(1.0, 1.0);

}  // namespace

class HardwareDisplayControllerTest : public testing::Test {
 public:
  HardwareDisplayControllerTest() : page_flips_(0) {}
  ~HardwareDisplayControllerTest() override {}

  void SetUp() override;
  void TearDown() override;

  void PageFlipCallback(gfx::SwapResult result,
                        const gfx::PresentationFeedback& feedback);

 protected:
  std::unique_ptr<ui::HardwareDisplayController> controller_;
  scoped_refptr<ui::MockDrmDevice> drm_;

  int page_flips_;
  gfx::SwapResult last_swap_result_;
  gfx::PresentationFeedback last_presentation_feedback_;

 private:
  DISALLOW_COPY_AND_ASSIGN(HardwareDisplayControllerTest);
};

void HardwareDisplayControllerTest::SetUp() {
  page_flips_ = 0;
  last_swap_result_ = gfx::SwapResult::SWAP_FAILED;

  std::vector<uint32_t> crtcs;
  crtcs.push_back(kPrimaryCrtc);
  crtcs.push_back(kSecondaryCrtc);
  drm_ = new ui::MockDrmDevice(false, crtcs, kPlanesPerCrtc);
  controller_.reset(new ui::HardwareDisplayController(
      std::unique_ptr<ui::CrtcController>(
          new ui::CrtcController(drm_.get(), kPrimaryCrtc, kPrimaryConnector)),
      gfx::Point()));
}

void HardwareDisplayControllerTest::TearDown() {
  controller_.reset();
  drm_ = nullptr;
}

void HardwareDisplayControllerTest::PageFlipCallback(
    gfx::SwapResult result,
    const gfx::PresentationFeedback& feedback) {
  page_flips_++;
  last_swap_result_ = result;
  last_presentation_feedback_ = feedback;
}

TEST_F(HardwareDisplayControllerTest, CheckModesettingResult) {
  ui::OverlayPlane plane(scoped_refptr<ui::ScanoutBuffer>(
                             new ui::MockScanoutBuffer(kDefaultModeSize)),
                         nullptr);

  EXPECT_TRUE(controller_->Modeset(plane, kDefaultMode));
  EXPECT_FALSE(plane.buffer->HasOneRef());
}

TEST_F(HardwareDisplayControllerTest, CheckStateAfterPageFlip) {
  ui::OverlayPlane plane1(scoped_refptr<ui::ScanoutBuffer>(
                              new ui::MockScanoutBuffer(kDefaultModeSize)),
                          nullptr);

  EXPECT_TRUE(controller_->Modeset(plane1, kDefaultMode));

  ui::OverlayPlane plane2(scoped_refptr<ui::ScanoutBuffer>(
                              new ui::MockScanoutBuffer(kDefaultModeSize)),
                          nullptr);
  std::vector<ui::OverlayPlane> planes =
      std::vector<ui::OverlayPlane>(1, plane2);
  controller_->SchedulePageFlip(
      planes, base::BindOnce(&HardwareDisplayControllerTest::PageFlipCallback,
                             base::Unretained(this)));
  drm_->RunCallbacks();
  EXPECT_TRUE(plane1.buffer->HasOneRef());
  EXPECT_FALSE(plane2.buffer->HasOneRef());

  EXPECT_EQ(gfx::SwapResult::SWAP_ACK, last_swap_result_);
  EXPECT_EQ(1, page_flips_);
  EXPECT_EQ(1, drm_->get_page_flip_call_count());
  EXPECT_EQ(0, drm_->get_overlay_flip_call_count());
}

TEST_F(HardwareDisplayControllerTest, CheckStateIfModesetFails) {
  drm_->set_set_crtc_expectation(false);

  ui::OverlayPlane plane(scoped_refptr<ui::ScanoutBuffer>(
                             new ui::MockScanoutBuffer(kDefaultModeSize)),
                         nullptr);

  EXPECT_FALSE(controller_->Modeset(plane, kDefaultMode));
}

TEST_F(HardwareDisplayControllerTest, CheckStateIfPageFlipFails) {
  drm_->set_page_flip_expectation(false);

  ui::OverlayPlane plane1(scoped_refptr<ui::ScanoutBuffer>(
                              new ui::MockScanoutBuffer(kDefaultModeSize)),
                          nullptr);

  EXPECT_TRUE(controller_->Modeset(plane1, kDefaultMode));

  ui::OverlayPlane plane2(scoped_refptr<ui::ScanoutBuffer>(
                              new ui::MockScanoutBuffer(kDefaultModeSize)),
                          nullptr);
  std::vector<ui::OverlayPlane> planes =
      std::vector<ui::OverlayPlane>(1, plane2);
  controller_->SchedulePageFlip(
      planes, base::BindOnce(&HardwareDisplayControllerTest::PageFlipCallback,
                             base::Unretained(this)));
  drm_->RunCallbacks();
  planes.clear();

  EXPECT_EQ(gfx::SwapResult::SWAP_FAILED, last_swap_result_);
  EXPECT_EQ(1, page_flips_);
  EXPECT_FALSE(plane1.buffer->HasOneRef());
  EXPECT_TRUE(plane2.buffer->HasOneRef());
}

TEST_F(HardwareDisplayControllerTest, CheckOverlayPresent) {
  ui::OverlayPlane plane1(scoped_refptr<ui::ScanoutBuffer>(
                              new ui::MockScanoutBuffer(kDefaultModeSize)),
                          nullptr);
  ui::OverlayPlane plane2(
      scoped_refptr<ui::ScanoutBuffer>(new ui::MockScanoutBuffer(kOverlaySize)),
      1, gfx::OVERLAY_TRANSFORM_NONE, gfx::Rect(kOverlaySize),
      gfx::RectF(kDefaultModeSizeF), true, nullptr);

  EXPECT_TRUE(controller_->Modeset(plane1, kDefaultMode));

  std::vector<ui::OverlayPlane> planes;
  planes.push_back(plane1);
  planes.push_back(plane2);

  controller_->SchedulePageFlip(
      planes, base::BindOnce(&HardwareDisplayControllerTest::PageFlipCallback,
                             base::Unretained(this)));
  drm_->RunCallbacks();
  EXPECT_EQ(gfx::SwapResult::SWAP_ACK, last_swap_result_);
  EXPECT_EQ(1, page_flips_);
  EXPECT_EQ(1, drm_->get_page_flip_call_count());
  EXPECT_EQ(1, drm_->get_overlay_flip_call_count());
}

TEST_F(HardwareDisplayControllerTest, CheckOverlayTestMode) {
  ui::OverlayPlane plane1(scoped_refptr<ui::ScanoutBuffer>(
                              new ui::MockScanoutBuffer(kDefaultModeSize)),
                          nullptr);
  ui::OverlayPlane plane2(
      scoped_refptr<ui::ScanoutBuffer>(new ui::MockScanoutBuffer(kOverlaySize)),
      1, gfx::OVERLAY_TRANSFORM_NONE, gfx::Rect(kOverlaySize),
      gfx::RectF(kDefaultModeSizeF), true, nullptr);

  EXPECT_TRUE(controller_->Modeset(plane1, kDefaultMode));

  std::vector<ui::OverlayPlane> planes;
  planes.push_back(plane1);
  planes.push_back(plane2);

  controller_->SchedulePageFlip(
      planes, base::BindOnce(&HardwareDisplayControllerTest::PageFlipCallback,
                             base::Unretained(this)));

  // A test call shouldn't cause new flips, but should succeed.
  EXPECT_TRUE(controller_->TestPageFlip(planes));
  drm_->RunCallbacks();
  EXPECT_EQ(gfx::SwapResult::SWAP_ACK, last_swap_result_);
  EXPECT_EQ(1, page_flips_);
  EXPECT_EQ(1, drm_->get_page_flip_call_count());
  EXPECT_EQ(1, drm_->get_overlay_flip_call_count());

  // Regular flips should continue on normally.
  controller_->SchedulePageFlip(
      planes, base::BindOnce(&HardwareDisplayControllerTest::PageFlipCallback,
                             base::Unretained(this)));
  drm_->RunCallbacks();
  EXPECT_EQ(gfx::SwapResult::SWAP_ACK, last_swap_result_);
  EXPECT_EQ(2, page_flips_);
  EXPECT_EQ(2, drm_->get_page_flip_call_count());
  EXPECT_EQ(2, drm_->get_overlay_flip_call_count());
}

TEST_F(HardwareDisplayControllerTest, AcceptUnderlays) {
  ui::OverlayPlane plane1(scoped_refptr<ui::ScanoutBuffer>(
                              new ui::MockScanoutBuffer(kDefaultModeSize)),
                          nullptr);
  ui::OverlayPlane plane2(scoped_refptr<ui::ScanoutBuffer>(
                              new ui::MockScanoutBuffer(kDefaultModeSize)),
                          -1, gfx::OVERLAY_TRANSFORM_NONE,
                          gfx::Rect(kDefaultModeSize),
                          gfx::RectF(kDefaultModeSizeF), true, nullptr);

  EXPECT_TRUE(controller_->Modeset(plane1, kDefaultMode));

  std::vector<ui::OverlayPlane> planes;
  planes.push_back(plane1);
  planes.push_back(plane2);

  controller_->SchedulePageFlip(
      planes, base::BindOnce(&HardwareDisplayControllerTest::PageFlipCallback,
                             base::Unretained(this)));
  drm_->RunCallbacks();
  EXPECT_EQ(gfx::SwapResult::SWAP_ACK, last_swap_result_);
  EXPECT_EQ(1, page_flips_);
}

TEST_F(HardwareDisplayControllerTest, PageflipMirroredControllers) {
  controller_->AddCrtc(std::unique_ptr<ui::CrtcController>(
      new ui::CrtcController(drm_.get(), kSecondaryCrtc, kSecondaryConnector)));

  ui::OverlayPlane plane1(scoped_refptr<ui::ScanoutBuffer>(
                              new ui::MockScanoutBuffer(kDefaultModeSize)),
                          nullptr);
  EXPECT_TRUE(controller_->Modeset(plane1, kDefaultMode));
  EXPECT_EQ(2, drm_->get_set_crtc_call_count());

  ui::OverlayPlane plane2(scoped_refptr<ui::ScanoutBuffer>(
                              new ui::MockScanoutBuffer(kDefaultModeSize)),
                          nullptr);
  std::vector<ui::OverlayPlane> planes =
      std::vector<ui::OverlayPlane>(1, plane2);
  controller_->SchedulePageFlip(
      planes, base::BindOnce(&HardwareDisplayControllerTest::PageFlipCallback,
                             base::Unretained(this)));
  drm_->RunCallbacks();
  EXPECT_EQ(gfx::SwapResult::SWAP_ACK, last_swap_result_);
  EXPECT_EQ(1, page_flips_);
  EXPECT_EQ(2, drm_->get_page_flip_call_count());
  EXPECT_EQ(1, page_flips_);
}

TEST_F(HardwareDisplayControllerTest, PlaneStateAfterRemoveCrtc) {
  ui::OverlayPlane plane1(scoped_refptr<ui::ScanoutBuffer>(
                              new ui::MockScanoutBuffer(kDefaultModeSize)),
                          nullptr);
  EXPECT_TRUE(controller_->Modeset(plane1, kDefaultMode));
  std::vector<ui::OverlayPlane> planes =
      std::vector<ui::OverlayPlane>(1, plane1);
  controller_->SchedulePageFlip(
      planes, base::BindOnce(&HardwareDisplayControllerTest::PageFlipCallback,
                             base::Unretained(this)));
  drm_->RunCallbacks();
  EXPECT_EQ(gfx::SwapResult::SWAP_ACK, last_swap_result_);
  EXPECT_EQ(1, page_flips_);

  const ui::HardwareDisplayPlane* owned_plane = nullptr;
  for (const auto& plane : drm_->plane_manager()->planes()) {
    if (plane->in_use())
      owned_plane = plane.get();
  }

  ASSERT_TRUE(owned_plane != nullptr);
  EXPECT_EQ(kPrimaryCrtc, owned_plane->owning_crtc());

  // Removing the crtc should not free the plane or change ownership.
  std::unique_ptr<ui::CrtcController> crtc =
      controller_->RemoveCrtc(drm_, kPrimaryCrtc);
  EXPECT_TRUE(owned_plane->in_use());
  EXPECT_EQ(kPrimaryCrtc, owned_plane->owning_crtc());
  // Check that controller doesn't affect the state of removed plane in
  // subsequent page flip.
  controller_->SchedulePageFlip(
      planes, base::BindOnce(&HardwareDisplayControllerTest::PageFlipCallback,
                             base::Unretained(this)));
  drm_->RunCallbacks();
  EXPECT_EQ(gfx::SwapResult::SWAP_ACK, last_swap_result_);
  EXPECT_EQ(1, page_flips_);
  EXPECT_TRUE(owned_plane->in_use());
  EXPECT_EQ(kPrimaryCrtc, owned_plane->owning_crtc());
}

TEST_F(HardwareDisplayControllerTest, PlaneStateAfterDestroyingCrtc) {
  ui::OverlayPlane plane1(scoped_refptr<ui::ScanoutBuffer>(
                              new ui::MockScanoutBuffer(kDefaultModeSize)),
                          nullptr);
  EXPECT_TRUE(controller_->Modeset(plane1, kDefaultMode));
  std::vector<ui::OverlayPlane> planes =
      std::vector<ui::OverlayPlane>(1, plane1);
  controller_->SchedulePageFlip(
      planes, base::BindOnce(&HardwareDisplayControllerTest::PageFlipCallback,
                             base::Unretained(this)));
  drm_->RunCallbacks();
  EXPECT_EQ(gfx::SwapResult::SWAP_ACK, last_swap_result_);
  EXPECT_EQ(1, page_flips_);

  const ui::HardwareDisplayPlane* owned_plane = nullptr;
  for (const auto& plane : drm_->plane_manager()->planes())
    if (plane->in_use())
      owned_plane = plane.get();
  ASSERT_TRUE(owned_plane != nullptr);
  EXPECT_EQ(kPrimaryCrtc, owned_plane->owning_crtc());
  std::unique_ptr<ui::CrtcController> crtc =
      controller_->RemoveCrtc(drm_, kPrimaryCrtc);
  // Destroying crtc should free the plane.
  crtc.reset();
  uint32_t crtc_nullid = 0;
  EXPECT_FALSE(owned_plane->in_use());
  EXPECT_EQ(crtc_nullid, owned_plane->owning_crtc());
}

TEST_F(HardwareDisplayControllerTest, PlaneStateAfterAddCrtc) {
  controller_->AddCrtc(std::unique_ptr<ui::CrtcController>(
      new ui::CrtcController(drm_.get(), kSecondaryCrtc, kSecondaryConnector)));

  ui::OverlayPlane plane1(scoped_refptr<ui::ScanoutBuffer>(
                              new ui::MockScanoutBuffer(kDefaultModeSize)),
                          nullptr);
  EXPECT_TRUE(controller_->Modeset(plane1, kDefaultMode));
  std::vector<ui::OverlayPlane> planes =
      std::vector<ui::OverlayPlane>(1, plane1);
  controller_->SchedulePageFlip(
      planes, base::BindOnce(&HardwareDisplayControllerTest::PageFlipCallback,
                             base::Unretained(this)));
  drm_->RunCallbacks();
  EXPECT_EQ(gfx::SwapResult::SWAP_ACK, last_swap_result_);
  EXPECT_EQ(1, page_flips_);

  ui::HardwareDisplayPlane* primary_crtc_plane = nullptr;
  for (const auto& plane : drm_->plane_manager()->planes()) {
    if (plane->in_use() && kPrimaryCrtc == plane->owning_crtc())
      primary_crtc_plane = plane.get();
  }

  ASSERT_TRUE(primary_crtc_plane != nullptr);

  std::unique_ptr<ui::HardwareDisplayController> hdc_controller;
  hdc_controller.reset(new ui::HardwareDisplayController(
      controller_->RemoveCrtc(drm_, kPrimaryCrtc), controller_->origin()));
  controller_->SchedulePageFlip(
      planes, base::BindOnce(&HardwareDisplayControllerTest::PageFlipCallback,
                             base::Unretained(this)));
  drm_->RunCallbacks();
  EXPECT_EQ(gfx::SwapResult::SWAP_ACK, last_swap_result_);
  EXPECT_EQ(2, page_flips_);
  EXPECT_TRUE(primary_crtc_plane->in_use());
  EXPECT_EQ(kPrimaryCrtc, primary_crtc_plane->owning_crtc());

  // We reset state of plane here to test that the plane was actually added to
  // hdc_controller. In which case, the right state should be set to plane
  // after page flip call is handled by the controller.
  primary_crtc_plane->set_in_use(false);
  primary_crtc_plane->set_owning_crtc(0);
  hdc_controller->SchedulePageFlip(
      planes, base::BindOnce(&HardwareDisplayControllerTest::PageFlipCallback,
                             base::Unretained(this)));
  drm_->RunCallbacks();
  EXPECT_EQ(gfx::SwapResult::SWAP_ACK, last_swap_result_);
  EXPECT_EQ(3, page_flips_);
  EXPECT_TRUE(primary_crtc_plane->in_use());
  EXPECT_EQ(kPrimaryCrtc, primary_crtc_plane->owning_crtc());
}

TEST_F(HardwareDisplayControllerTest, ModesetWhilePageFlipping) {
  ui::OverlayPlane plane1(scoped_refptr<ui::ScanoutBuffer>(
                              new ui::MockScanoutBuffer(kDefaultModeSize)),
                          nullptr);
  EXPECT_TRUE(controller_->Modeset(plane1, kDefaultMode));
  std::vector<ui::OverlayPlane> planes =
      std::vector<ui::OverlayPlane>(1, plane1);
  controller_->SchedulePageFlip(
      planes, base::BindOnce(&HardwareDisplayControllerTest::PageFlipCallback,
                             base::Unretained(this)));

  EXPECT_TRUE(controller_->Modeset(plane1, kDefaultMode));
  drm_->RunCallbacks();
  EXPECT_EQ(gfx::SwapResult::SWAP_ACK, last_swap_result_);
  EXPECT_EQ(1, page_flips_);
}

TEST_F(HardwareDisplayControllerTest, FailPageFlipping) {
  drm_->set_page_flip_expectation(false);

  ui::OverlayPlane plane1(scoped_refptr<ui::ScanoutBuffer>(
                              new ui::MockScanoutBuffer(kDefaultModeSize)),
                          nullptr);
  EXPECT_TRUE(controller_->Modeset(plane1, kDefaultMode));
  std::vector<ui::OverlayPlane> planes =
      std::vector<ui::OverlayPlane>(1, plane1);
  controller_->SchedulePageFlip(
      planes, base::BindOnce(&HardwareDisplayControllerTest::PageFlipCallback,
                             base::Unretained(this)));

  drm_->RunCallbacks();
  EXPECT_EQ(gfx::SwapResult::SWAP_FAILED, last_swap_result_);
  EXPECT_EQ(1, page_flips_);
}

TEST_F(HardwareDisplayControllerTest, CheckNoPrimaryPlane) {
  ui::OverlayPlane plane1(scoped_refptr<ui::ScanoutBuffer>(
                              new ui::MockScanoutBuffer(kDefaultModeSize)),
                          1, gfx::OVERLAY_TRANSFORM_NONE,
                          gfx::Rect(kDefaultModeSize), gfx::RectF(0, 0, 1, 1),
                          true, nullptr);
  EXPECT_TRUE(controller_->Modeset(plane1, kDefaultMode));
  std::vector<ui::OverlayPlane> planes =
      std::vector<ui::OverlayPlane>(1, plane1);
  controller_->SchedulePageFlip(
      planes, base::BindOnce(&HardwareDisplayControllerTest::PageFlipCallback,
                             base::Unretained(this)));

  drm_->RunCallbacks();
  EXPECT_EQ(gfx::SwapResult::SWAP_ACK, last_swap_result_);
  EXPECT_EQ(1, page_flips_);
}

TEST_F(HardwareDisplayControllerTest, AddCrtcMidPageFlip) {
  ui::OverlayPlane plane1(scoped_refptr<ui::ScanoutBuffer>(
                              new ui::MockScanoutBuffer(kDefaultModeSize)),
                          nullptr);
  EXPECT_TRUE(controller_->Modeset(plane1, kDefaultMode));
  std::vector<ui::OverlayPlane> planes =
      std::vector<ui::OverlayPlane>(1, plane1);
  controller_->SchedulePageFlip(
      planes, base::BindOnce(&HardwareDisplayControllerTest::PageFlipCallback,
                             base::Unretained(this)));

  controller_->AddCrtc(std::unique_ptr<ui::CrtcController>(
      new ui::CrtcController(drm_.get(), kSecondaryCrtc, kSecondaryConnector)));

  drm_->RunCallbacks();
  EXPECT_EQ(gfx::SwapResult::SWAP_ACK, last_swap_result_);
  EXPECT_EQ(1, page_flips_);
}

TEST_F(HardwareDisplayControllerTest, RemoveCrtcMidPageFlip) {
  ui::OverlayPlane plane1(scoped_refptr<ui::ScanoutBuffer>(
                              new ui::MockScanoutBuffer(kDefaultModeSize)),
                          nullptr);
  EXPECT_TRUE(controller_->Modeset(plane1, kDefaultMode));
  std::vector<ui::OverlayPlane> planes =
      std::vector<ui::OverlayPlane>(1, plane1);
  controller_->SchedulePageFlip(
      planes, base::BindOnce(&HardwareDisplayControllerTest::PageFlipCallback,
                             base::Unretained(this)));

  controller_->RemoveCrtc(drm_, kPrimaryCrtc);

  EXPECT_EQ(1, page_flips_);
  drm_->RunCallbacks();
  EXPECT_EQ(gfx::SwapResult::SWAP_ACK, last_swap_result_);
  EXPECT_EQ(1, page_flips_);
}

TEST_F(HardwareDisplayControllerTest, Disable) {
  ui::OverlayPlane plane1(scoped_refptr<ui::ScanoutBuffer>(
                              new ui::MockScanoutBuffer(kDefaultModeSize)),
                          nullptr);
  EXPECT_TRUE(controller_->Modeset(plane1, kDefaultMode));

  ui::OverlayPlane plane2(new ui::MockScanoutBuffer(kOverlaySize), 1,
                          gfx::OVERLAY_TRANSFORM_NONE, gfx::Rect(kOverlaySize),
                          gfx::RectF(kDefaultModeSizeF), true, nullptr);
  std::vector<ui::OverlayPlane> planes;
  planes.push_back(plane1);
  planes.push_back(plane2);

  controller_->SchedulePageFlip(
      planes, base::BindOnce(&HardwareDisplayControllerTest::PageFlipCallback,
                             base::Unretained(this)));
  drm_->RunCallbacks();
  EXPECT_EQ(gfx::SwapResult::SWAP_ACK, last_swap_result_);
  EXPECT_EQ(1, page_flips_);

  controller_->Disable();

  int planes_in_use = 0;
  for (const auto& plane : drm_->plane_manager()->planes()) {
    if (plane->in_use())
      planes_in_use++;
  }
  // Only the primary plane is in use.
  ASSERT_EQ(1, planes_in_use);
}
