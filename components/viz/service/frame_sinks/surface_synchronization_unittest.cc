// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/containers/flat_set.h"
#include "base/test/simple_test_tick_clock.h"
#include "components/viz/common/surfaces/surface_id.h"
#include "components/viz/service/frame_sinks/compositor_frame_sink_support.h"
#include "components/viz/service/frame_sinks/frame_sink_manager_impl.h"
#include "components/viz/test/begin_frame_args_test.h"
#include "components/viz/test/compositor_frame_helpers.h"
#include "components/viz/test/fake_external_begin_frame_source.h"
#include "components/viz/test/fake_surface_observer.h"
#include "components/viz/test/mock_compositor_frame_sink_client.h"
#include "components/viz/test/test_frame_sink_manager_client.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;
using testing::Eq;
using testing::IsEmpty;
using testing::UnorderedElementsAre;

namespace viz {
namespace test {
namespace {

constexpr bool kIsRoot = true;
constexpr bool kIsChildRoot = false;
constexpr bool kNeedsSyncPoints = true;
constexpr FrameSinkId kDisplayFrameSink(2, 0);
constexpr FrameSinkId kParentFrameSink(3, 0);
constexpr FrameSinkId kChildFrameSink1(65563, 0);
constexpr FrameSinkId kChildFrameSink2(65564, 0);
constexpr FrameSinkId kArbitraryFrameSink(1337, 7331);

std::vector<SurfaceId> empty_surface_ids() {
  return std::vector<SurfaceId>();
}

SurfaceId MakeSurfaceId(const FrameSinkId& frame_sink_id,
                        uint32_t parent_sequence_number,
                        uint32_t child_sequence_number = 1u) {
  return SurfaceId(frame_sink_id,
                   LocalSurfaceId(parent_sequence_number, child_sequence_number,
                                  base::UnguessableToken::Deserialize(0, 1u)));
}

CompositorFrame MakeCompositorFrame(
    std::vector<SurfaceId> activation_dependencies,
    std::vector<SurfaceId> referenced_surfaces,
    std::vector<TransferableResource> resource_list,
    const FrameDeadline& deadline = FrameDeadline()) {
  return CompositorFrameBuilder()
      .AddDefaultRenderPass()
      .SetActivationDependencies(std::move(activation_dependencies))
      .SetReferencedSurfaces(std::move(referenced_surfaces))
      .SetTransferableResources(std::move(resource_list))
      .SetDeadline(deadline)
      .Build();
}

}  // namespace

class FakeExternalBeginFrameSourceClient
    : public FakeExternalBeginFrameSource::Client {
 public:
  FakeExternalBeginFrameSourceClient() = default;
  ~FakeExternalBeginFrameSourceClient() = default;

  bool has_observers() const { return observer_count_ > 0; }

  // FakeExternalBeginFrameSource::Client implementation:
  void OnAddObserver(BeginFrameObserver* obs) override { ++observer_count_; }

  void OnRemoveObserver(BeginFrameObserver* obs) override {
    DCHECK_GT(observer_count_, 0);
    --observer_count_;
  }

 private:
  int observer_count_ = 0;

  DISALLOW_COPY_AND_ASSIGN(FakeExternalBeginFrameSourceClient);
};

class SurfaceSynchronizationTest : public testing::Test {
 public:
  SurfaceSynchronizationTest()
      : frame_sink_manager_client_(&frame_sink_manager_),
        surface_observer_(false) {}
  ~SurfaceSynchronizationTest() override {}

  CompositorFrameSinkSupport& display_support() {
    return *supports_[kDisplayFrameSink];
  }
  Surface* display_surface() {
    return display_support().GetLastCreatedSurfaceForTesting();
  }

  CompositorFrameSinkSupport& parent_support() {
    return *supports_[kParentFrameSink];
  }
  Surface* parent_surface() {
    return parent_support().GetLastCreatedSurfaceForTesting();
  }

  CompositorFrameSinkSupport& child_support1() {
    return *supports_[kChildFrameSink1];
  }
  Surface* child_surface1() {
    return child_support1().GetLastCreatedSurfaceForTesting();
  }

  CompositorFrameSinkSupport& child_support2() {
    return *supports_[kChildFrameSink2];
  }
  Surface* child_surface2() {
    return child_support2().GetLastCreatedSurfaceForTesting();
  }

  void CreateFrameSink(const FrameSinkId& frame_sink_id, bool is_root) {
    supports_[frame_sink_id] = std::make_unique<CompositorFrameSinkSupport>(
        &support_client_, &frame_sink_manager_, frame_sink_id, is_root,
        kNeedsSyncPoints);
  }

  void DestroyFrameSink(const FrameSinkId& frame_sink_id) {
    auto it = supports_.find(frame_sink_id);
    if (it == supports_.end())
      return;
    supports_.erase(it);
  }

  FrameSinkManagerImpl& frame_sink_manager() { return frame_sink_manager_; }

  // Returns all the references where |surface_id| is the parent.
  const base::flat_set<SurfaceId>& GetChildReferences(
      const SurfaceId& surface_id) {
    return frame_sink_manager()
        .surface_manager()
        ->GetSurfacesReferencedByParent(surface_id);
  }

  // Returns true if there is a temporary reference for |surface_id|.
  bool HasTemporaryReference(const SurfaceId& surface_id) {
    return frame_sink_manager().surface_manager()->HasTemporaryReference(
        surface_id);
  }

  Surface* GetLatestInFlightSurface(const SurfaceId& primary_surface_id,
                                    const SurfaceId& fallback_surface_id) {
    return frame_sink_manager().surface_manager()->GetLatestInFlightSurface(
        primary_surface_id, fallback_surface_id);
  }

  FakeExternalBeginFrameSource* begin_frame_source() {
    return begin_frame_source_.get();
  }

  base::TimeTicks Now() { return now_src_->NowTicks(); }

  FrameDeadline MakeDefaultDeadline() {
    return FrameDeadline(Now(), 4u, BeginFrameArgs::DefaultInterval(), false);
  }

  void SendNextBeginFrame() {
    // Creep the time forward so that any BeginFrameArgs is not equal to the
    // last one otherwise we violate the BeginFrameSource contract.
    now_src_->Advance(BeginFrameArgs::DefaultInterval());
    BeginFrameArgs args = begin_frame_source_->CreateBeginFrameArgs(
        BEGINFRAME_FROM_HERE, now_src_.get());
    begin_frame_source_->TestOnBeginFrame(args);
  }

  void SendLateBeginFrame() {
    // Creep the time forward so that any BeginFrameArgs is not equal to the
    // last one otherwise we violate the BeginFrameSource contract.
    now_src_->Advance(4u * BeginFrameArgs::DefaultInterval());
    BeginFrameArgs args = begin_frame_source_->CreateBeginFrameArgs(
        BEGINFRAME_FROM_HERE, now_src_.get());
    begin_frame_source_->TestOnBeginFrame(args);
  }

  void SetFrameSinkHierarchy(const FrameSinkId& parent_frame_sink_id,
                             const FrameSinkId& child_frame_sink_id) {
    frame_sink_manager_client_.SetFrameSinkHierarchy(parent_frame_sink_id,
                                                     child_frame_sink_id);
  }

  void DisableAssignTemporaryReferences() {
    frame_sink_manager_client_.DisableAssignTemporaryReferences();
  }

  FakeSurfaceObserver& surface_observer() { return surface_observer_; }

  // testing::Test:
  void SetUp() override {
    testing::Test::SetUp();

    begin_frame_source_ =
        std::make_unique<FakeExternalBeginFrameSource>(0.f, false);
    begin_frame_source_->SetClient(&begin_frame_source_client_);
    now_src_ = std::make_unique<base::SimpleTestTickClock>();
    frame_sink_manager_.surface_manager()->SetTickClockForTesting(
        now_src_.get());
    frame_sink_manager_.surface_manager()->AddObserver(&surface_observer_);
    frame_sink_manager_.SetLocalClient(&frame_sink_manager_client_);
    supports_[kDisplayFrameSink] = std::make_unique<CompositorFrameSinkSupport>(
        &support_client_, &frame_sink_manager_, kDisplayFrameSink, kIsRoot,
        kNeedsSyncPoints);

    supports_[kParentFrameSink] = std::make_unique<CompositorFrameSinkSupport>(
        &support_client_, &frame_sink_manager_, kParentFrameSink, kIsChildRoot,
        kNeedsSyncPoints);

    supports_[kChildFrameSink1] = std::make_unique<CompositorFrameSinkSupport>(
        &support_client_, &frame_sink_manager_, kChildFrameSink1, kIsChildRoot,
        kNeedsSyncPoints);

    supports_[kChildFrameSink2] = std::make_unique<CompositorFrameSinkSupport>(
        &support_client_, &frame_sink_manager_, kChildFrameSink2, kIsChildRoot,
        kNeedsSyncPoints);

    // Normally, the BeginFrameSource would be registered by the Display. We
    // register it here so that BeginFrames are received by the display support,
    // for use in the PassesOnBeginFrameAcks test. Other supports do not receive
    // BeginFrames, since the frame sink hierarchy is not set up in this test.
    frame_sink_manager_.RegisterBeginFrameSource(begin_frame_source_.get(),
                                                 kDisplayFrameSink);
  }

  void TearDown() override {
    frame_sink_manager_.surface_manager()->RemoveObserver(&surface_observer_);
    frame_sink_manager_.UnregisterBeginFrameSource(begin_frame_source_.get());

    frame_sink_manager_client_.Reset();

    begin_frame_source_->SetClient(nullptr);
    begin_frame_source_.reset();

    supports_.clear();

    surface_observer_.Reset();
  }

  bool IsMarkedForDestruction(const SurfaceId& surface_id) {
    return frame_sink_manager_.surface_manager()->IsMarkedForDestruction(
        surface_id);
  }

  Surface* GetSurfaceForId(const SurfaceId& surface_id) {
    return frame_sink_manager_.surface_manager()->GetSurfaceForId(surface_id);
  }

 protected:
  testing::NiceMock<MockCompositorFrameSinkClient> support_client_;

 private:
  std::unique_ptr<base::SimpleTestTickClock> now_src_;
  FrameSinkManagerImpl frame_sink_manager_;
  TestFrameSinkManagerClient frame_sink_manager_client_;
  FakeSurfaceObserver surface_observer_;
  FakeExternalBeginFrameSourceClient begin_frame_source_client_;
  std::unique_ptr<FakeExternalBeginFrameSource> begin_frame_source_;
  std::unordered_map<FrameSinkId,
                     std::unique_ptr<CompositorFrameSinkSupport>,
                     FrameSinkIdHash>
      supports_;

  DISALLOW_COPY_AND_ASSIGN(SurfaceSynchronizationTest);
};

// The display root surface should have a surface reference from the top-level
// root added/removed when a CompositorFrame is submitted with a new
// SurfaceId.
TEST_F(SurfaceSynchronizationTest, RootSurfaceReceivesReferences) {
  const SurfaceId display_id_first = MakeSurfaceId(kDisplayFrameSink, 1);
  const SurfaceId display_id_second = MakeSurfaceId(kDisplayFrameSink, 2);

  // Submit a CompositorFrame for the first display root surface.
  display_support().SubmitCompositorFrame(display_id_first.local_surface_id(),
                                          MakeDefaultCompositorFrame());

  // A surface reference from the top-level root is added and there shouldn't be
  // a temporary reference.
  EXPECT_FALSE(HasTemporaryReference(display_id_first));
  EXPECT_THAT(GetChildReferences(
                  frame_sink_manager().surface_manager()->GetRootSurfaceId()),
              UnorderedElementsAre(display_id_first));

  // Submit a CompositorFrame for the second display root surface.
  display_support().SubmitCompositorFrame(display_id_second.local_surface_id(),
                                          MakeDefaultCompositorFrame());

  // A surface reference from the top-level root to |display_id_second| should
  // be added and the reference to |display_root_first| removed.
  EXPECT_FALSE(HasTemporaryReference(display_id_second));
  EXPECT_THAT(GetChildReferences(
                  frame_sink_manager().surface_manager()->GetRootSurfaceId()),
              UnorderedElementsAre(display_id_second));

  frame_sink_manager().surface_manager()->GarbageCollectSurfaces();

  // Surface |display_id_first| is unreachable and should get deleted.
  EXPECT_EQ(nullptr, GetSurfaceForId(display_id_first));
}

// The parent Surface is blocked on |child_id1| and |child_id2|.
TEST_F(SurfaceSynchronizationTest, BlockedOnTwo) {
  const SurfaceId parent_id = MakeSurfaceId(kParentFrameSink, 1);
  const SurfaceId child_id1 = MakeSurfaceId(kChildFrameSink1, 1);
  const SurfaceId child_id2 = MakeSurfaceId(kChildFrameSink2, 1);

  parent_support().SubmitCompositorFrame(
      parent_id.local_surface_id(),
      MakeCompositorFrame({child_id1, child_id2}, empty_surface_ids(),
                          std::vector<TransferableResource>()));

  // parent_support is blocked on |child_id1| and |child_id2|.
  EXPECT_TRUE(parent_surface()->has_deadline());
  EXPECT_FALSE(parent_surface()->HasActiveFrame());
  EXPECT_TRUE(parent_surface()->HasPendingFrame());
  EXPECT_THAT(parent_surface()->activation_dependencies(),
              UnorderedElementsAre(child_id1, child_id2));

  // Submit a CompositorFrame without any dependencies to |child_id1|.
  // parent_support should now only be blocked on |child_id2|.
  child_support1().SubmitCompositorFrame(child_id1.local_surface_id(),
                                         MakeDefaultCompositorFrame());

  EXPECT_TRUE(parent_surface()->has_deadline());
  EXPECT_FALSE(parent_surface()->HasActiveFrame());
  EXPECT_TRUE(parent_surface()->HasPendingFrame());
  EXPECT_THAT(parent_surface()->activation_dependencies(),
              UnorderedElementsAre(child_id2));

  // Submit a CompositorFrame without any dependencies to |child_id2|.
  // parent_support should be activated.
  child_support2().SubmitCompositorFrame(child_id2.local_surface_id(),
                                         MakeDefaultCompositorFrame());

  EXPECT_FALSE(child_surface2()->has_deadline());
  EXPECT_TRUE(parent_surface()->HasActiveFrame());
  EXPECT_FALSE(parent_surface()->HasPendingFrame());
  EXPECT_THAT(parent_surface()->activation_dependencies(), IsEmpty());
}

// The parent Surface is blocked on |child_id2| which is blocked on
// |child_id3|.
TEST_F(SurfaceSynchronizationTest, BlockedChain) {
  const SurfaceId parent_id = MakeSurfaceId(kParentFrameSink, 1);
  const SurfaceId child_id1 = MakeSurfaceId(kChildFrameSink1, 1);
  const SurfaceId child_id2 = MakeSurfaceId(kChildFrameSink2, 1);

  parent_support().SubmitCompositorFrame(
      parent_id.local_surface_id(),
      MakeCompositorFrame({child_id1}, empty_surface_ids(),
                          std::vector<TransferableResource>()));

  // parent_support is blocked on |child_id1|.
  EXPECT_TRUE(parent_surface()->has_deadline());
  EXPECT_FALSE(parent_surface()->HasActiveFrame());
  EXPECT_TRUE(parent_surface()->HasPendingFrame());
  EXPECT_THAT(parent_surface()->activation_dependencies(),
              UnorderedElementsAre(child_id1));
  // The parent should not report damage until it activates.
  EXPECT_FALSE(surface_observer().IsSurfaceDamaged(parent_id));

  child_support1().SubmitCompositorFrame(
      child_id1.local_surface_id(),
      MakeCompositorFrame({child_id2}, empty_surface_ids(),
                          std::vector<TransferableResource>()));

  // child_support1 should now be blocked on |child_id2|.
  EXPECT_TRUE(child_surface1()->has_deadline());
  EXPECT_FALSE(child_surface1()->HasActiveFrame());
  EXPECT_TRUE(child_surface1()->HasPendingFrame());
  EXPECT_THAT(child_surface1()->activation_dependencies(),
              UnorderedElementsAre(child_id2));
  // The parent and child should not report damage until they activate.
  EXPECT_FALSE(surface_observer().IsSurfaceDamaged(parent_id));
  EXPECT_FALSE(surface_observer().IsSurfaceDamaged(child_id1));

  // The parent should still be blocked on |child_id1| because it's pending.
  EXPECT_THAT(parent_surface()->activation_dependencies(),
              UnorderedElementsAre(child_id1));

  // Submit a CompositorFrame without any dependencies to |child_id2|.
  // parent_support should be activated.
  child_support2().SubmitCompositorFrame(
      child_id2.local_surface_id(),
      MakeCompositorFrame(empty_surface_ids(), empty_surface_ids(),
                          std::vector<TransferableResource>()));

  EXPECT_FALSE(child_surface2()->has_deadline());

  // child_surface1 should now be active.
  EXPECT_TRUE(child_surface1()->HasActiveFrame());
  EXPECT_FALSE(child_surface1()->HasPendingFrame());
  EXPECT_THAT(child_surface1()->activation_dependencies(), IsEmpty());

  // parent_surface should now be active.
  EXPECT_TRUE(parent_surface()->HasActiveFrame());
  EXPECT_FALSE(parent_surface()->HasPendingFrame());
  EXPECT_THAT(parent_surface()->activation_dependencies(), IsEmpty());

  // All three surfaces |parent_id|, |child_id1|, and |child_id2| should
  // now report damage. This would trigger a new display frame.
  EXPECT_TRUE(surface_observer().IsSurfaceDamaged(parent_id));
  EXPECT_TRUE(surface_observer().IsSurfaceDamaged(child_id1));
  EXPECT_TRUE(surface_observer().IsSurfaceDamaged(child_id2));
}

// parent_surface and child_surface1 are blocked on |child_id2|.
TEST_F(SurfaceSynchronizationTest, TwoBlockedOnOne) {
  const SurfaceId parent_id = MakeSurfaceId(kParentFrameSink, 1);
  const SurfaceId child_id1 = MakeSurfaceId(kChildFrameSink1, 1);
  const SurfaceId child_id2 = MakeSurfaceId(kChildFrameSink2, 1);

  parent_support().SubmitCompositorFrame(
      parent_id.local_surface_id(),
      MakeCompositorFrame({child_id2}, empty_surface_ids(),
                          std::vector<TransferableResource>()));

  // parent_support is blocked on |child_id2|.
  EXPECT_TRUE(parent_surface()->has_deadline());
  EXPECT_FALSE(parent_surface()->HasActiveFrame());
  EXPECT_TRUE(parent_surface()->HasPendingFrame());
  EXPECT_THAT(parent_surface()->activation_dependencies(),
              UnorderedElementsAre(child_id2));

  // child_support1 should now be blocked on |child_id2|.
  child_support1().SubmitCompositorFrame(
      child_id1.local_surface_id(),
      MakeCompositorFrame({child_id2}, empty_surface_ids(),
                          std::vector<TransferableResource>()));

  EXPECT_TRUE(child_surface1()->has_deadline());
  EXPECT_FALSE(child_surface1()->HasActiveFrame());
  EXPECT_TRUE(child_surface1()->HasPendingFrame());
  EXPECT_THAT(child_surface1()->activation_dependencies(),
              UnorderedElementsAre(child_id2));

  // The parent should still be blocked on |child_id2|.
  EXPECT_THAT(parent_surface()->activation_dependencies(),
              UnorderedElementsAre(child_id2));

  // Submit a CompositorFrame without any dependencies to |child_id2|.
  // parent_support should be activated.
  child_support2().SubmitCompositorFrame(child_id2.local_surface_id(),
                                         MakeDefaultCompositorFrame());

  EXPECT_FALSE(child_surface2()->has_deadline());

  // child_surface1 should now be active.
  EXPECT_TRUE(child_surface1()->HasActiveFrame());
  EXPECT_FALSE(child_surface1()->HasPendingFrame());
  EXPECT_THAT(child_surface1()->activation_dependencies(), IsEmpty());

  // parent_surface should now be active.
  EXPECT_TRUE(parent_surface()->HasActiveFrame());
  EXPECT_FALSE(parent_surface()->HasPendingFrame());
  EXPECT_THAT(parent_surface()->activation_dependencies(), IsEmpty());
}

// parent_surface is blocked on |child_id1|, and child_surface2 is blocked on
// |child_id2| until the deadline hits.
TEST_F(SurfaceSynchronizationTest, DeadlineHits) {
  const SurfaceId parent_id = MakeSurfaceId(kParentFrameSink, 1);
  const SurfaceId child_id1 = MakeSurfaceId(kChildFrameSink1, 1);
  const SurfaceId child_id2 = MakeSurfaceId(kChildFrameSink2, 1);

  parent_support().SubmitCompositorFrame(
      parent_id.local_surface_id(),
      MakeCompositorFrame({child_id1}, empty_surface_ids(),
                          std::vector<TransferableResource>(),
                          MakeDefaultDeadline()));

  // parent_support is blocked on |child_id1|.
  EXPECT_TRUE(parent_surface()->has_deadline());
  EXPECT_FALSE(parent_surface()->HasActiveFrame());
  EXPECT_TRUE(parent_surface()->HasPendingFrame());
  EXPECT_THAT(parent_surface()->activation_dependencies(),
              UnorderedElementsAre(child_id1));

  child_support1().SubmitCompositorFrame(
      child_id1.local_surface_id(),
      MakeCompositorFrame({child_id2}, empty_surface_ids(),
                          std::vector<TransferableResource>(),
                          MakeDefaultDeadline()));

  // child_support1 should now be blocked on |child_id2|.
  EXPECT_TRUE(child_surface1()->has_deadline());
  EXPECT_FALSE(child_surface1()->HasActiveFrame());
  EXPECT_TRUE(child_surface1()->HasPendingFrame());
  EXPECT_THAT(child_surface1()->activation_dependencies(),
              UnorderedElementsAre(child_id2));

  // The parent should still be blocked on |child_id1| because it's pending.
  EXPECT_THAT(parent_surface()->activation_dependencies(),
              UnorderedElementsAre(child_id1));

  for (int i = 0; i < 3; ++i) {
    SendNextBeginFrame();
    // There is still a looming deadline! Eeek!
    EXPECT_TRUE(parent_surface()->has_deadline());

    // parent_support is still blocked on |child_id1|.
    EXPECT_FALSE(parent_surface()->HasActiveFrame());
    EXPECT_TRUE(parent_surface()->HasPendingFrame());
    EXPECT_THAT(parent_surface()->activation_dependencies(),
                UnorderedElementsAre(child_id1));

    // child_support1 is still blocked on |child_id2|.
    EXPECT_FALSE(child_surface1()->HasActiveFrame());
    EXPECT_TRUE(child_surface1()->HasPendingFrame());
    EXPECT_THAT(child_surface1()->activation_dependencies(),
                UnorderedElementsAre(child_id2));
  }

  SendNextBeginFrame();

  // The deadline has passed.
  EXPECT_FALSE(parent_surface()->has_deadline());

  // parent_surface has been activated.
  EXPECT_TRUE(parent_surface()->HasActiveFrame());
  EXPECT_FALSE(parent_surface()->HasPendingFrame());
  EXPECT_THAT(parent_surface()->activation_dependencies(), IsEmpty());

  // child_surface1 has been activated.
  EXPECT_TRUE(child_surface1()->HasActiveFrame());
  EXPECT_FALSE(child_surface1()->HasPendingFrame());
  EXPECT_THAT(child_surface1()->activation_dependencies(), IsEmpty());
}

// This test verifies that unlimited deadline mode works and that surfaces will
// not activate until dependencies are resolved.
TEST_F(SurfaceSynchronizationTest, UnlimitedDeadline) {
  // Turn on unlimited deadline mode.
  frame_sink_manager()
      .surface_manager()
      ->SetActivationDeadlineInFramesForTesting(base::nullopt);

  const SurfaceId parent_id = MakeSurfaceId(kParentFrameSink, 1);
  const SurfaceId child_id1 = MakeSurfaceId(kChildFrameSink1, 1);

  // The deadline specified by the parent is ignored in unlimited deadline
  // mode.
  parent_support().SubmitCompositorFrame(
      parent_id.local_surface_id(),
      MakeCompositorFrame({child_id1}, empty_surface_ids(),
                          std::vector<TransferableResource>(),
                          MakeDefaultDeadline()));

  // parent_support is blocked on |child_id1|.
  EXPECT_FALSE(parent_surface()->HasActiveFrame());
  EXPECT_TRUE(parent_surface()->HasPendingFrame());
  EXPECT_THAT(parent_surface()->activation_dependencies(),
              UnorderedElementsAre(child_id1));

  for (int i = 0; i < 4; ++i) {
    SendNextBeginFrame();
    // parent_support is still blocked on |child_id1|.
    EXPECT_FALSE(parent_surface()->HasActiveFrame());
    EXPECT_TRUE(parent_surface()->HasPendingFrame());
    EXPECT_THAT(parent_surface()->activation_dependencies(),
                UnorderedElementsAre(child_id1));
  }

  // parent_support is STILL blocked on |child_id1|.
  EXPECT_FALSE(parent_surface()->HasActiveFrame());
  EXPECT_TRUE(parent_surface()->HasPendingFrame());
  EXPECT_THAT(parent_surface()->activation_dependencies(),
              UnorderedElementsAre(child_id1));

  child_support1().SubmitCompositorFrame(child_id1.local_surface_id(),
                                         MakeDefaultCompositorFrame());

  // parent_surface has been activated.
  EXPECT_TRUE(parent_surface()->HasActiveFrame());
  EXPECT_FALSE(parent_surface()->HasPendingFrame());
  EXPECT_THAT(parent_surface()->activation_dependencies(), IsEmpty());
}

// parent_surface is blocked on |child_id1| until a late BeginFrame arrives and
// triggers a deadline.
TEST_F(SurfaceSynchronizationTest, LateBeginFrameTriggersDeadline) {
  const SurfaceId parent_id = MakeSurfaceId(kParentFrameSink, 1);
  const SurfaceId child_id1 = MakeSurfaceId(kChildFrameSink1, 1);

  parent_support().SubmitCompositorFrame(
      parent_id.local_surface_id(),
      MakeCompositorFrame({child_id1}, empty_surface_ids(),
                          std::vector<TransferableResource>()));

  // parent_support is blocked on |child_id1|.
  EXPECT_TRUE(parent_surface()->has_deadline());
  EXPECT_FALSE(parent_surface()->HasActiveFrame());
  EXPECT_TRUE(parent_surface()->HasPendingFrame());
  EXPECT_THAT(parent_surface()->activation_dependencies(),
              UnorderedElementsAre(child_id1));

  SendLateBeginFrame();

  // The deadline has passed.
  EXPECT_FALSE(parent_surface()->has_deadline());

  // parent_surface has been activated.
  EXPECT_TRUE(parent_surface()->HasActiveFrame());
  EXPECT_FALSE(parent_surface()->HasPendingFrame());
  EXPECT_THAT(parent_surface()->activation_dependencies(), IsEmpty());
}

// This test verifies at the Surface activates once a CompositorFrame is
// submitted that has no unresolved dependencies.
TEST_F(SurfaceSynchronizationTest, NewFrameOverridesOldDependencies) {
  const SurfaceId parent_id = MakeSurfaceId(kParentFrameSink, 1);
  const SurfaceId arbitrary_id = MakeSurfaceId(kArbitraryFrameSink, 1);

  // Submit a CompositorFrame that depends on |arbitrary_id|.
  parent_support().SubmitCompositorFrame(
      parent_id.local_surface_id(),
      MakeCompositorFrame({arbitrary_id}, empty_surface_ids(),
                          std::vector<TransferableResource>()));

  // Verify that the CompositorFrame is blocked on |arbitrary_id|.
  EXPECT_FALSE(parent_surface()->HasActiveFrame());
  EXPECT_TRUE(parent_surface()->HasPendingFrame());
  EXPECT_THAT(parent_surface()->activation_dependencies(),
              UnorderedElementsAre(arbitrary_id));

  // Submit a CompositorFrame that has no dependencies.
  parent_support().SubmitCompositorFrame(parent_id.local_surface_id(),
                                         MakeDefaultCompositorFrame());

  // Verify that the CompositorFrame has been activated.
  EXPECT_TRUE(parent_surface()->HasActiveFrame());
  EXPECT_FALSE(parent_surface()->HasPendingFrame());
  EXPECT_THAT(parent_surface()->activation_dependencies(), IsEmpty());
}

// This test verifies that a pending CompositorFrame does not affect surface
// references. A new surface from a child will continue to exist as a temporary
// reference until the parent's frame activates.
TEST_F(SurfaceSynchronizationTest, OnlyActiveFramesAffectSurfaceReferences) {
  SetFrameSinkHierarchy(kParentFrameSink, kChildFrameSink1);
  SetFrameSinkHierarchy(kParentFrameSink, kChildFrameSink2);

  const SurfaceId parent_id = MakeSurfaceId(kParentFrameSink, 1);
  const SurfaceId child_id1 = MakeSurfaceId(kChildFrameSink1, 1);
  const SurfaceId child_id2 = MakeSurfaceId(kChildFrameSink2, 1);

  // child_support1 submits a CompositorFrame without any dependencies.
  // DidReceiveCompositorFrameAck should call on immediate activation.
  EXPECT_CALL(support_client_, DidReceiveCompositorFrameAck(_)).Times(1);
  child_support1().SubmitCompositorFrame(child_id1.local_surface_id(),
                                         MakeDefaultCompositorFrame());
  testing::Mock::VerifyAndClearExpectations(&support_client_);

  // Verify that the child surface is not blocked.
  EXPECT_TRUE(child_surface1()->HasActiveFrame());
  EXPECT_FALSE(child_surface1()->HasPendingFrame());
  EXPECT_THAT(child_surface1()->activation_dependencies(), IsEmpty());

  // Verify that there's a temporary reference for |child_id1|.
  EXPECT_TRUE(HasTemporaryReference(child_id1));

  // parent_support submits a CompositorFrame that depends on |child_id1|
  // (which is already active) and |child_id2|. Thus, the parent should not
  // activate immediately. DidReceiveCompositorFrameAck should not be called
  // immediately because the parent CompositorFrame is also blocked on
  // |child_id2|.
  EXPECT_CALL(support_client_, DidReceiveCompositorFrameAck(_)).Times(0);
  parent_support().SubmitCompositorFrame(
      parent_id.local_surface_id(),
      MakeCompositorFrame({child_id2}, {child_id1},
                          std::vector<TransferableResource>()));
  EXPECT_FALSE(parent_surface()->HasActiveFrame());
  EXPECT_TRUE(parent_surface()->HasPendingFrame());
  EXPECT_THAT(parent_surface()->activation_dependencies(),
              UnorderedElementsAre(child_id2));
  EXPECT_THAT(GetChildReferences(parent_id), IsEmpty());
  testing::Mock::VerifyAndClearExpectations(&support_client_);

  // Verify that there's a temporary reference for |child_id1| that still
  // exists.
  EXPECT_TRUE(HasTemporaryReference(child_id1));

  // child_support2 submits a CompositorFrame without any dependencies.
  // Both the child and the parent should immediately ACK CompositorFrames
  // on activation.
  EXPECT_CALL(support_client_, DidReceiveCompositorFrameAck(_)).Times(2);
  child_support2().SubmitCompositorFrame(child_id2.local_surface_id(),
                                         MakeDefaultCompositorFrame());
  testing::Mock::VerifyAndClearExpectations(&support_client_);

  // Verify that the child surface is not blocked.
  EXPECT_TRUE(child_surface1()->HasActiveFrame());
  EXPECT_FALSE(child_surface1()->HasPendingFrame());
  EXPECT_THAT(child_surface1()->activation_dependencies(), IsEmpty());

  // Verify that the parent surface's CompositorFrame has activated and that
  // the temporary reference has been replaced by a permanent one.
  EXPECT_TRUE(parent_surface()->HasActiveFrame());
  EXPECT_FALSE(parent_surface()->HasPendingFrame());
  EXPECT_THAT(parent_surface()->activation_dependencies(), IsEmpty());
  EXPECT_FALSE(HasTemporaryReference(child_id1));
  EXPECT_THAT(GetChildReferences(parent_id), UnorderedElementsAre(child_id1));
}

// This test verifies that we do not double count returned resources when a
// CompositorFrame starts out as pending, then becomes active, and then is
// replaced with another active CompositorFrame.
TEST_F(SurfaceSynchronizationTest, ResourcesOnlyReturnedOnce) {
  const SurfaceId parent_id = MakeSurfaceId(kParentFrameSink, 1);
  const SurfaceId child_id = MakeSurfaceId(kChildFrameSink1, 1);

  // The parent submits a CompositorFrame that depends on |child_id| before
  // the child submits a CompositorFrame. The CompositorFrame also has
  // resources in its resource list.
  TransferableResource resource;
  resource.id = 1337;
  resource.format = ALPHA_8;
  resource.filter = 1234;
  resource.size = gfx::Size(1234, 5678);
  std::vector<TransferableResource> resource_list = {resource};
  parent_support().SubmitCompositorFrame(
      parent_id.local_surface_id(),
      MakeCompositorFrame({child_id}, empty_surface_ids(), resource_list));

  // Verify that the CompositorFrame is blocked on |child_id|.
  EXPECT_FALSE(parent_surface()->HasActiveFrame());
  EXPECT_TRUE(parent_surface()->HasPendingFrame());
  EXPECT_THAT(parent_surface()->activation_dependencies(),
              UnorderedElementsAre(child_id));

  child_support1().SubmitCompositorFrame(
      child_id.local_surface_id(),
      MakeCompositorFrame(empty_surface_ids(), empty_surface_ids(),
                          std::vector<TransferableResource>()));

  // Verify that the child CompositorFrame activates immediately.
  EXPECT_TRUE(child_surface1()->HasActiveFrame());
  EXPECT_FALSE(child_surface1()->HasPendingFrame());
  EXPECT_THAT(child_surface1()->activation_dependencies(), IsEmpty());

  // Verify that the parent has activated.
  EXPECT_TRUE(parent_surface()->HasActiveFrame());
  EXPECT_FALSE(parent_surface()->HasPendingFrame());
  EXPECT_THAT(parent_surface()->activation_dependencies(), IsEmpty());

  std::vector<ReturnedResource> returned_resources = {
      resource.ToReturnedResource()};
  EXPECT_CALL(support_client_,
              DidReceiveCompositorFrameAck(returned_resources));

  // The parent submits a CompositorFrame without any dependencies. That
  // frame should activate immediately, replacing the earlier frame. The
  // resource from the earlier frame should be returned to the client.
  parent_support().SubmitCompositorFrame(
      parent_id.local_surface_id(),
      MakeCompositorFrame({empty_surface_ids()}, {empty_surface_ids()},
                          std::vector<TransferableResource>()));
  EXPECT_TRUE(parent_surface()->HasActiveFrame());
  EXPECT_FALSE(parent_surface()->HasPendingFrame());
  EXPECT_THAT(parent_surface()->activation_dependencies(), IsEmpty());
}

// This test verifies that if a surface has both a pending and active
// CompositorFrame and the pending CompositorFrame activates, replacing
// the existing active CompositorFrame, then the surface reference hierarchy
// will be updated allowing garbage collection of surfaces that are no longer
// referenced.
TEST_F(SurfaceSynchronizationTest, DropStaleReferencesAfterActivation) {
  const SurfaceId parent_id = MakeSurfaceId(kParentFrameSink, 1);
  const SurfaceId child_id1 = MakeSurfaceId(kChildFrameSink1, 1);
  const SurfaceId child_id2 = MakeSurfaceId(kChildFrameSink2, 1);

  // The parent submits a CompositorFrame that depends on |child_id1| before
  // the child submits a CompositorFrame.
  EXPECT_CALL(support_client_, DidReceiveCompositorFrameAck(_)).Times(0);
  parent_support().SubmitCompositorFrame(
      parent_id.local_surface_id(),
      MakeCompositorFrame({child_id1}, empty_surface_ids(),
                          std::vector<TransferableResource>()));

  // Verify that the CompositorFrame is blocked on |child_id|.
  EXPECT_FALSE(parent_surface()->HasActiveFrame());
  EXPECT_TRUE(parent_surface()->HasPendingFrame());
  EXPECT_THAT(parent_surface()->activation_dependencies(),
              UnorderedElementsAre(child_id1));
  testing::Mock::VerifyAndClearExpectations(&support_client_);

  // Verify that no references are added while the CompositorFrame is
  // pending.
  EXPECT_THAT(GetChildReferences(parent_id), IsEmpty());

  // DidReceiveCompositorFrameAck should get called twice: once for the child
  // and once for the now active parent CompositorFrame.
  EXPECT_CALL(support_client_, DidReceiveCompositorFrameAck(_)).Times(2);
  child_support1().SubmitCompositorFrame(child_id1.local_surface_id(),
                                         MakeDefaultCompositorFrame());
  testing::Mock::VerifyAndClearExpectations(&support_client_);

  // Verify that the child CompositorFrame activates immediately.
  EXPECT_TRUE(child_surface1()->HasActiveFrame());
  EXPECT_FALSE(child_surface1()->HasPendingFrame());
  EXPECT_THAT(child_surface1()->activation_dependencies(), IsEmpty());

  // Verify that the parent Surface has activated.
  EXPECT_TRUE(parent_surface()->HasActiveFrame());
  EXPECT_FALSE(parent_surface()->HasPendingFrame());
  EXPECT_THAT(parent_surface()->activation_dependencies(), IsEmpty());

  // Submit a new parent CompositorFrame to add a reference.
  parent_support().SubmitCompositorFrame(
      parent_id.local_surface_id(),
      MakeCompositorFrame(empty_surface_ids(), {child_id1},
                          std::vector<TransferableResource>()));

  // Verify that the parent Surface has activated.
  EXPECT_TRUE(parent_surface()->HasActiveFrame());
  EXPECT_FALSE(parent_surface()->HasPendingFrame());
  EXPECT_THAT(parent_surface()->activation_dependencies(), IsEmpty());

  // Verify that there is no temporary reference for the child and that
  // the reference from the parent to the child still exists.
  EXPECT_FALSE(HasTemporaryReference(child_id1));
  EXPECT_THAT(GetChildReferences(parent_id), UnorderedElementsAre(child_id1));

  // The parent submits another CompositorFrame that depends on |child_id2|.
  // Submitting a pending CompositorFrame will not trigger a
  // CompositorFrameAck.
  EXPECT_CALL(support_client_, DidReceiveCompositorFrameAck(_)).Times(0);
  parent_support().SubmitCompositorFrame(
      parent_id.local_surface_id(),
      MakeCompositorFrame({child_id2}, empty_surface_ids(),
                          std::vector<TransferableResource>()));
  testing::Mock::VerifyAndClearExpectations(&support_client_);

  // The parent surface should now have both a pending and activate
  // CompositorFrame. Verify that the set of child references from
  // |parent_id| are only from the active CompositorFrame.
  EXPECT_TRUE(parent_surface()->HasActiveFrame());
  EXPECT_TRUE(parent_surface()->HasPendingFrame());
  EXPECT_THAT(parent_surface()->activation_dependencies(),
              UnorderedElementsAre(child_id2));
  EXPECT_THAT(GetChildReferences(parent_id), UnorderedElementsAre(child_id1));

  child_support2().SubmitCompositorFrame(child_id2.local_surface_id(),
                                         MakeDefaultCompositorFrame());

  // Verify that the parent Surface has activated and no longer has a
  // pending CompositorFrame. Also verify that |child_id1| is no longer a
  // child reference of |parent_id|.
  EXPECT_TRUE(parent_surface()->HasActiveFrame());
  EXPECT_FALSE(parent_surface()->HasPendingFrame());
  EXPECT_THAT(parent_surface()->activation_dependencies(), IsEmpty());
  // The parent will not immediately refer to the child until it submits a new
  // CompositorFrame with the reference.
  EXPECT_THAT(GetChildReferences(parent_id), IsEmpty());
}

// Verifies that LatencyInfo does not get too large after multiple resizes.
TEST_F(SurfaceSynchronizationTest, LimitLatencyInfo) {
  const SurfaceId parent_id1 = MakeSurfaceId(kParentFrameSink, 1);
  const SurfaceId parent_id2 = MakeSurfaceId(kParentFrameSink, 2);
  const ui::LatencyComponentType latency_type1 =
      ui::DISPLAY_COMPOSITOR_RECEIVED_FRAME_COMPONENT;
  const int64_t latency_id1 = 234;
  const ui::LatencyComponentType latency_type2 = ui::TAB_SHOW_COMPONENT;
  const int64_t latency_id2 = 31434351;

  // Submit a frame with latency info
  ui::LatencyInfo info;
  info.AddLatencyNumber(latency_type1, latency_id1);

  CompositorFrameBuilder builder;
  builder.AddDefaultRenderPass();
  for (int i = 0; i < 60; ++i)
    builder.AddLatencyInfo(info);
  CompositorFrame frame = builder.Build();

  parent_support().SubmitCompositorFrame(parent_id1.local_surface_id(),
                                         std::move(frame));

  // Verify that the old surface has an active frame and no pending frame.
  Surface* old_surface = GetSurfaceForId(parent_id1);
  ASSERT_NE(nullptr, old_surface);
  EXPECT_TRUE(old_surface->HasActiveFrame());
  EXPECT_FALSE(old_surface->HasPendingFrame());

  // Submit another frame with some other latency info and a different
  // LocalSurfaceId.
  ui::LatencyInfo info2;
  info2.AddLatencyNumber(latency_type2, latency_id2);

  builder.AddDefaultRenderPass();
  for (int i = 0; i < 60; ++i)
    builder.AddLatencyInfo(info);
  CompositorFrame frame2 = builder.Build();

  parent_support().SubmitCompositorFrame(parent_id2.local_surface_id(),
                                         std::move(frame2));

  // Verify that the new surface has an active frame and no pending frames.
  Surface* surface = GetSurfaceForId(parent_id2);
  ASSERT_NE(nullptr, surface);
  EXPECT_TRUE(surface->HasActiveFrame());
  EXPECT_FALSE(surface->HasPendingFrame());

  // Verify that the new surface has no latency info objects because it grew
  // too large.
  std::vector<ui::LatencyInfo> info_list;
  surface->TakeLatencyInfo(&info_list);
  EXPECT_EQ(0u, info_list.size());
}

// Checks whether the latency info are moved to the new surface from the old
// one when LocalSurfaceId changes. No frame has unresolved dependencies.
TEST_F(SurfaceSynchronizationTest,
       LatencyInfoCarriedOverOnResize_NoUnresolvedDependencies) {
  const SurfaceId parent_id1 = MakeSurfaceId(kParentFrameSink, 1);
  const SurfaceId parent_id2 = MakeSurfaceId(kParentFrameSink, 2);
  const ui::LatencyComponentType latency_type1 =
      ui::INPUT_EVENT_LATENCY_RENDERER_SWAP_COMPONENT;
  const int64_t latency_id1 = 234;
  const ui::LatencyComponentType latency_type2 = ui::TAB_SHOW_COMPONENT;
  const int64_t latency_id2 = 31434351;

  // Submit a frame with latency info
  ui::LatencyInfo info;
  info.AddLatencyNumber(latency_type1, latency_id1);

  CompositorFrame frame = CompositorFrameBuilder()
                              .AddDefaultRenderPass()
                              .AddLatencyInfo(info)
                              .Build();

  parent_support().SubmitCompositorFrame(parent_id1.local_surface_id(),
                                         std::move(frame));

  // Verify that the old surface has an active frame and no pending frame.
  Surface* old_surface = GetSurfaceForId(parent_id1);
  ASSERT_NE(nullptr, old_surface);
  EXPECT_TRUE(old_surface->HasActiveFrame());
  EXPECT_FALSE(old_surface->HasPendingFrame());

  // Submit another frame with some other latency info and a different
  // LocalSurfaceId.
  ui::LatencyInfo info2;
  info2.AddLatencyNumber(latency_type2, latency_id2);

  CompositorFrame frame2 = CompositorFrameBuilder()
                               .AddDefaultRenderPass()
                               .AddLatencyInfo(info2)
                               .Build();

  parent_support().SubmitCompositorFrame(parent_id2.local_surface_id(),
                                         std::move(frame2));

  // Verify that the new surface has an active frame and no pending frames.
  Surface* surface = GetSurfaceForId(parent_id2);
  ASSERT_NE(nullptr, surface);
  EXPECT_TRUE(surface->HasActiveFrame());
  EXPECT_FALSE(surface->HasPendingFrame());

  // Verify that the new surface has both latency info elements.
  std::vector<ui::LatencyInfo> info_list;
  surface->TakeLatencyInfo(&info_list);
  EXPECT_EQ(2u, info_list.size());

  ui::LatencyInfo aggregated_latency_info = info_list[0];
  aggregated_latency_info.AddNewLatencyFrom(info_list[1]);

  // Two components are the original ones, and the third one is
  // DISPLAY_COMPOSITOR_RECEIVED_FRAME_COMPONENT, logged on compositor frame
  // submit.
  EXPECT_EQ(3u, aggregated_latency_info.latency_components().size());

  ui::LatencyInfo::LatencyComponent comp1;
  EXPECT_TRUE(
      aggregated_latency_info.FindLatency(latency_type1, latency_id1, &comp1));
  EXPECT_TRUE(
      aggregated_latency_info.FindLatency(latency_type2, latency_id2, nullptr));
  EXPECT_TRUE(aggregated_latency_info.FindLatency(
      ui::DISPLAY_COMPOSITOR_RECEIVED_FRAME_COMPONENT, nullptr));
}

// Checks whether the latency info are moved to the new surface from the old
// one when LocalSurfaceId changes. Old surface has unresolved
// dependencies.
TEST_F(SurfaceSynchronizationTest,
       LatencyInfoCarriedOverOnResize_OldSurfaceHasPendingAndActiveFrame) {
  const SurfaceId parent_id1 = MakeSurfaceId(kParentFrameSink, 1);
  const SurfaceId parent_id2 = MakeSurfaceId(kParentFrameSink, 2);
  const SurfaceId child_id = MakeSurfaceId(kChildFrameSink1, 1);

  const ui::LatencyComponentType latency_type1 =
      ui::INPUT_EVENT_LATENCY_RENDERER_SWAP_COMPONENT;
  const int64_t latency_id1 = 234;
  const ui::LatencyComponentType latency_type2 = ui::TAB_SHOW_COMPONENT;
  const int64_t latency_id2 = 31434351;

  // Submit a frame with no unresolved dependecy.
  ui::LatencyInfo info;
  info.AddLatencyNumber(latency_type1, latency_id1);

  CompositorFrame frame = MakeDefaultCompositorFrame();
  frame.metadata.latency_info.push_back(info);

  parent_support().SubmitCompositorFrame(parent_id1.local_surface_id(),
                                         std::move(frame));

  // Submit a frame with unresolved dependencies.
  ui::LatencyInfo info2;
  info2.AddLatencyNumber(latency_type2, latency_id2);

  CompositorFrame frame2 = MakeCompositorFrame(
      {child_id}, empty_surface_ids(), std::vector<TransferableResource>());
  frame2.metadata.latency_info.push_back(info2);

  parent_support().SubmitCompositorFrame(parent_id1.local_surface_id(),
                                         std::move(frame2));

  // Verify that the old surface has both an active and a pending frame.
  Surface* old_surface = GetSurfaceForId(parent_id1);
  ASSERT_NE(nullptr, old_surface);
  EXPECT_TRUE(old_surface->HasActiveFrame());
  EXPECT_TRUE(old_surface->HasPendingFrame());

  // Submit a frame with a new local surface id.
  parent_support().SubmitCompositorFrame(parent_id2.local_surface_id(),
                                         MakeDefaultCompositorFrame());

  // Verify that the new surface has an active frame only.
  Surface* surface = GetSurfaceForId(parent_id2);
  ASSERT_NE(nullptr, surface);
  EXPECT_TRUE(surface->HasActiveFrame());
  EXPECT_FALSE(surface->HasPendingFrame());

  // Verify that the new surface has latency info from both active and pending
  // frame of the old surface.
  std::vector<ui::LatencyInfo> info_list;
  surface->TakeLatencyInfo(&info_list);
  EXPECT_EQ(2u, info_list.size());

  ui::LatencyInfo aggregated_latency_info = info_list[0];
  aggregated_latency_info.AddNewLatencyFrom(info_list[1]);

  // Two components are the original ones, and the third one is
  // DISPLAY_COMPOSITOR_RECEIVED_FRAME_COMPONENT, logged on compositor frame
  // submit.
  EXPECT_EQ(3u, aggregated_latency_info.latency_components().size());

  ui::LatencyInfo::LatencyComponent comp1;
  EXPECT_TRUE(
      aggregated_latency_info.FindLatency(latency_type1, latency_id1, &comp1));
  EXPECT_TRUE(
      aggregated_latency_info.FindLatency(latency_type2, latency_id2, nullptr));
  EXPECT_TRUE(aggregated_latency_info.FindLatency(
      ui::DISPLAY_COMPOSITOR_RECEIVED_FRAME_COMPONENT, nullptr));
}

// Checks whether the latency info are moved to the new surface from the old
// one when LocalSurfaceId changes. The new surface has unresolved
// dependencies.
TEST_F(SurfaceSynchronizationTest,
       LatencyInfoCarriedOverOnResize_NewSurfaceHasPendingFrame) {
  const SurfaceId parent_id1 = MakeSurfaceId(kParentFrameSink, 1);
  const SurfaceId parent_id2 = MakeSurfaceId(kParentFrameSink, 2);
  const SurfaceId child_id = MakeSurfaceId(kChildFrameSink1, 1);

  const ui::LatencyComponentType latency_type1 =
      ui::INPUT_EVENT_LATENCY_RENDERER_SWAP_COMPONENT;
  const int64_t latency_id1 = 234;
  const ui::LatencyComponentType latency_type2 = ui::TAB_SHOW_COMPONENT;
  const int64_t latency_id2 = 31434351;

  // Submit a frame with no unresolved dependencies.
  ui::LatencyInfo info;
  info.AddLatencyNumber(latency_type1, latency_id1);

  CompositorFrame frame = MakeDefaultCompositorFrame();
  frame.metadata.latency_info.push_back(info);

  parent_support().SubmitCompositorFrame(parent_id1.local_surface_id(),
                                         std::move(frame));

  // Verify that the old surface has an active frame only.
  Surface* old_surface = GetSurfaceForId(parent_id1);
  ASSERT_NE(nullptr, old_surface);
  EXPECT_TRUE(old_surface->HasActiveFrame());
  EXPECT_FALSE(old_surface->HasPendingFrame());

  // Submit a frame with a new local surface id and with unresolved
  // dependencies.
  ui::LatencyInfo info2;
  info2.AddLatencyNumber(latency_type2, latency_id2);

  CompositorFrame frame2 = MakeCompositorFrame(
      {child_id}, empty_surface_ids(), std::vector<TransferableResource>());
  frame2.metadata.latency_info.push_back(info2);

  parent_support().SubmitCompositorFrame(parent_id2.local_surface_id(),
                                         std::move(frame2));

  // Verify that the new surface has a pending frame and no active frame.
  Surface* surface = GetSurfaceForId(parent_id2);
  ASSERT_NE(nullptr, surface);
  EXPECT_TRUE(surface->HasPendingFrame());
  EXPECT_FALSE(surface->HasActiveFrame());

  // Resolve the dependencies. The frame in parent's surface must become active.
  child_support1().SubmitCompositorFrame(child_id.local_surface_id(),
                                         MakeDefaultCompositorFrame());
  EXPECT_FALSE(surface->HasPendingFrame());
  EXPECT_TRUE(surface->HasActiveFrame());

  // Both latency info elements must exist in the now-activated frame of the
  // new surface.
  std::vector<ui::LatencyInfo> info_list;
  surface->TakeLatencyInfo(&info_list);
  EXPECT_EQ(2u, info_list.size());

  ui::LatencyInfo aggregated_latency_info = info_list[0];
  aggregated_latency_info.AddNewLatencyFrom(info_list[1]);

  // Two components are the original ones, and the third one is
  // DISPLAY_COMPOSITOR_RECEIVED_FRAME_COMPONENT, logged on compositor frame
  // submit.
  EXPECT_EQ(3u, aggregated_latency_info.latency_components().size());

  ui::LatencyInfo::LatencyComponent comp1;
  EXPECT_TRUE(
      aggregated_latency_info.FindLatency(latency_type1, latency_id1, &comp1));
  EXPECT_TRUE(
      aggregated_latency_info.FindLatency(latency_type2, latency_id2, nullptr));
  EXPECT_TRUE(aggregated_latency_info.FindLatency(
      ui::DISPLAY_COMPOSITOR_RECEIVED_FRAME_COMPONENT, nullptr));
}

// Checks that resources and ack are sent together if possible.
TEST_F(SurfaceSynchronizationTest, ReturnResourcesWithAck) {
  const SurfaceId parent_id = MakeSurfaceId(kParentFrameSink, 1);
  TransferableResource resource;
  resource.id = 1234;
  parent_support().SubmitCompositorFrame(
      parent_id.local_surface_id(),
      MakeCompositorFrame(empty_surface_ids(), empty_surface_ids(),
                          {resource}));
  std::vector<ReturnedResource> returned_resources =
      TransferableResource::ReturnResources({resource});
  EXPECT_CALL(support_client_, ReclaimResources(_)).Times(0);
  EXPECT_CALL(support_client_,
              DidReceiveCompositorFrameAck(Eq(returned_resources)));
  parent_support().SubmitCompositorFrame(parent_id.local_surface_id(),
                                         MakeDefaultCompositorFrame());
}

// Verifies that if a surface is marked destroyed and a new frame arrives for
// it, it will be recovered.
TEST_F(SurfaceSynchronizationTest, SurfaceResurrection) {
  const SurfaceId parent_id = MakeSurfaceId(kParentFrameSink, 1);
  const SurfaceId child_id = MakeSurfaceId(kChildFrameSink1, 3);

  // Create the child surface by submitting a frame to it.
  EXPECT_EQ(nullptr, GetSurfaceForId(child_id));
  TransferableResource resource;
  resource.id = 1234;
  child_support1().SubmitCompositorFrame(
      child_id.local_surface_id(),
      MakeCompositorFrame(empty_surface_ids(), empty_surface_ids(),
                          {resource}));
  // Verify that the child surface is created.
  Surface* surface = GetSurfaceForId(child_id);
  EXPECT_NE(nullptr, surface);

  // Add a reference from the parent to the child.
  parent_support().SubmitCompositorFrame(
      parent_id.local_surface_id(),
      MakeCompositorFrame({child_id}, {child_id},
                          std::vector<TransferableResource>()));

  // Attempt to destroy the child surface. The surface must still exist since
  // the parent needs it but it will be marked as destroyed.
  child_support1().EvictLastActivatedSurface();
  surface = GetSurfaceForId(child_id);
  EXPECT_NE(nullptr, surface);
  EXPECT_TRUE(IsMarkedForDestruction(child_id));

  // Child submits another frame to the same local surface id that is marked
  // destroyed.
  {
    std::vector<ReturnedResource> returned_resources =
        TransferableResource::ReturnResources({resource});
    EXPECT_CALL(support_client_, ReclaimResources(_)).Times(0);
    EXPECT_CALL(support_client_,
                DidReceiveCompositorFrameAck(Eq(returned_resources)));
    surface_observer().Reset();
    child_support1().SubmitCompositorFrame(child_id.local_surface_id(),
                                           MakeDefaultCompositorFrame());
    testing::Mock::VerifyAndClearExpectations(&support_client_);
  }

  // Verify that the surface that was marked destroyed is recovered and is being
  // used again.
  Surface* surface2 = GetSurfaceForId(child_id);
  EXPECT_EQ(surface, surface2);
  EXPECT_FALSE(IsMarkedForDestruction(child_id));
  EXPECT_EQ(child_id, surface_observer().last_created_surface_id());
}

// Verifies that if a surface is marked destroyed and a new frame arrives after
// a CompositorFrameSink is destroyed and recreated then it will be recovered.
TEST_F(SurfaceSynchronizationTest, SurfaceResurrectionAfterDestruction) {
  const SurfaceId parent_id = MakeSurfaceId(kParentFrameSink, 1);
  const SurfaceId child_id = MakeSurfaceId(kChildFrameSink1, 3);

  // Create the child surface by submitting a frame to it.
  EXPECT_EQ(nullptr, GetSurfaceForId(child_id));
  child_support1().SubmitCompositorFrame(child_id.local_surface_id(),
                                         MakeDefaultCompositorFrame());

  // Verify that the child surface is created.
  Surface* surface = GetSurfaceForId(child_id);
  EXPECT_NE(nullptr, surface);

  // Add a reference from the parent to the child.
  parent_support().SubmitCompositorFrame(
      parent_id.local_surface_id(),
      MakeCompositorFrame({child_id}, {child_id},
                          std::vector<TransferableResource>()));

  // Attempt to destroy the child surface. The surface must still exist since
  // the parent needs it but it will be marked as destroyed.
  child_support1().EvictLastActivatedSurface();
  surface = GetSurfaceForId(child_id);
  EXPECT_NE(nullptr, surface);
  EXPECT_TRUE(IsMarkedForDestruction(child_id));

  // Child submits another frame to the same local surface id that is marked
  // destroyed.
  surface_observer().Reset();
  DestroyFrameSink(child_id.frame_sink_id());
  CreateFrameSink(child_id.frame_sink_id(), false);
  child_support1().SubmitCompositorFrame(child_id.local_surface_id(),
                                         MakeDefaultCompositorFrame());

  // Verify that the surface that was marked destroyed is recovered and is being
  // used again.
  Surface* surface2 = GetSurfaceForId(child_id);
  EXPECT_EQ(surface, surface2);
  EXPECT_FALSE(IsMarkedForDestruction(child_id));
  EXPECT_EQ(surface2->client().get(), &child_support1());
  EXPECT_EQ(child_id, surface_observer().last_created_surface_id());
}
// Verifies that if a LocalSurfaceId belonged to a surface that doesn't
// exist anymore, it can still be reused for new surfaces.
TEST_F(SurfaceSynchronizationTest, LocalSurfaceIdIsReusable) {
  const SurfaceId parent_id = MakeSurfaceId(kParentFrameSink, 1);
  const SurfaceId child_id = MakeSurfaceId(kChildFrameSink1, 3);

  // Submit the first frame. Creates the surface.
  child_support1().SubmitCompositorFrame(child_id.local_surface_id(),
                                         MakeDefaultCompositorFrame());
  EXPECT_NE(nullptr, GetSurfaceForId(child_id));

  // Add a reference from parent.
  parent_support().SubmitCompositorFrame(
      parent_id.local_surface_id(),
      MakeCompositorFrame({child_id}, {child_id},
                          std::vector<TransferableResource>()));

  // Remove the reference from parant. This allows us to destroy the surface.
  parent_support().SubmitCompositorFrame(parent_id.local_surface_id(),
                                         MakeDefaultCompositorFrame());

  // Destroy the surface.
  child_support1().EvictLastActivatedSurface();
  frame_sink_manager().surface_manager()->GarbageCollectSurfaces();

  EXPECT_EQ(nullptr, GetSurfaceForId(child_id));

  // Submit another frame with the same local surface id. This should work fine
  // and a new surface must be created.
  child_support1().SubmitCompositorFrame(child_id.local_surface_id(),
                                         MakeDefaultCompositorFrame());
  EXPECT_NE(nullptr, GetSurfaceForId(child_id));
}

// This test verifies that a crash does not occur if garbage collection is
// triggered during surface dependency resolution. This test triggers garbage
// collection during surface resolution, by causing an activation to remove
// a surface subtree from the root. Both the old subtree and the new
// activated subtree refer to the same dependency. The old subtree was activated
// by deadline, and the new subtree was activated by a dependency finally
// resolving.
TEST_F(SurfaceSynchronizationTest, DependencyTrackingGarbageCollection) {
  const SurfaceId display_id = MakeSurfaceId(kDisplayFrameSink, 1);
  const SurfaceId parent_id1 = MakeSurfaceId(kParentFrameSink, 1);
  const SurfaceId parent_id2 = MakeSurfaceId(kParentFrameSink, 2);
  const SurfaceId child_id = MakeSurfaceId(kChildFrameSink1, 1);

  parent_support().SubmitCompositorFrame(
      parent_id1.local_surface_id(),
      MakeCompositorFrame({child_id}, empty_surface_ids(),
                          std::vector<TransferableResource>(),
                          MakeDefaultDeadline()));
  display_support().SubmitCompositorFrame(
      display_id.local_surface_id(),
      MakeCompositorFrame({parent_id1}, empty_surface_ids(),
                          std::vector<TransferableResource>(),
                          MakeDefaultDeadline()));

  EXPECT_TRUE(parent_surface()->has_deadline());

  // Advance BeginFrames to trigger a deadline.
  for (int i = 0; i < 3; ++i) {
    SendNextBeginFrame();
    EXPECT_TRUE(display_surface()->has_deadline());
    EXPECT_TRUE(parent_surface()->has_deadline());
  }
  SendNextBeginFrame();

  EXPECT_TRUE(display_surface()->HasActiveFrame());
  EXPECT_FALSE(display_surface()->HasPendingFrame());
  EXPECT_TRUE(parent_surface()->HasActiveFrame());
  EXPECT_FALSE(parent_surface()->HasPendingFrame());

  parent_support().SubmitCompositorFrame(
      parent_id2.local_surface_id(),
      MakeCompositorFrame({child_id}, empty_surface_ids(),
                          std::vector<TransferableResource>(),
                          MakeDefaultDeadline()));
  display_support().SubmitCompositorFrame(
      display_id.local_surface_id(),
      MakeCompositorFrame({parent_id2}, empty_surface_ids(),
                          std::vector<TransferableResource>(),
                          MakeDefaultDeadline()));

  // The display surface now has two CompositorFrames. One that is pending,
  // indirectly blocked on child_id and one that is active, also indirectly
  // referring to child_id, but activated due to the deadline above.
  EXPECT_TRUE(display_surface()->HasActiveFrame());
  EXPECT_TRUE(display_surface()->HasPendingFrame());

  // Submitting a CompositorFrame will trigger garbage collection of the
  // |parent_id1| subtree. This should not crash.
  child_support1().SubmitCompositorFrame(child_id.local_surface_id(),
                                         MakeDefaultCompositorFrame());
}

// This test verifies that a crash does not occur if garbage collection is
// triggered when a deadline forces frame activation. This test triggers garbage
// collection during deadline activation by causing the activation of a display
// frame to replace a previously activated display frame that was referring to
// a now-unreachable surface subtree. That subtree gets garbage collected during
// deadline activation. SurfaceDependencyTracker is also tracking a surface
// from that subtree due to an unresolved dependency. This test verifies that
// this dependency resolution does not crash.
TEST_F(SurfaceSynchronizationTest, GarbageCollectionOnDeadline) {
  const SurfaceId display_id = MakeSurfaceId(kDisplayFrameSink, 1);
  const SurfaceId parent_id1 = MakeSurfaceId(kParentFrameSink, 1);
  const SurfaceId parent_id2 = MakeSurfaceId(kParentFrameSink, 2);
  const SurfaceId child_id = MakeSurfaceId(kChildFrameSink1, 1);

  // |parent_id1| is blocked on |child_id|.
  parent_support().SubmitCompositorFrame(
      parent_id1.local_surface_id(),
      MakeCompositorFrame({child_id}, empty_surface_ids(),
                          std::vector<TransferableResource>(),
                          MakeDefaultDeadline()));

  display_support().SubmitCompositorFrame(
      display_id.local_surface_id(),
      MakeCompositorFrame({parent_id1}, {parent_id1},
                          std::vector<TransferableResource>(),
                          MakeDefaultDeadline()));

  EXPECT_TRUE(display_surface()->has_deadline());
  EXPECT_TRUE(parent_surface()->has_deadline());
  EXPECT_TRUE(display_surface()->HasPendingFrame());
  EXPECT_FALSE(display_surface()->HasActiveFrame());

  // Advance BeginFrames to trigger a deadline. This activates the
  // CompositorFrame submitted above.
  for (int i = 0; i < 3; ++i) {
    SendNextBeginFrame();
    EXPECT_TRUE(display_surface()->has_deadline());
    EXPECT_TRUE(parent_surface()->has_deadline());
  }
  SendNextBeginFrame();
  EXPECT_FALSE(display_surface()->has_deadline());
  EXPECT_FALSE(parent_surface()->has_deadline());
  EXPECT_FALSE(display_surface()->HasPendingFrame());
  EXPECT_TRUE(display_surface()->HasActiveFrame());

  // By submitting a display CompositorFrame, and replacing the parent's
  // CompositorFrame with another surface ID, parent_id1 becomes unreachable
  // and a candidate for garbage collection.
  display_support().SubmitCompositorFrame(
      display_id.local_surface_id(),
      MakeCompositorFrame({parent_id2}, empty_surface_ids(),
                          std::vector<TransferableResource>(),
                          MakeDefaultDeadline()));
  EXPECT_TRUE(display_surface()->has_deadline());

  // Now |parent_id1| is only kept alive by the active |display_id| frame.
  parent_support().SubmitCompositorFrame(
      parent_id2.local_surface_id(),
      MakeCompositorFrame({child_id}, empty_surface_ids(),
                          std::vector<TransferableResource>(),
                          MakeDefaultDeadline()));
  EXPECT_TRUE(display_surface()->has_deadline());
  EXPECT_TRUE(parent_surface()->has_deadline());

  // SurfaceDependencyTracker should now be tracking |display_id|, |parent_id1|
  // and |parent_id2|. By activating the pending |display_id| frame by deadline,
  // |parent_id1| becomes unreachable and is garbage collected while
  // SurfaceDependencyTracker is in the process of activating surfaces. This
  // should not cause a crash or use-after-free.
  for (int i = 0; i < 3; ++i) {
    SendNextBeginFrame();
    EXPECT_TRUE(display_surface()->has_deadline());
  }
  SendNextBeginFrame();
  EXPECT_FALSE(display_surface()->has_deadline());
}

// This test verifies that a CompositorFrame will only blocked on embedded
// surfaces but not on other retained surface IDs in the CompositorFrame.
TEST_F(SurfaceSynchronizationTest, OnlyBlockOnEmbeddedSurfaces) {
  const SurfaceId display_id = MakeSurfaceId(kDisplayFrameSink, 1);
  const SurfaceId parent_id1 = MakeSurfaceId(kParentFrameSink, 1);
  const SurfaceId parent_id2 = MakeSurfaceId(kParentFrameSink, 2);

  // Submitting a CompositorFrame with |parent_id2| so that the display
  // CompositorFrame can hold a reference to it.
  parent_support().SubmitCompositorFrame(parent_id1.local_surface_id(),
                                         MakeDefaultCompositorFrame());

  display_support().SubmitCompositorFrame(
      display_id.local_surface_id(),
      MakeCompositorFrame({parent_id2}, {parent_id1},
                          std::vector<TransferableResource>()));

  EXPECT_TRUE(display_surface()->HasPendingFrame());
  EXPECT_FALSE(display_surface()->HasActiveFrame());
  EXPECT_TRUE(display_surface()->has_deadline());

  // Verify that the display CompositorFrame will only block on |parent_id2|
  // but not |parent_id1|.
  EXPECT_THAT(display_surface()->activation_dependencies(),
              UnorderedElementsAre(parent_id2));
  // Verify that the display surface holds no references while its
  // CompositorFrame is pending.
  EXPECT_THAT(GetChildReferences(display_id), IsEmpty());

  // Submitting a CompositorFrame with |parent_id2| should unblock the
  // display CompositorFrame.
  parent_support().SubmitCompositorFrame(parent_id2.local_surface_id(),
                                         MakeDefaultCompositorFrame());

  EXPECT_FALSE(display_surface()->has_deadline());
  EXPECT_FALSE(display_surface()->HasPendingFrame());
  EXPECT_TRUE(display_surface()->HasActiveFrame());
  EXPECT_THAT(display_surface()->activation_dependencies(), IsEmpty());
}

// This test verifies that a late arriving CompositorFrame activates
// immediately and does not trigger a new deadline.
TEST_F(SurfaceSynchronizationTest, LateArrivingDependency) {
  const SurfaceId display_id = MakeSurfaceId(kDisplayFrameSink, 1);
  const SurfaceId parent_id1 = MakeSurfaceId(kParentFrameSink, 1);
  const SurfaceId child_id1 = MakeSurfaceId(kChildFrameSink1, 1);

  display_support().SubmitCompositorFrame(
      display_id.local_surface_id(),
      MakeCompositorFrame({parent_id1}, empty_surface_ids(),
                          std::vector<TransferableResource>(),
                          MakeDefaultDeadline()));

  EXPECT_TRUE(display_surface()->HasPendingFrame());
  EXPECT_FALSE(display_surface()->HasActiveFrame());
  EXPECT_TRUE(display_surface()->has_deadline());

  // Advance BeginFrames to trigger a deadline. This activates the
  // CompositorFrame submitted above.
  for (int i = 0; i < 3; ++i) {
    SendNextBeginFrame();
    EXPECT_TRUE(display_surface()->has_deadline());
  }
  SendNextBeginFrame();
  EXPECT_FALSE(display_surface()->has_deadline());
  EXPECT_FALSE(display_surface()->HasPendingFrame());
  EXPECT_TRUE(display_surface()->HasActiveFrame());

  // A late arriving CompositorFrame should activate immediately without
  // scheduling a deadline and without waiting for dependencies to resolve.
  parent_support().SubmitCompositorFrame(
      parent_id1.local_surface_id(),
      MakeCompositorFrame({child_id1}, empty_surface_ids(),
                          std::vector<TransferableResource>(),
                          MakeDefaultDeadline()));
  EXPECT_FALSE(parent_surface()->has_deadline());
  EXPECT_FALSE(parent_surface()->HasPendingFrame());
  EXPECT_TRUE(parent_surface()->HasActiveFrame());
}

// This test verifies that a late arriving CompositorFrame activates
// immediately along with its subtree and does not trigger a new deadline.
TEST_F(SurfaceSynchronizationTest, MultiLevelLateArrivingDependency) {
  const SurfaceId display_id = MakeSurfaceId(kDisplayFrameSink, 1);
  const SurfaceId parent_id = MakeSurfaceId(kParentFrameSink, 1);
  const SurfaceId child_id = MakeSurfaceId(kChildFrameSink1, 1);
  const SurfaceId arbitrary_id = MakeSurfaceId(kArbitraryFrameSink, 1);

  display_support().SubmitCompositorFrame(
      display_id.local_surface_id(),
      MakeCompositorFrame({parent_id}, empty_surface_ids(),
                          std::vector<TransferableResource>(),
                          MakeDefaultDeadline()));
  EXPECT_TRUE(display_surface()->HasPendingFrame());
  EXPECT_FALSE(display_surface()->HasActiveFrame());
  EXPECT_TRUE(display_surface()->has_deadline());

  // Issue some BeginFrames to trigger the deadline and activate the display's
  // surface. |parent_id| is now late. Advance BeginFrames to trigger a
  // deadline.
  for (int i = 0; i < 4; ++i) {
    EXPECT_TRUE(display_surface()->has_deadline());
    SendNextBeginFrame();
  }
  EXPECT_FALSE(display_surface()->HasPendingFrame());
  EXPECT_TRUE(display_surface()->HasActiveFrame());
  EXPECT_FALSE(display_surface()->has_deadline());

  // The child surface is not currently causally linked to the display's
  // surface and so it gets a separate deadline.
  child_support1().SubmitCompositorFrame(
      child_id.local_surface_id(),
      MakeCompositorFrame({arbitrary_id}, empty_surface_ids(),
                          std::vector<TransferableResource>(),
                          MakeDefaultDeadline()));
  EXPECT_TRUE(child_surface1()->HasPendingFrame());
  EXPECT_FALSE(child_surface1()->HasActiveFrame());
  EXPECT_TRUE(child_surface1()->has_deadline());

  // Submitting a CompositorFrame to the parent surface creates a dependency
  // chain from the display to the parent to the child, allowing them all to
  // assume the same deadline. Both the parent and the child are determined to
  // be late and activate immediately.
  parent_support().SubmitCompositorFrame(
      parent_id.local_surface_id(),
      MakeCompositorFrame({child_id}, empty_surface_ids(),
                          std::vector<TransferableResource>(),
                          MakeDefaultDeadline()));
  EXPECT_FALSE(parent_surface()->HasPendingFrame());
  EXPECT_TRUE(parent_surface()->HasActiveFrame());
  EXPECT_FALSE(parent_surface()->has_deadline());

  EXPECT_FALSE(child_surface1()->HasPendingFrame());
  EXPECT_TRUE(child_surface1()->HasActiveFrame());
  EXPECT_FALSE(child_surface1()->has_deadline());
}

// This test verifies that a late arriving CompositorFrame does not activate
// if its dependent CompositorFrame has been dropped in the intervening time.
// This test also verifies that there is no crash when the dependent
// CompositorFrame is evicted and replaced by a new pending CompositorFrame.
TEST_F(SurfaceSynchronizationTest, MissingActiveFrameWithLateDependencies) {
  const SurfaceId display_id = MakeSurfaceId(kDisplayFrameSink, 1);
  const SurfaceId parent_id1 = MakeSurfaceId(kParentFrameSink, 1);
  const SurfaceId parent_id2 = MakeSurfaceId(kParentFrameSink, 2);
  const SurfaceId child_id1 = MakeSurfaceId(kChildFrameSink1, 1);

  display_support().SubmitCompositorFrame(
      display_id.local_surface_id(),
      MakeCompositorFrame({parent_id1}, empty_surface_ids(),
                          std::vector<TransferableResource>(),
                          MakeDefaultDeadline()));

  EXPECT_TRUE(display_surface()->HasPendingFrame());
  EXPECT_FALSE(display_surface()->HasActiveFrame());
  EXPECT_TRUE(display_surface()->has_deadline());

  // Advance BeginFrames to trigger a deadline. This activates the
  // CompositorFrame submitted above.
  for (int i = 0; i < 3; ++i) {
    SendNextBeginFrame();
    EXPECT_TRUE(display_surface()->has_deadline());
  }
  SendNextBeginFrame();
  EXPECT_FALSE(display_surface()->has_deadline());
  EXPECT_FALSE(display_surface()->HasPendingFrame());
  EXPECT_TRUE(display_surface()->HasActiveFrame());

  display_support().EvictLastActivatedSurface();
  display_support().SubmitCompositorFrame(
      display_id.local_surface_id(),
      MakeCompositorFrame({parent_id2}, empty_surface_ids(),
                          std::vector<TransferableResource>(),
                          MakeDefaultDeadline()));

  // A late arriving CompositorFrame will not activate immediately if the
  // dependent CompositorFrame has been evicted.
  parent_support().SubmitCompositorFrame(
      parent_id1.local_surface_id(),
      MakeCompositorFrame({child_id1}, empty_surface_ids(),
                          std::vector<TransferableResource>(),
                          MakeDefaultDeadline()));
  EXPECT_TRUE(parent_surface()->has_deadline());
  EXPECT_TRUE(parent_surface()->HasPendingFrame());
  EXPECT_FALSE(parent_surface()->HasActiveFrame());
}

// This test verifies that CompositorFrames submitted to a surface referenced
// by a parent CompositorFrame as a fallback will be rejected and ACK'ed
// immediately.
TEST_F(SurfaceSynchronizationTest, FallbackSurfacesClosed) {
  const SurfaceId parent_id1 = MakeSurfaceId(kParentFrameSink, 1);
  // This is the fallback child surface that the parent holds a reference to.
  const SurfaceId child_id1 = MakeSurfaceId(kChildFrameSink1, 1);
  // This is the primary child surface that the parent wants to block on.
  const SurfaceId child_id2 = MakeSurfaceId(kChildFrameSink1, 2);

  // child_support1 submits a CompositorFrame without any dependencies.
  // DidReceiveCompositorFrameAck should call on immediate activation.
  // However, resources will not be returned because this frame is a candidate
  // for display.
  TransferableResource resource;
  resource.id = 1337;
  resource.format = ALPHA_8;
  resource.filter = 1234;
  resource.size = gfx::Size(1234, 5678);
  std::vector<ReturnedResource> returned_resources =
      TransferableResource::ReturnResources({resource});

  EXPECT_CALL(support_client_, DidReceiveCompositorFrameAck(
                                   Eq(std::vector<ReturnedResource>())));
  child_support1().SubmitCompositorFrame(
      child_id1.local_surface_id(),
      MakeCompositorFrame(empty_surface_ids(), empty_surface_ids(), {resource},
                          MakeDefaultDeadline()));
  EXPECT_FALSE(child_surface1()->has_deadline());
  testing::Mock::VerifyAndClearExpectations(&support_client_);

  // The parent is blocked on |child_id2| and references |child_id1|. The
  // surface corresponding to |child_id1| will not accept new CompositorFrames
  // while the parent CompositorFrame is blocked.
  parent_support().SubmitCompositorFrame(
      parent_id1.local_surface_id(),
      MakeCompositorFrame({child_id2}, {child_id1},
                          std::vector<TransferableResource>(),
                          MakeDefaultDeadline()));
  EXPECT_TRUE(parent_surface()->has_deadline());
  EXPECT_TRUE(parent_surface()->HasPendingFrame());
  EXPECT_FALSE(parent_surface()->HasActiveFrame());

  // Resources will be returned immediately because |child_id1|'s surface is
  // closed.
  TransferableResource resource2;
  resource2.id = 1246;
  resource2.format = ALPHA_8;
  resource2.filter = 1357;
  resource2.size = gfx::Size(8765, 4321);
  std::vector<ReturnedResource> returned_resources2 =
      TransferableResource::ReturnResources({resource2});
  EXPECT_CALL(support_client_,
              DidReceiveCompositorFrameAck(Eq(returned_resources2)));
  child_support1().SubmitCompositorFrame(
      child_id1.local_surface_id(),
      MakeCompositorFrame(empty_surface_ids(), empty_surface_ids(), {resource2},
                          MakeDefaultDeadline()));
  testing::Mock::VerifyAndClearExpectations(&support_client_);

  // Advance BeginFrames to trigger a deadline. This activates the
  // CompositorFrame submitted to the parent.
  for (int i = 0; i < 3; ++i) {
    SendNextBeginFrame();
    EXPECT_TRUE(parent_surface()->has_deadline());
  }
  SendNextBeginFrame();
  EXPECT_FALSE(parent_surface()->has_deadline());
  EXPECT_FALSE(parent_surface()->HasPendingFrame());
  EXPECT_TRUE(parent_surface()->HasActiveFrame());

  // Resources will be returned immediately because |child_id1|'s surface is
  // closed forever.
  EXPECT_CALL(support_client_,
              DidReceiveCompositorFrameAck(Eq(returned_resources2)));
  child_support1().SubmitCompositorFrame(
      child_id1.local_surface_id(),
      MakeCompositorFrame(empty_surface_ids(), empty_surface_ids(), {resource2},
                          MakeDefaultDeadline()));
  testing::Mock::VerifyAndClearExpectations(&support_client_);
}

// This test verifies that two surface subtrees have independent deadlines.
TEST_F(SurfaceSynchronizationTest, IndependentDeadlines) {
  const SurfaceId parent_id1 = MakeSurfaceId(kParentFrameSink, 1);
  const SurfaceId child_id1 = MakeSurfaceId(kChildFrameSink1, 1);
  const SurfaceId child_id2 = MakeSurfaceId(kChildFrameSink2, 1);
  const SurfaceId arbitrary_id = MakeSurfaceId(kArbitraryFrameSink, 1);

  child_support1().SubmitCompositorFrame(child_id1.local_surface_id(),
                                         MakeDefaultCompositorFrame());
  EXPECT_FALSE(child_surface1()->HasPendingFrame());
  EXPECT_TRUE(child_surface1()->HasActiveFrame());

  child_support2().SubmitCompositorFrame(child_id2.local_surface_id(),
                                         MakeDefaultCompositorFrame());
  EXPECT_FALSE(child_surface2()->HasPendingFrame());
  EXPECT_TRUE(child_surface2()->HasActiveFrame());

  parent_support().SubmitCompositorFrame(
      parent_id1.local_surface_id(),
      MakeCompositorFrame({child_id1, child_id2}, empty_surface_ids(),
                          std::vector<TransferableResource>(),
                          MakeDefaultDeadline()));

  EXPECT_FALSE(parent_surface()->HasPendingFrame());
  EXPECT_TRUE(parent_surface()->HasActiveFrame());
  EXPECT_FALSE(parent_surface()->has_deadline());

  // Submit another CompositorFrame to |child_id1| that blocks on
  // |arbitrary_id|.
  child_support1().SubmitCompositorFrame(
      child_id1.local_surface_id(),
      MakeCompositorFrame(
          {arbitrary_id}, empty_surface_ids(),
          std::vector<TransferableResource>(),
          FrameDeadline(Now(), 3, BeginFrameArgs::DefaultInterval(), false)));
  EXPECT_TRUE(child_surface1()->HasPendingFrame());
  EXPECT_TRUE(child_surface1()->HasActiveFrame());
  EXPECT_TRUE(child_surface1()->has_deadline());

  // Advance to the next BeginFrame. |child_id1|'s pending Frame should activate
  // after 2 frames.
  SendNextBeginFrame();

  // Submit another CompositorFrame to |child_id2| that blocks on
  // |arbitrary_id|.
  child_support2().SubmitCompositorFrame(
      child_id2.local_surface_id(),
      MakeCompositorFrame({arbitrary_id}, empty_surface_ids(),
                          std::vector<TransferableResource>(),
                          MakeDefaultDeadline()));
  EXPECT_TRUE(child_surface2()->HasPendingFrame());
  EXPECT_TRUE(child_surface2()->HasActiveFrame());
  EXPECT_TRUE(child_surface2()->has_deadline());

  // If we issue another two BeginFrames both children should remain blocked.
  SendNextBeginFrame();
  EXPECT_TRUE(child_surface1()->has_deadline());
  EXPECT_TRUE(child_surface2()->has_deadline());

  // Issuing another BeginFrame should activate the frame in |child_id1| but not
  // |child_id2|. This verifies that |child_id1| and |child_id2| have different
  // deadlines.
  SendNextBeginFrame();

  EXPECT_TRUE(child_surface2()->has_deadline());

  EXPECT_FALSE(child_surface1()->has_deadline());
  EXPECT_FALSE(child_surface1()->HasPendingFrame());
  EXPECT_TRUE(child_surface1()->HasActiveFrame());

  SendNextBeginFrame();

  EXPECT_TRUE(child_surface2()->has_deadline());
  EXPECT_TRUE(child_surface2()->HasPendingFrame());
  EXPECT_TRUE(child_surface2()->HasActiveFrame());

  // Issuing another BeginFrame should activate the frame in |child_id2|.
  SendNextBeginFrame();

  EXPECT_FALSE(child_surface2()->has_deadline());
  EXPECT_FALSE(child_surface2()->HasPendingFrame());
  EXPECT_TRUE(child_surface2()->HasActiveFrame());
}

// This test verifies that a child inherits its deadline from its dependent
// parent (embedder) surface.
TEST_F(SurfaceSynchronizationTest, DeadlineInheritance) {
  const SurfaceId parent_id1 = MakeSurfaceId(kParentFrameSink, 1);
  const SurfaceId child_id1 = MakeSurfaceId(kChildFrameSink1, 1);
  const SurfaceId arbitrary_id = MakeSurfaceId(kArbitraryFrameSink, 1);

  // Using the default lower bound deadline results in the deadline of 2 frames
  // effectively being ignored because the default lower bound is 4 frames.
  parent_support().SubmitCompositorFrame(
      parent_id1.local_surface_id(),
      MakeCompositorFrame(
          {child_id1}, empty_surface_ids(), std::vector<TransferableResource>(),
          FrameDeadline(Now(), 2, BeginFrameArgs::DefaultInterval(), true)));

  EXPECT_TRUE(parent_surface()->HasPendingFrame());
  EXPECT_FALSE(parent_surface()->HasActiveFrame());
  EXPECT_TRUE(parent_surface()->has_deadline());

  // Advance to the next BeginFrame. The parent surface will activate in 3
  // frames.
  SendNextBeginFrame();

  child_support1().SubmitCompositorFrame(
      child_id1.local_surface_id(),
      MakeCompositorFrame({arbitrary_id}, empty_surface_ids(),
                          std::vector<TransferableResource>(),
                          MakeDefaultDeadline()));
  EXPECT_TRUE(child_surface1()->HasPendingFrame());
  EXPECT_FALSE(child_surface1()->HasActiveFrame());
  EXPECT_TRUE(child_surface1()->has_deadline());

  // If we issue another three BeginFrames then both the parent and the child
  // should activate, verifying that the child's deadline is inherited from the
  // parent.
  for (int i = 0; i < 3; ++i) {
    EXPECT_TRUE(parent_surface()->has_deadline());
    EXPECT_TRUE(child_surface1()->has_deadline());
    SendNextBeginFrame();
  }

  // Verify that both the parent and child have activated.
  EXPECT_FALSE(parent_surface()->HasPendingFrame());
  EXPECT_TRUE(parent_surface()->HasActiveFrame());
  EXPECT_FALSE(parent_surface()->has_deadline());

  EXPECT_FALSE(child_surface1()->HasPendingFrame());
  EXPECT_TRUE(child_surface1()->HasActiveFrame());
  EXPECT_FALSE(child_surface1()->has_deadline());
}

// This test verifies that all surfaces within a dependency chain will
// ultimately inherit the same deadline even if the grandchild is available
// before the child.
TEST_F(SurfaceSynchronizationTest, MultiLevelDeadlineInheritance) {
  const SurfaceId display_id = MakeSurfaceId(kDisplayFrameSink, 1);
  const SurfaceId parent_id = MakeSurfaceId(kParentFrameSink, 1);
  const SurfaceId child_id = MakeSurfaceId(kChildFrameSink1, 1);
  const SurfaceId arbitrary_id = MakeSurfaceId(kArbitraryFrameSink, 1);

  display_support().SubmitCompositorFrame(
      display_id.local_surface_id(),
      MakeCompositorFrame({parent_id}, empty_surface_ids(),
                          std::vector<TransferableResource>(),
                          MakeDefaultDeadline()));
  EXPECT_TRUE(display_surface()->HasPendingFrame());
  EXPECT_FALSE(display_surface()->HasActiveFrame());
  EXPECT_TRUE(display_surface()->has_deadline());

  // Issue a BeginFrame to move closer to the display's deadline.
  SendNextBeginFrame();

  // The child surface is not currently causally linked to the display's
  // surface and so it gets a separate deadline.
  child_support1().SubmitCompositorFrame(
      child_id.local_surface_id(),
      MakeCompositorFrame({arbitrary_id}, empty_surface_ids(),
                          std::vector<TransferableResource>(),
                          MakeDefaultDeadline()));
  EXPECT_TRUE(child_surface1()->HasPendingFrame());
  EXPECT_FALSE(child_surface1()->HasActiveFrame());
  EXPECT_TRUE(child_surface1()->has_deadline());

  // Submitting a CompositorFrame to the parent frame creates a dependency
  // chain from the display to the parent to the child, allowing them all to
  // assume the same deadline.
  parent_support().SubmitCompositorFrame(
      parent_id.local_surface_id(),
      MakeCompositorFrame({child_id}, empty_surface_ids(),
                          std::vector<TransferableResource>(),
                          MakeDefaultDeadline()));
  EXPECT_TRUE(parent_surface()->HasPendingFrame());
  EXPECT_FALSE(parent_surface()->HasActiveFrame());
  EXPECT_TRUE(parent_surface()->has_deadline());

  // Advancing the time by three BeginFrames should activate all the surfaces.
  for (int i = 0; i < 3; ++i) {
    EXPECT_TRUE(display_surface()->has_deadline());
    EXPECT_TRUE(parent_surface()->has_deadline());
    EXPECT_TRUE(child_surface1()->has_deadline());
    SendNextBeginFrame();
  }

  // Verify that all the CompositorFrames have activated.
  EXPECT_FALSE(display_surface()->HasPendingFrame());
  EXPECT_TRUE(display_surface()->HasActiveFrame());
  EXPECT_FALSE(display_surface()->has_deadline());

  EXPECT_FALSE(parent_surface()->HasPendingFrame());
  EXPECT_TRUE(parent_surface()->HasActiveFrame());
  EXPECT_FALSE(parent_surface()->has_deadline());

  EXPECT_FALSE(child_surface1()->HasPendingFrame());
  EXPECT_TRUE(child_surface1()->HasActiveFrame());
  EXPECT_FALSE(child_surface1()->has_deadline());
}

// This test verifies that no crash occurs if a CompositorFrame activates AFTER
// its FrameSink has been destroyed.
TEST_F(SurfaceSynchronizationTest, FrameActivationAfterFrameSinkDestruction) {
  const SurfaceId display_id = MakeSurfaceId(kDisplayFrameSink, 1);
  const SurfaceId parent_id = MakeSurfaceId(kParentFrameSink, 1);
  const SurfaceId child_id = MakeSurfaceId(kChildFrameSink1, 1);

  parent_support().SubmitCompositorFrame(parent_id.local_surface_id(),
                                         MakeDefaultCompositorFrame());

  EXPECT_FALSE(parent_surface()->has_deadline());
  EXPECT_TRUE(parent_surface()->HasActiveFrame());
  EXPECT_FALSE(parent_surface()->HasPendingFrame());

  // Submit a CompositorFrame that refers to to |parent_id|.
  display_support().SubmitCompositorFrame(
      display_id.local_surface_id(),
      MakeCompositorFrame(empty_surface_ids(), {parent_id},
                          std::vector<TransferableResource>()));

  EXPECT_FALSE(display_surface()->has_deadline());
  EXPECT_FALSE(display_surface()->HasPendingFrame());
  EXPECT_TRUE(display_surface()->HasActiveFrame());
  EXPECT_THAT(GetChildReferences(display_id), UnorderedElementsAre(parent_id));

  // Submit a new CompositorFrame to the parent CompositorFrameSink. It should
  // now have a pending and active CompositorFrame.
  parent_support().SubmitCompositorFrame(
      parent_id.local_surface_id(),
      MakeCompositorFrame({child_id}, empty_surface_ids(),
                          std::vector<TransferableResource>()));

  EXPECT_TRUE(parent_surface()->has_deadline());
  EXPECT_TRUE(parent_surface()->HasActiveFrame());
  EXPECT_TRUE(parent_surface()->HasPendingFrame());
  surface_observer().Reset();

  // Destroy the parent CompositorFrameSink. The parent_surface will be kept
  // alive by the display.
  DestroyFrameSink(kParentFrameSink);

  Surface* parent_surface = GetSurfaceForId(parent_id);
  ASSERT_NE(nullptr, parent_surface);

  EXPECT_TRUE(parent_surface->has_deadline());
  EXPECT_TRUE(parent_surface->HasActiveFrame());
  EXPECT_TRUE(parent_surface->HasPendingFrame());

  // Advance BeginFrames to trigger a deadline. This activates the
  // CompositorFrame submitted above.
  for (int i = 0; i < 4; ++i)
    SendNextBeginFrame();

  // The parent surface stays alive through the display.
  parent_surface = GetSurfaceForId(parent_id);
  EXPECT_NE(nullptr, parent_surface);
  EXPECT_TRUE(surface_observer().IsSurfaceDamaged(parent_id));

  // Submitting a new CompositorFrame to the display should free the parent.
  display_support().SubmitCompositorFrame(display_id.local_surface_id(),
                                          MakeDefaultCompositorFrame());

  frame_sink_manager().surface_manager()->GarbageCollectSurfaces();

  parent_surface = GetSurfaceForId(parent_id);
  EXPECT_EQ(nullptr, parent_surface);
}

TEST_F(SurfaceSynchronizationTest, PreviousFrameSurfaceId) {
  const SurfaceId parent_id1 = MakeSurfaceId(kParentFrameSink, 1);
  const SurfaceId parent_id2 = MakeSurfaceId(kParentFrameSink, 2);
  const SurfaceId child_id = MakeSurfaceId(kChildFrameSink1, 1);

  // Submit a frame with no dependencies to |parent_id1|.
  parent_support().SubmitCompositorFrame(
      parent_id1.local_surface_id(),
      MakeCompositorFrame(empty_surface_ids(), empty_surface_ids(),
                          std::vector<TransferableResource>()));

  // Submit a frame with unresolved dependencies to |parent_id2|. The frame
  // should become pending and previous_frame_surface_id() should return
  // |parent_id1|.
  parent_support().SubmitCompositorFrame(
      parent_id2.local_surface_id(),
      MakeCompositorFrame({child_id}, empty_surface_ids(),
                          std::vector<TransferableResource>()));
  Surface* parent_surface2 =
      frame_sink_manager().surface_manager()->GetSurfaceForId(parent_id2);
  EXPECT_FALSE(parent_surface2->HasActiveFrame());
  EXPECT_TRUE(parent_surface2->HasPendingFrame());

  // Activate the pending frame in |parent_id2|. previous_frame_surface_id()
  // should still return |parent_id1|.
  parent_surface2->ActivatePendingFrameForDeadline(base::nullopt);
  EXPECT_TRUE(parent_surface2->HasActiveFrame());
  EXPECT_FALSE(parent_surface2->HasPendingFrame());
  EXPECT_EQ(parent_id1, parent_surface2->previous_frame_surface_id());
}

TEST_F(SurfaceSynchronizationTest, FrameIndexWithPendingFrames) {
  const SurfaceId parent_id = MakeSurfaceId(kParentFrameSink, 1);
  const SurfaceId child_id1 = MakeSurfaceId(kChildFrameSink1, 1);
  constexpr int n_iterations = 7;

  // Submit a frame with no dependencies that will activate immediately. Record
  // the initial frame index.
  parent_support().SubmitCompositorFrame(
      parent_id.local_surface_id(),
      MakeCompositorFrame(empty_surface_ids(), empty_surface_ids(),
                          std::vector<TransferableResource>()));
  Surface* parent_surface =
      frame_sink_manager().surface_manager()->GetSurfaceForId(parent_id);
  uint64_t initial_frame_index = parent_surface->GetActiveFrameIndex();

  // Submit frames with unresolved dependencies. GetActiveFrameIndex should
  // return the same value as before.
  for (int i = 0; i < n_iterations; i++) {
    parent_support().SubmitCompositorFrame(
        parent_id.local_surface_id(),
        MakeCompositorFrame({child_id1}, empty_surface_ids(),
                            std::vector<TransferableResource>()));
    EXPECT_EQ(initial_frame_index, parent_surface->GetActiveFrameIndex());
  }

  // Activate the pending frame. GetActiveFrameIndex should return the frame
  // index of the newly activated frame.
  parent_surface->ActivatePendingFrameForDeadline(base::nullopt);
  EXPECT_EQ(initial_frame_index + n_iterations,
            parent_surface->GetActiveFrameIndex());
}

// This test verifies that a new surface with a pending CompositorFrame gets
// a temporary reference immediately, as opposed to when the surface activates.
TEST_F(SurfaceSynchronizationTest, PendingSurfaceKeptAlive) {
  SetFrameSinkHierarchy(kDisplayFrameSink, kParentFrameSink);
  SetFrameSinkHierarchy(kParentFrameSink, kChildFrameSink1);

  const SurfaceId parent_id = MakeSurfaceId(kParentFrameSink, 1);
  const SurfaceId child_id1 = MakeSurfaceId(kChildFrameSink1, 1);

  // |parent_id| depends on |child_id1|. It shouldn't activate.
  parent_support().SubmitCompositorFrame(
      parent_id.local_surface_id(),
      MakeCompositorFrame({child_id1}, empty_surface_ids(),
                          std::vector<TransferableResource>()));
  EXPECT_FALSE(parent_surface()->HasActiveFrame());
  EXPECT_TRUE(parent_surface()->HasPendingFrame());
  EXPECT_TRUE(HasTemporaryReference(parent_id));
}

// Tests getting the correct active frame index.
TEST_F(SurfaceSynchronizationTest, ActiveFrameIndex) {
  const SurfaceId parent_id = MakeSurfaceId(kParentFrameSink, 1);
  const SurfaceId child_id1 = MakeSurfaceId(kChildFrameSink1, 1);
  const SurfaceId child_id2 = MakeSurfaceId(kChildFrameSink2, 1);

  parent_support().SubmitCompositorFrame(
      parent_id.local_surface_id(),
      MakeCompositorFrame({child_id1, child_id2}, empty_surface_ids(),
                          std::vector<TransferableResource>()));

  // parent_support is blocked on |child_id1| and |child_id2|.
  EXPECT_FALSE(parent_surface()->HasActiveFrame());
  EXPECT_EQ(0u, parent_surface()->GetActiveFrameIndex());

  child_support1().SubmitCompositorFrame(child_id1.local_surface_id(),
                                         MakeDefaultCompositorFrame());
  child_support2().SubmitCompositorFrame(child_id2.local_surface_id(),
                                         MakeDefaultCompositorFrame());
  EXPECT_TRUE(parent_surface()->HasActiveFrame());
  EXPECT_EQ(3u, parent_surface()->GetActiveFrameIndex());
}

// This test verifies that SurfaceManager::GetLatestInFlightSurface returns
// the latest child surface not yet set as a fallback by the parent.
// Alternatively, it returns the fallback surface specified, if no tempoary
// references to child surfaces are available. This mechanism is used by surface
// synchronization to present the freshest surfaces available at aggregation
// time.
TEST_F(SurfaceSynchronizationTest, LatestInFlightSurface) {
  SetFrameSinkHierarchy(kParentFrameSink, kChildFrameSink1);

  const SurfaceId parent_id = MakeSurfaceId(kParentFrameSink, 1);
  const SurfaceId child_id1 = MakeSurfaceId(kChildFrameSink1, 1);
  const SurfaceId child_id2 = MakeSurfaceId(kChildFrameSink1, 2);
  const SurfaceId child_id3 = MakeSurfaceId(kChildFrameSink1, 2, 2);
  const SurfaceId child_id4 = MakeSurfaceId(kChildFrameSink1, 2, 3);

  child_support1().SubmitCompositorFrame(child_id1.local_surface_id(),
                                         MakeDefaultCompositorFrame());

  parent_support().SubmitCompositorFrame(
      parent_id.local_surface_id(),
      MakeCompositorFrame({child_id1}, empty_surface_ids(),
                          std::vector<TransferableResource>()));

  // Verify that the child CompositorFrame activates immediately.
  EXPECT_TRUE(child_surface1()->HasActiveFrame());
  EXPECT_FALSE(child_surface1()->HasPendingFrame());
  EXPECT_THAT(child_surface1()->activation_dependencies(), IsEmpty());

  // Verify that the parent Surface has activated.
  EXPECT_TRUE(parent_surface()->HasActiveFrame());
  EXPECT_FALSE(parent_surface()->HasPendingFrame());
  EXPECT_THAT(parent_surface()->activation_dependencies(), IsEmpty());

  // Verify that there is a temporary reference for the child and there is
  // no reference from the parent to the child yet.
  EXPECT_TRUE(HasTemporaryReference(child_id1));
  EXPECT_THAT(GetChildReferences(parent_id), IsEmpty());
  EXPECT_EQ(GetSurfaceForId(child_id1),
            GetLatestInFlightSurface(child_id2, child_id1));

  parent_support().SubmitCompositorFrame(
      parent_id.local_surface_id(),
      MakeCompositorFrame(empty_surface_ids(), {child_id1},
                          std::vector<TransferableResource>()));

  // Verify that the parent Surface has activated.
  EXPECT_TRUE(parent_surface()->HasActiveFrame());
  EXPECT_FALSE(parent_surface()->HasPendingFrame());
  EXPECT_THAT(parent_surface()->activation_dependencies(), IsEmpty());

  // Verify that there is no temporary reference for the child and there is
  // a reference from the parent to the child.
  EXPECT_FALSE(HasTemporaryReference(child_id1));
  EXPECT_THAT(GetChildReferences(parent_id), UnorderedElementsAre(child_id1));
  EXPECT_EQ(GetSurfaceForId(child_id1),
            GetLatestInFlightSurface(child_id2, child_id1));

  // Don't assign owner for temporary reference to |child_id2|.
  DisableAssignTemporaryReferences();

  // Submit a child CompositorFrame to a new SurfaceId and verify that
  // GetLatestInFlightSurface returns the right surface.
  child_support1().SubmitCompositorFrame(child_id2.local_surface_id(),
                                         MakeDefaultCompositorFrame());

  // Verify that there is a temporary reference for child_id2 and there is
  // a reference from the parent to child_id1.
  EXPECT_TRUE(HasTemporaryReference(child_id2));
  EXPECT_THAT(GetChildReferences(parent_id), UnorderedElementsAre(child_id1));

  EXPECT_EQ(GetSurfaceForId(child_id2),
            GetLatestInFlightSurface(child_id3, child_id1));

  // If the primary surface is old, then we shouldn't return an in-flight
  // surface that is newer than the primary.
  EXPECT_EQ(GetSurfaceForId(child_id1),
            GetLatestInFlightSurface(child_id1, child_id1));

  // Submit a child CompositorFrame to a new SurfaceId and verify that
  // GetLatestInFlightSurface returns the right surface.
  child_support1().SubmitCompositorFrame(child_id3.local_surface_id(),
                                         MakeDefaultCompositorFrame());

  // Verify that there is a temporary reference for child_id3.
  EXPECT_TRUE(HasTemporaryReference(child_id3));

  EXPECT_EQ(GetSurfaceForId(child_id3),
            GetLatestInFlightSurface(child_id4, child_id1));
  EXPECT_THAT(GetChildReferences(parent_id), UnorderedElementsAre(child_id1));

  // If the primary surface is old, then we shouldn't return an in-flight
  // surface that is newer than the primary.
  EXPECT_EQ(GetSurfaceForId(child_id2),
            GetLatestInFlightSurface(child_id3, child_id1));
}

// This test verifies that GetLatestInFlightSurface will return nullptr
// if it has a bogus fallback SurfaceID.
TEST_F(SurfaceSynchronizationTest, LatestInFlightSurfaceWithBogusFallback) {
  const SurfaceId parent_id = MakeSurfaceId(kParentFrameSink, 1);
  const SurfaceId child_id1 = MakeSurfaceId(kChildFrameSink1, 1);

  child_support1().SubmitCompositorFrame(child_id1.local_surface_id(),
                                         MakeDefaultCompositorFrame());

  parent_support().SubmitCompositorFrame(
      parent_id.local_surface_id(),
      MakeCompositorFrame({child_id1}, empty_surface_ids(),
                          std::vector<TransferableResource>()));

  // Verify that the parent and child CompositorFrames are active.
  EXPECT_TRUE(child_surface1()->HasActiveFrame());
  EXPECT_FALSE(child_surface1()->HasPendingFrame());
  EXPECT_THAT(child_surface1()->activation_dependencies(), IsEmpty());

  EXPECT_TRUE(parent_surface()->HasActiveFrame());
  EXPECT_FALSE(parent_surface()->HasPendingFrame());
  EXPECT_THAT(parent_surface()->activation_dependencies(), IsEmpty());

  // If the fallback surface doesn't exist, then GetLatestInFlightSurface should
  // always return nullptr.
  const SurfaceId bogus_child_id = MakeSurfaceId(kChildFrameSink1, 10);
  EXPECT_EQ(nullptr, GetLatestInFlightSurface(child_id1, bogus_child_id));
}

// This test verifies that GetLatestInFlightSurface will return the fallback
// surface if the primary and fallback SurfaceIds have different FrameSinkIds.
// This is important to preserve the property that that latest in-flight surface
// is no newer than the primary. If the FrameSinkId changes then we cannot be
// sure of that so we simply return the fallback surface.
TEST_F(SurfaceSynchronizationTest, LatestInFlightSurfaceDifferentFrameSinkIds) {
  const SurfaceId parent_id = MakeSurfaceId(kParentFrameSink, 1);
  const SurfaceId child_id1 = MakeSurfaceId(kChildFrameSink1, 1);
  const SurfaceId child_id2 = MakeSurfaceId(kChildFrameSink2, 1);

  child_support1().SubmitCompositorFrame(child_id1.local_surface_id(),
                                         MakeDefaultCompositorFrame());

  parent_support().SubmitCompositorFrame(
      parent_id.local_surface_id(),
      MakeCompositorFrame({child_id2}, {child_id1},
                          std::vector<TransferableResource>()));

  // Submit a child CompositorFrame without a different FrameSinkId and verify
  // that if the fallback and primary differ in FrameSinkId then
  // GetLatestInFlightSurface will always return the specified fallback.
  child_support2().SubmitCompositorFrame(child_id2.local_surface_id(),
                                         MakeDefaultCompositorFrame());

  // Submit a child CompositorFrame without a different FrameSinkId and verify
  // that if the fallback and primary differ in FrameSinkId then
  // GetLatestInFlightSurface will always return the specified fallback.
  child_support2().SubmitCompositorFrame(child_id2.local_surface_id(),
                                         MakeDefaultCompositorFrame());
  EXPECT_EQ(GetSurfaceForId(child_id1),
            GetLatestInFlightSurface(child_id2, child_id1));
}

// This test verifies that GetLatestInFlightSurface will never return the
// primary surface ID even if it is in the temporary reference queue.
TEST_F(SurfaceSynchronizationTest, LatestInFlightSurfaceSkipPrimary) {
  const SurfaceId parent_id = MakeSurfaceId(kParentFrameSink, 1);
  const SurfaceId child_id1 = MakeSurfaceId(kChildFrameSink1, 1);
  const SurfaceId child_id2 = MakeSurfaceId(kChildFrameSink1, 2);
  const SurfaceId child_id3 = MakeSurfaceId(kChildFrameSink1, 3);

  // Don't automatically assign temporary references.
  DisableAssignTemporaryReferences();

  child_support1().SubmitCompositorFrame(child_id1.local_surface_id(),
                                         MakeDefaultCompositorFrame());

  // Create a reference from |parent_id| to |child_id|.
  parent_support().SubmitCompositorFrame(
      parent_id.local_surface_id(),
      MakeCompositorFrame(empty_surface_ids(), {child_id1},
                          std::vector<TransferableResource>()));

  child_support1().SubmitCompositorFrame(child_id2.local_surface_id(),
                                         MakeDefaultCompositorFrame());

  EXPECT_EQ(GetSurfaceForId(child_id2),
            GetLatestInFlightSurface(child_id3, child_id1));

  child_support1().SubmitCompositorFrame(child_id3.local_surface_id(),
                                         MakeDefaultCompositorFrame());

  // GetLatestInFlightSurface will never return the primary surface ID
  // even if it's available.
  EXPECT_EQ(GetSurfaceForId(child_id2),
            GetLatestInFlightSurface(child_id3, child_id1));
}

// This test verifies that GetLatestInFlightSurface will skip a surface if
// its nonce is different.
TEST_F(SurfaceSynchronizationTest, LatestInFlightSurfaceSkipDifferentNonce) {
  const SurfaceId parent_id = MakeSurfaceId(kParentFrameSink, 1);
  const base::UnguessableToken nonce1(
      base::UnguessableToken::Deserialize(0, 1));
  const base::UnguessableToken nonce2(
      base::UnguessableToken::Deserialize(1, 1));
  const base::UnguessableToken nonce3(
      base::UnguessableToken::Deserialize(2, 1));
  const SurfaceId child_id1 =
      SurfaceId(kChildFrameSink1, LocalSurfaceId(1, nonce1));
  const SurfaceId child_id2 =
      SurfaceId(kChildFrameSink1, LocalSurfaceId(2, nonce1));
  const SurfaceId child_id3 =
      SurfaceId(kChildFrameSink1, LocalSurfaceId(3, nonce2));
  const SurfaceId child_id4 =
      SurfaceId(kChildFrameSink1, LocalSurfaceId(4, nonce2));
  const SurfaceId child_id5 =
      SurfaceId(kChildFrameSink1, LocalSurfaceId(5, nonce3));

  // Don't automatically assign temporary references.
  DisableAssignTemporaryReferences();

  child_support1().SubmitCompositorFrame(child_id1.local_surface_id(),
                                         MakeDefaultCompositorFrame());

  // Create a reference from |parent_id| to |child_id|.
  parent_support().SubmitCompositorFrame(
      parent_id.local_surface_id(),
      MakeCompositorFrame(empty_surface_ids(), {child_id1},
                          std::vector<TransferableResource>()));

  child_support1().SubmitCompositorFrame(child_id2.local_surface_id(),
                                         MakeDefaultCompositorFrame());

  EXPECT_EQ(GetSurfaceForId(child_id2),
            GetLatestInFlightSurface(child_id4, child_id1));

  child_support1().SubmitCompositorFrame(child_id3.local_surface_id(),
                                         MakeDefaultCompositorFrame());

  // GetLatestInFlightSurface will return child_id3 because the nonce
  // matches that of child_id4.
  EXPECT_EQ(GetSurfaceForId(child_id3),
            GetLatestInFlightSurface(child_id4, child_id1));

  // GetLatestInFlightSurface will return child_id2 because the nonce
  // doesn't match |child_id1| or |child_id5|.
  EXPECT_EQ(GetSurfaceForId(child_id2),
            GetLatestInFlightSurface(child_id5, child_id1));
}

// This test verifies that if a child submits a LocalSurfaceId newer that the
// parent's dependency, then the parent will drop its dependency and activate
// if possible.
TEST_F(SurfaceSynchronizationTest, DropDependenciesThatWillNeverArrive) {
  const SurfaceId parent_id = MakeSurfaceId(kParentFrameSink, 1);
  const SurfaceId child_id11 = MakeSurfaceId(kChildFrameSink1, 1);
  const SurfaceId child_id12 = MakeSurfaceId(kChildFrameSink1, 2);
  const SurfaceId child_id21 = MakeSurfaceId(kChildFrameSink2, 1);

  // |parent_id| depends on { child_id11, child_id21 }. It shouldn't activate.
  parent_support().SubmitCompositorFrame(
      parent_id.local_surface_id(),
      MakeCompositorFrame({child_id11, child_id21}, empty_surface_ids(),
                          std::vector<TransferableResource>()));
  EXPECT_FALSE(parent_surface()->HasActiveFrame());
  EXPECT_TRUE(parent_surface()->HasPendingFrame());

  // The first child submits a new CompositorFrame to |child_id12|. |parent_id|
  // no longer depends on |child_id11| because it cannot expect it to arrive.
  // However, the parent is still blocked on |child_id21|.
  child_support1().SubmitCompositorFrame(
      child_id12.local_surface_id(),
      MakeCompositorFrame(empty_surface_ids(), empty_surface_ids(),
                          std::vector<TransferableResource>()));
  EXPECT_FALSE(parent_surface()->HasActiveFrame());
  EXPECT_TRUE(parent_surface()->HasPendingFrame());
  EXPECT_THAT(parent_surface()->activation_dependencies(),
              UnorderedElementsAre(child_id21));

  // Finally, the second child submits a frame to the remaining dependency and
  // the parent activates.
  child_support2().SubmitCompositorFrame(
      child_id21.local_surface_id(),
      MakeCompositorFrame(empty_surface_ids(), empty_surface_ids(),
                          std::vector<TransferableResource>()));
  EXPECT_TRUE(parent_surface()->HasActiveFrame());
  EXPECT_FALSE(parent_surface()->HasPendingFrame());
  EXPECT_THAT(parent_surface()->activation_dependencies(), IsEmpty());
}

// This test verifies that a surface will continue to observe a child surface
// until its dependency arrives.
TEST_F(SurfaceSynchronizationTest, ObserveDependenciesUntilArrival) {
  const SurfaceId parent_id = MakeSurfaceId(kParentFrameSink, 1);
  const SurfaceId child_id21 = MakeSurfaceId(kChildFrameSink1, 2, 1);
  const SurfaceId child_id22 = MakeSurfaceId(kChildFrameSink1, 2, 2);

  // |parent_id| depends on |child_id22|. It shouldn't activate.
  parent_support().SubmitCompositorFrame(
      parent_id.local_surface_id(),
      MakeCompositorFrame({child_id22}, empty_surface_ids(),
                          std::vector<TransferableResource>()));
  EXPECT_FALSE(parent_surface()->HasActiveFrame());
  EXPECT_TRUE(parent_surface()->HasPendingFrame());

  // The child submits to |child_id21|. The parent should not activate.
  child_support1().SubmitCompositorFrame(
      child_id21.local_surface_id(),
      MakeCompositorFrame(empty_surface_ids(), empty_surface_ids(),
                          std::vector<TransferableResource>()));
  EXPECT_FALSE(parent_surface()->HasActiveFrame());
  EXPECT_TRUE(parent_surface()->HasPendingFrame());
  EXPECT_THAT(parent_surface()->activation_dependencies(),
              UnorderedElementsAre(child_id22));

  // The child submits to |child_id22|. The parent should activate.
  child_support1().SubmitCompositorFrame(
      child_id22.local_surface_id(),
      MakeCompositorFrame(empty_surface_ids(), empty_surface_ids(),
                          std::vector<TransferableResource>()));
  EXPECT_TRUE(parent_surface()->HasActiveFrame());
  EXPECT_FALSE(parent_surface()->HasPendingFrame());
  EXPECT_THAT(parent_surface()->activation_dependencies(), IsEmpty());
}

// This test verifies that we don't mark the previous active surface for
// destruction until the new surface activates.
TEST_F(SurfaceSynchronizationTest,
       MarkPreviousSurfaceForDestructionAfterActivation) {
  const SurfaceId parent_id1 = MakeSurfaceId(kParentFrameSink, 1);
  const SurfaceId parent_id2 = MakeSurfaceId(kParentFrameSink, 2);
  const SurfaceId arbitrary_id = MakeSurfaceId(kArbitraryFrameSink, 1);

  // Submit a CompositorFrame that has no dependencies.
  parent_support().SubmitCompositorFrame(parent_id1.local_surface_id(),
                                         MakeDefaultCompositorFrame());

  // Verify that the CompositorFrame has been activated.
  Surface* parent_surface1 = GetSurfaceForId(parent_id1);
  EXPECT_TRUE(parent_surface1->HasActiveFrame());
  EXPECT_FALSE(parent_surface1->HasPendingFrame());
  EXPECT_THAT(parent_surface1->activation_dependencies(), IsEmpty());
  EXPECT_FALSE(IsMarkedForDestruction(parent_id1));

  parent_support().SubmitCompositorFrame(
      parent_id2.local_surface_id(),
      MakeCompositorFrame({arbitrary_id}, empty_surface_ids(),
                          std::vector<TransferableResource>(),
                          MakeDefaultDeadline()));

  // Verify that the CompositorFrame to the new surface has not been activated.
  Surface* parent_surface2 = GetSurfaceForId(parent_id2);
  EXPECT_FALSE(parent_surface2->HasActiveFrame());
  EXPECT_TRUE(parent_surface2->HasPendingFrame());
  EXPECT_THAT(parent_surface2->activation_dependencies(),
              UnorderedElementsAre(arbitrary_id));
  EXPECT_FALSE(IsMarkedForDestruction(parent_id1));
  EXPECT_FALSE(IsMarkedForDestruction(parent_id2));

  // Advance BeginFrames to trigger a deadline.
  for (int i = 0; i < 3; ++i) {
    SendNextBeginFrame();
    EXPECT_TRUE(parent_surface2->has_deadline());
  }
  SendNextBeginFrame();
  EXPECT_FALSE(parent_surface2->has_deadline());

  // Verify that the CompositorFrame has been activated.
  EXPECT_TRUE(parent_surface2->HasActiveFrame());
  EXPECT_FALSE(parent_surface2->HasPendingFrame());
  EXPECT_THAT(parent_surface2->activation_dependencies(), IsEmpty());

  // Verify that the old surface is now marked for destruction.
  EXPECT_TRUE(IsMarkedForDestruction(parent_id1));
  EXPECT_FALSE(IsMarkedForDestruction(parent_id2));
}

// This test verifies that CompositorFrameSinkSupport does not refer to
// a valid but non-existant |last_activated_surface_id_|.
TEST_F(SurfaceSynchronizationTest, SetPreviousFrameSurfaceDoesntCrash) {
  const SurfaceId parent_id = MakeSurfaceId(kParentFrameSink, 1);
  const SurfaceId parent_id2 = MakeSurfaceId(kParentFrameSink, 2);
  const SurfaceId child_id1 = MakeSurfaceId(kChildFrameSink1, 1);

  // The parent CompositorFrame is not blocked on anything and so it should
  // immediately activate.
  EXPECT_FALSE(parent_support().last_activated_surface_id().is_valid());
  parent_support().SubmitCompositorFrame(parent_id.local_surface_id(),
                                         MakeDefaultCompositorFrame());

  // Verify that the parent CompositorFrame has activated.
  EXPECT_FALSE(parent_surface()->has_deadline());
  EXPECT_TRUE(parent_surface()->HasActiveFrame());
  EXPECT_FALSE(parent_surface()->HasPendingFrame());
  EXPECT_THAT(parent_surface()->activation_dependencies(), IsEmpty());
  EXPECT_TRUE(parent_support().last_activated_surface_id().is_valid());

  // Submit another CompositorFrame to |parent_id|, but this time block it
  // on |child_id1|.
  parent_support().SubmitCompositorFrame(
      parent_id.local_surface_id(),
      MakeCompositorFrame({child_id1}, empty_surface_ids(),
                          std::vector<TransferableResource>(),
                          MakeDefaultDeadline()));

  // Verify that the surface has both a pending and activate CompositorFrame.
  EXPECT_TRUE(parent_surface()->has_deadline());
  EXPECT_TRUE(parent_surface()->HasActiveFrame());
  EXPECT_TRUE(parent_surface()->HasPendingFrame());
  EXPECT_THAT(parent_surface()->activation_dependencies(),
              UnorderedElementsAre(child_id1));

  // Evict the activated surface in the parent_support.
  EXPECT_TRUE(parent_support().last_activated_surface_id().is_valid());
  parent_support().EvictLastActivatedSurface();
  EXPECT_FALSE(parent_support().last_activated_surface_id().is_valid());

  // The CompositorFrame in the evicted |parent_id| activates here because it
  // was blocked on |child_id1|.
  child_support1().SubmitCompositorFrame(child_id1.local_surface_id(),
                                         MakeDefaultCompositorFrame());

  // parent_support will be informed of the activation of a CompositorFrame
  // associated with |parent_id|.
  EXPECT_TRUE(parent_support().last_activated_surface_id().is_valid());

  // Perform a garbage collection. |parent_id| should no longer exist.
  EXPECT_NE(nullptr, GetSurfaceForId(parent_id));
  frame_sink_manager().surface_manager()->GarbageCollectSurfaces();
  EXPECT_EQ(nullptr, GetSurfaceForId(parent_id));

  // This should not crash as the previous surface was cleared in
  // CompositorFrameSinkSupport.
  parent_support().SubmitCompositorFrame(parent_id2.local_surface_id(),
                                         MakeDefaultCompositorFrame());
}

}  // namespace test
}  // namespace viz
