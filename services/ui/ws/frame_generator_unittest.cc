// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/ui/ws/frame_generator.h"

#include "base/macros.h"
#include "components/viz/common/frame_sinks/begin_frame_source.h"
#include "components/viz/common/quads/render_pass.h"
#include "components/viz/test/begin_frame_args_test.h"
#include "components/viz/test/fake_external_begin_frame_source.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ui {
namespace ws {
namespace test {

namespace {

constexpr float kRefreshRate = 0.f;
constexpr bool kTickAutomatically = false;
constexpr float kDefaultScaleFactor = 1.0f;
constexpr float kArbitraryScaleFactor = 0.5f;
constexpr gfx::Size kArbitrarySize(3, 4);
constexpr gfx::Size kAnotherArbitrarySize(5, 6);
const viz::SurfaceId kArbitrarySurfaceId(
    viz::FrameSinkId(1, 1),
    viz::LocalSurfaceId(1, base::UnguessableToken::Create()));
const viz::SurfaceInfo kArbitrarySurfaceInfo(kArbitrarySurfaceId,
                                             1.0f,
                                             gfx::Size(100, 100));
}

// TestClientBinding Observes a BeginFrame and accepts CompositorFrame submitted
// from FrameGenerator. It provides a way to inspect CompositorFrames.
class TestClientBinding : public viz::mojom::CompositorFrameSink,
                          public viz::BeginFrameObserver {
 public:
  explicit TestClientBinding(viz::mojom::CompositorFrameSinkClient* sink_client)
      : sink_client_(sink_client) {}
  ~TestClientBinding() override = default;

  // viz::mojom::CompositorFrameSink implementation:
  void SubmitCompositorFrame(
      const viz::LocalSurfaceId& local_surface_id,
      viz::CompositorFrame frame,
      base::Optional<viz::HitTestRegionList> hit_test_region_list,
      uint64_t submit_time) override {
    ++frames_submitted_;
    last_frame_ = std::move(frame);
    last_begin_frame_ack_ = last_frame_.metadata.begin_frame_ack;
  }

  void SubmitCompositorFrameSync(
      const viz::LocalSurfaceId& local_surface_id,
      viz::CompositorFrame frame,
      base::Optional<viz::HitTestRegionList> hit_test_region_list,
      uint64_t submit_time,
      SubmitCompositorFrameSyncCallback callback) override {}

  void DidNotProduceFrame(const viz::BeginFrameAck& ack) override {
    last_begin_frame_ack_ = ack;
  }

  void DidAllocateSharedBitmap(mojo::ScopedSharedBufferHandle buffer,
                               const viz::SharedBitmapId& id) override {}

  void DidDeleteSharedBitmap(const viz::SharedBitmapId& id) override {}

  void SetNeedsBeginFrame(bool needs_begin_frame) override {
    if (needs_begin_frame == observing_begin_frames_)
      return;

    observing_begin_frames_ = needs_begin_frame;
    if (needs_begin_frame) {
      begin_frame_source_->AddObserver(this);
    } else
      begin_frame_source_->RemoveObserver(this);
  }

  void SetWantsAnimateOnlyBeginFrames() override {}

  // viz::BeginFrameObserver implementation.
  void OnBeginFrame(const viz::BeginFrameArgs& args) override {
    sink_client_->OnBeginFrame(args);
    last_begin_frame_args_ = args;
  }

  const viz::BeginFrameArgs& LastUsedBeginFrameArgs() const override {
    return last_begin_frame_args_;
  }

  bool WantsAnimateOnlyBeginFrames() const override { return false; }

  void OnBeginFrameSourcePausedChanged(bool paused) override {}

  void SetBeginFrameSource(viz::BeginFrameSource* begin_frame_source) {
    begin_frame_source_ = begin_frame_source;
  }

  const viz::RenderPassList& last_render_pass_list() const {
    return last_frame_.render_pass_list;
  }

  const viz::CompositorFrameMetadata& last_metadata() const {
    return last_frame_.metadata;
  }

  int frames_submitted() const { return frames_submitted_; }

  const viz::BeginFrameAck& last_begin_frame_ack() const {
    return last_begin_frame_ack_;
  }

 private:
  viz::mojom::CompositorFrameSinkClient* sink_client_;
  viz::BeginFrameArgs last_begin_frame_args_;
  viz::CompositorFrame last_frame_;
  viz::BeginFrameSource* begin_frame_source_ = nullptr;
  bool observing_begin_frames_ = false;
  int frames_submitted_ = 0;
  viz::BeginFrameAck last_begin_frame_ack_;
};

class FrameGeneratorTest : public testing::Test {
 public:
  FrameGeneratorTest() = default;
  ~FrameGeneratorTest() override = default;

  // testing::Test overrides:
  void SetUp() override {
    testing::Test::SetUp();

    frame_generator_ = std::make_unique<FrameGenerator>();
    begin_frame_source_ = std::make_unique<viz::FakeExternalBeginFrameSource>(
        kRefreshRate, kTickAutomatically);

    // FrameGenerator requires a valid SurfaceInfo before generating
    // CompositorFrames.
    std::unique_ptr<TestClientBinding> client_binding =
        std::make_unique<TestClientBinding>(frame_generator_.get());
    binding_ = client_binding.get();
    IssueBeginFrame();

    // FrameGenerator does not request BeginFrames right after creation.
    EXPECT_EQ(0, NumberOfFramesReceived());
    client_binding->SetBeginFrameSource(begin_frame_source_.get());
    frame_generator_->Bind(std::move(client_binding));
  };

  // InitWithSurfaceInfo creates a TestClientBinding and binds it to
  // |frame_generator_|. After InitWithSurfaceInfo finishes, |frame_generator_|
  // has a valid SurfaceInfo and does not request BeginFrames.
  void InitWithSurfaceInfo() {
    frame_generator_->SetEmbeddedSurface(kArbitrarySurfaceInfo);

    // Issue a BeginFrame so that frame_generator_ stops requesting BeginFrames
    // after submitting a CompositorFrame.
    IssueBeginFrame();
    EXPECT_EQ(1, NumberOfFramesReceived());
  }

  void IssueBeginFrame() {
    begin_frame_source_->TestOnBeginFrame(viz::CreateBeginFrameArgsForTesting(
        BEGINFRAME_FROM_HERE, 0, next_sequence_number_));
    ++next_sequence_number_;
  }

  int NumberOfFramesReceived() const { return binding_->frames_submitted(); }

  const viz::BeginFrameAck& LastBeginFrameAck() const {
    return binding_->last_begin_frame_ack();
  }

  const viz::CompositorFrameMetadata& LastMetadata() const {
    return binding_->last_metadata();
  }

  const viz::RenderPassList& LastRenderPassList() const {
    return binding_->last_render_pass_list();
  }

  FrameGenerator* frame_generator() { return frame_generator_.get(); }
  viz::BeginFrameSource* begin_frame_source() {
    return begin_frame_source_.get();
  }

  TestClientBinding* binding() { return binding_; }

 private:
  std::unique_ptr<viz::FakeExternalBeginFrameSource> begin_frame_source_;
  std::unique_ptr<FrameGenerator> frame_generator_;
  TestClientBinding* binding_ = nullptr;
  int next_sequence_number_ = 1;

  DISALLOW_COPY_AND_ASSIGN(FrameGeneratorTest);
};

// FrameGenerator has an invalid SurfaceInfo as default. FrameGenerator does not
// submit CompositorFrames when its SurfaceInfo is invalid.
TEST_F(FrameGeneratorTest, InvalidSurfaceInfo) {
  IssueBeginFrame();
  EXPECT_EQ(0, NumberOfFramesReceived());
  EXPECT_EQ(viz::BeginFrameAck(), LastBeginFrameAck());
}

TEST_F(FrameGeneratorTest, OnFirstSurfaceActivation) {
  InitWithSurfaceInfo();

  // Verify that the CompositorFrame refers to the window manager's surface via
  // referenced_surfaces.
  const viz::CompositorFrameMetadata& last_metadata = LastMetadata();
  const std::vector<viz::SurfaceId>& referenced_surfaces =
      last_metadata.referenced_surfaces;
  EXPECT_EQ(1lu, referenced_surfaces.size());
  EXPECT_EQ(kArbitrarySurfaceId, referenced_surfaces.front());

  viz::BeginFrameAck expected_ack(0, 2, true);
  EXPECT_EQ(expected_ack, LastBeginFrameAck());
  EXPECT_EQ(expected_ack, last_metadata.begin_frame_ack);

  // FrameGenerator stops requesting BeginFrames after submitting a
  // CompositorFrame.
  IssueBeginFrame();
  EXPECT_EQ(expected_ack, LastBeginFrameAck());
}

TEST_F(FrameGeneratorTest, SetDeviceScaleFactor) {
  InitWithSurfaceInfo();

  // FrameGenerator does not request BeginFrames if its device scale factor
  // remains unchanged.
  frame_generator()->SetDeviceScaleFactor(kDefaultScaleFactor);
  IssueBeginFrame();
  EXPECT_EQ(1, NumberOfFramesReceived());
  const viz::CompositorFrameMetadata& last_metadata = LastMetadata();
  EXPECT_EQ(kDefaultScaleFactor, last_metadata.device_scale_factor);

  frame_generator()->SetDeviceScaleFactor(kArbitraryScaleFactor);
  IssueBeginFrame();
  EXPECT_EQ(2, NumberOfFramesReceived());
  const viz::CompositorFrameMetadata& second_last_metadata = LastMetadata();
  EXPECT_EQ(kArbitraryScaleFactor, second_last_metadata.device_scale_factor);
}

TEST_F(FrameGeneratorTest, SetHighContrastMode) {
  InitWithSurfaceInfo();

  // Changing high contrast mode should trigger a BeginFrame.
  frame_generator()->SetHighContrastMode(true);
  IssueBeginFrame();
  EXPECT_EQ(2, NumberOfFramesReceived());

  // Verify that the last frame has an invert filter.
  const viz::RenderPassList& render_pass_list = LastRenderPassList();
  const cc::FilterOperations expected_filters(
      {cc::FilterOperation::CreateInvertFilter(1.f)});
  EXPECT_EQ(expected_filters, render_pass_list.front()->filters);
}

TEST_F(FrameGeneratorTest, WindowBoundsChanged) {
  InitWithSurfaceInfo();

  // Window bounds change triggers a BeginFrame.
  constexpr viz::RenderPassId expected_render_pass_id = 1u;
  frame_generator()->OnWindowSizeChanged(kArbitrarySize);
  IssueBeginFrame();
  EXPECT_EQ(2, NumberOfFramesReceived());
  viz::RenderPass* received_render_pass = LastRenderPassList().front().get();
  EXPECT_EQ(expected_render_pass_id, received_render_pass->id);
  EXPECT_EQ(kArbitrarySize, received_render_pass->output_rect.size());
  EXPECT_EQ(kArbitrarySize, received_render_pass->damage_rect.size());
  EXPECT_EQ(gfx::Transform(), received_render_pass->transform_to_root_target);
}

// Change window bounds twice before issuing a BeginFrame. The CompositorFrame
// submitted by frame_generator() should only has the second bounds.
TEST_F(FrameGeneratorTest, WindowBoundsChangedTwice) {
  InitWithSurfaceInfo();

  frame_generator()->OnWindowSizeChanged(kArbitrarySize);
  frame_generator()->OnWindowSizeChanged(kAnotherArbitrarySize);
  IssueBeginFrame();
  EXPECT_EQ(2, NumberOfFramesReceived());
  viz::RenderPass* received_render_pass = LastRenderPassList().front().get();
  EXPECT_EQ(kAnotherArbitrarySize, received_render_pass->output_rect.size());
  EXPECT_EQ(kAnotherArbitrarySize, received_render_pass->damage_rect.size());

  // frame_generator() stops requesting BeginFrames after getting one.
  IssueBeginFrame();
  EXPECT_EQ(2, NumberOfFramesReceived());
}

TEST_F(FrameGeneratorTest, WindowDamaged) {
  InitWithSurfaceInfo();

  frame_generator()->OnWindowDamaged();
  IssueBeginFrame();
  EXPECT_EQ(2, NumberOfFramesReceived());
}

}  // namespace test
}  // namespace ws
}  // namespace ui
