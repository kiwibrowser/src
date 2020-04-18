// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_SERVICE_FRAME_SINKS_FRAME_SINK_MANAGER_IMPL_H_
#define COMPONENTS_VIZ_SERVICE_FRAME_SINKS_FRAME_SINK_MANAGER_IMPL_H_

#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "base/containers/unique_ptr_adapters.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/optional.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_checker.h"
#include "components/viz/common/constants.h"
#include "components/viz/common/surfaces/frame_sink_id.h"
#include "components/viz/service/frame_sinks/frame_sink_observer.h"
#include "components/viz/service/frame_sinks/primary_begin_frame_source.h"
#include "components/viz/service/frame_sinks/video_capture/frame_sink_video_capturer_manager.h"
#include "components/viz/service/frame_sinks/video_detector.h"
#include "components/viz/service/hit_test/hit_test_aggregator_delegate.h"
#include "components/viz/service/hit_test/hit_test_manager.h"
#include "components/viz/service/surfaces/surface_manager.h"
#include "components/viz/service/surfaces/surface_observer.h"
#include "components/viz/service/viz_service_export.h"
#include "gpu/ipc/common/surface_handle.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/viz/privileged/interfaces/compositing/frame_sink_manager.mojom.h"
#include "services/viz/privileged/interfaces/compositing/frame_sink_video_capture.mojom.h"
#include "services/viz/public/interfaces/compositing/video_detector_observer.mojom.h"

namespace viz {

class CapturableFrameSink;
class CompositorFrameSinkSupport;
class DisplayProvider;

// FrameSinkManagerImpl manages BeginFrame hierarchy. This is the implementation
// detail for FrameSinkManagerImpl.
class VIZ_SERVICE_EXPORT FrameSinkManagerImpl
    : public SurfaceObserver,
      public FrameSinkVideoCapturerManager,
      public mojom::FrameSinkManager,
      public HitTestAggregatorDelegate {
 public:
  FrameSinkManagerImpl(base::Optional<uint32_t> activation_deadline_in_frames =
                           kDefaultActivationDeadlineInFrames,
                       DisplayProvider* display_provider = nullptr);
  ~FrameSinkManagerImpl() override;

  // Binds |this| as a FrameSinkManagerImpl for |request| on |task_runner|. On
  // Mac |task_runner| will be the resize helper task runner. May only be called
  // once.
  void BindAndSetClient(mojom::FrameSinkManagerRequest request,
                        scoped_refptr<base::SingleThreadTaskRunner> task_runner,
                        mojom::FrameSinkManagerClientPtr client);

  // Sets up a direction connection to |client| without using Mojo.
  void SetLocalClient(mojom::FrameSinkManagerClient* client);

  // mojom::FrameSinkManager implementation:
  void RegisterFrameSinkId(const FrameSinkId& frame_sink_id) override;
  void InvalidateFrameSinkId(const FrameSinkId& frame_sink_id) override;
  void EnableSynchronizationReporting(
      const FrameSinkId& frame_sink_id,
      const std::string& reporting_label) override;
  void SetFrameSinkDebugLabel(const FrameSinkId& frame_sink_id,
                              const std::string& debug_label) override;
  void CreateRootCompositorFrameSink(
      mojom::RootCompositorFrameSinkParamsPtr params) override;
  void CreateCompositorFrameSink(
      const FrameSinkId& frame_sink_id,
      mojom::CompositorFrameSinkRequest request,
      mojom::CompositorFrameSinkClientPtr client) override;
  void DestroyCompositorFrameSink(
      const FrameSinkId& frame_sink_id,
      DestroyCompositorFrameSinkCallback callback) override;
  void RegisterFrameSinkHierarchy(
      const FrameSinkId& parent_frame_sink_id,
      const FrameSinkId& child_frame_sink_id) override;
  void UnregisterFrameSinkHierarchy(
      const FrameSinkId& parent_frame_sink_id,
      const FrameSinkId& child_frame_sink_id) override;
  void AssignTemporaryReference(const SurfaceId& surface_id,
                                const FrameSinkId& owner) override;
  void DropTemporaryReference(const SurfaceId& surface_id) override;
  void AddVideoDetectorObserver(
      mojom::VideoDetectorObserverPtr observer) override;
  void CreateVideoCapturer(
      mojom::FrameSinkVideoCapturerRequest request) override;
  void EvictSurfaces(const std::vector<SurfaceId>& surface_ids) override;
  void RequestCopyOfOutput(const SurfaceId& surface_id,
                           std::unique_ptr<CopyOutputRequest> request) override;

  // SurfaceObserver implementation.
  void OnSurfaceCreated(const SurfaceId& surface_id) override;
  void OnFirstSurfaceActivation(const SurfaceInfo& surface_info) override;
  void OnSurfaceActivated(const SurfaceId& surface_id,
                          base::Optional<base::TimeDelta> duration) override;
  bool OnSurfaceDamaged(const SurfaceId& surface_id,
                        const BeginFrameAck& ack) override;
  void OnSurfaceDiscarded(const SurfaceId& surface_id) override;
  void OnSurfaceDestroyed(const SurfaceId& surface_id) override;
  void OnSurfaceDamageExpected(const SurfaceId& surface_id,
                               const BeginFrameArgs& args) override;

  // HitTestAggregatorDelegate implementation:
  void OnAggregatedHitTestRegionListUpdated(
      const FrameSinkId& frame_sink_id,
      const std::vector<AggregatedHitTestRegion>& hit_test_data) override;

  // CompositorFrameSinkSupport, hierarchy, and BeginFrameSource can be
  // registered and unregistered in any order with respect to each other.
  //
  // This happens in practice, e.g. the relationship to between ui::Compositor /
  // DelegatedFrameHost is known before ui::Compositor has a surface/client).
  // However, DelegatedFrameHost can register itself as a client before its
  // relationship with the ui::Compositor is known.

  // Registers a CompositorFrameSinkSupport for |frame_sink_id|. |frame_sink_id|
  // must be unregistered when |support| is destroyed.
  void RegisterCompositorFrameSinkSupport(const FrameSinkId& frame_sink_id,
                                          CompositorFrameSinkSupport* support);
  void UnregisterCompositorFrameSinkSupport(const FrameSinkId& frame_sink_id);

  // Associates a |source| with a particular framesink.  That framesink and
  // any children of that framesink with valid clients can potentially use
  // that |source|.
  void RegisterBeginFrameSource(BeginFrameSource* source,
                                const FrameSinkId& frame_sink_id);
  void UnregisterBeginFrameSource(BeginFrameSource* source);

  // Returns a stable BeginFrameSource that forwards BeginFrames from the first
  // available BeginFrameSource.
  BeginFrameSource* GetPrimaryBeginFrameSource();

  SurfaceManager* surface_manager() { return &surface_manager_; }

  const HitTestManager* hit_test_manager() { return &hit_test_manager_; }

  void SubmitHitTestRegionList(
      const SurfaceId& surface_id,
      uint64_t frame_index,
      base::Optional<HitTestRegionList> hit_test_region_list);

  // Instantiates |video_detector_| for tests where we simulate the passage of
  // time.
  VideoDetector* CreateVideoDetectorForTesting(
      const base::TickClock* tick_clock,
      scoped_refptr<base::SequencedTaskRunner> task_runner);

  // Called when |frame_token| is changed on a submitted CompositorFrame.
  void OnFrameTokenChanged(const FrameSinkId& frame_sink_id,
                           uint32_t frame_token);

  void AddObserver(FrameSinkObserver* obs);
  void RemoveObserver(FrameSinkObserver* obs);

  // Returns ids of all FrameSinks that were created.
  std::vector<FrameSinkId> GetCreatedFrameSinkIds() const;
  // Returns ids of all FrameSinks that were registered.
  std::vector<FrameSinkId> GetRegisteredFrameSinkIds() const;

  // Returns children of a FrameSink that has |parent_frame_sink_id|.
  // Returns an empty set if a parent doesn't have any children.
  base::flat_set<FrameSinkId> GetChildrenByParent(
      const FrameSinkId& parent_frame_sink_id) const;
  const CompositorFrameSinkSupport* GetFrameSinkForId(
      const FrameSinkId& frame_sink_id) const;

 private:
  friend class FrameSinkManagerTest;

  // BeginFrameSource routing information for a FrameSinkId.
  struct FrameSinkSourceMapping {
    FrameSinkSourceMapping();
    FrameSinkSourceMapping(FrameSinkSourceMapping&& other);
    ~FrameSinkSourceMapping();
    FrameSinkSourceMapping& operator=(FrameSinkSourceMapping&& other);

    bool has_children() const { return !children.empty(); }
    // The currently assigned begin frame source for this client.
    BeginFrameSource* source = nullptr;
    // This represents a dag of parent -> children mapping.
    base::flat_set<FrameSinkId> children;

   private:
    DISALLOW_COPY_AND_ASSIGN(FrameSinkSourceMapping);
  };

  void RecursivelyAttachBeginFrameSource(const FrameSinkId& frame_sink_id,
                                         BeginFrameSource* source);
  void RecursivelyDetachBeginFrameSource(const FrameSinkId& frame_sink_id,
                                         BeginFrameSource* source);

  // FrameSinkVideoCapturerManager implementation:
  CapturableFrameSink* FindCapturableFrameSink(
      const FrameSinkId& frame_sink_id) override;
  void OnCapturerConnectionLost(FrameSinkVideoCapturerImpl* capturer) override;

  // Returns true if |child framesink| is or has |search_frame_sink_id| as a
  // child.
  bool ChildContains(const FrameSinkId& child_frame_sink_id,
                     const FrameSinkId& search_frame_sink_id) const;

  // Provides a Display for CreateRootCompositorFrameSink().
  DisplayProvider* const display_provider_;

  // Set of BeginFrameSource along with associated FrameSinkIds. Any child
  // that is implicitly using this frame sink must be reachable by the
  // parent in the dag.
  base::flat_map<BeginFrameSource*, FrameSinkId> registered_sources_;

  // Contains FrameSinkId hierarchy and BeginFrameSource mapping.
  base::flat_map<FrameSinkId, FrameSinkSourceMapping> frame_sink_source_map_;

  // CompositorFrameSinkSupports get added to this map on creation and removed
  // on destruction.
  base::flat_map<FrameSinkId, CompositorFrameSinkSupport*> support_map_;

  // [Root]CompositorFrameSinkImpls are owned in this map.
  base::flat_map<FrameSinkId, std::unique_ptr<mojom::CompositorFrameSink>>
      sink_map_;

  // The set of FrameSinkIds that the client wants synchronization event
  // notifications for.
  base::flat_map<FrameSinkId, std::string> synchronization_event_labels_;

  PrimaryBeginFrameSource primary_source_;

  // |surface_manager_| should be placed under |primary_source_| so that all
  // surfaces are destroyed before |primary_source_|.
  SurfaceManager surface_manager_;

  HitTestManager hit_test_manager_;

  base::flat_set<std::unique_ptr<FrameSinkVideoCapturerImpl>,
                 base::UniquePtrComparator>
      video_capturers_;

  THREAD_CHECKER(thread_checker_);

  // |video_detector_| is instantiated lazily in order to avoid overhead on
  // platforms that don't need video detection.
  std::unique_ptr<VideoDetector> video_detector_;

  // This will point to |client_ptr_| if using Mojo or a provided client if
  // directly connected. Use this to make function calls.
  mojom::FrameSinkManagerClient* client_ = nullptr;

  mojom::FrameSinkManagerClientPtr client_ptr_;
  mojo::Binding<mojom::FrameSinkManager> binding_;

  base::ObserverList<FrameSinkObserver> observer_list_;

  DISALLOW_COPY_AND_ASSIGN(FrameSinkManagerImpl);
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_SERVICE_FRAME_SINKS_FRAME_SINK_MANAGER_IMPL_H_
