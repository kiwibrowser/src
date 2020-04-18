// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_TEST_LAYER_TEST_COMMON_H_
#define CC_TEST_LAYER_TEST_COMMON_H_

#include <stddef.h>

#include <memory>
#include <utility>

#include "cc/animation/animation_timeline.h"
#include "cc/test/fake_layer_tree_host.h"
#include "cc/test/fake_layer_tree_host_client.h"
#include "cc/test/test_task_graph_runner.h"
#include "cc/trees/effect_node.h"
#include "cc/trees/layer_tree_host_impl.h"
#include "components/viz/common/quads/render_pass.h"

#define EXPECT_SET_NEEDS_COMMIT(expect, code_to_test)                 \
  do {                                                                \
    EXPECT_CALL(*layer_tree_host_, SetNeedsCommit()).Times((expect)); \
    code_to_test;                                                     \
    Mock::VerifyAndClearExpectations(layer_tree_host_.get());         \
  } while (false)

#define EXPECT_SET_NEEDS_UPDATE(expect, code_to_test)                       \
  do {                                                                      \
    EXPECT_CALL(*layer_tree_host_, SetNeedsUpdateLayers()).Times((expect)); \
    code_to_test;                                                           \
    Mock::VerifyAndClearExpectations(layer_tree_host_.get());               \
  } while (false)

namespace gfx { class Rect; }

namespace viz {
class QuadList;
}

namespace cc {
class LayerImpl;
class LayerTreeFrameSink;
class RenderSurfaceImpl;

// Returns the RenderSurfaceImpl into which the given layer draws.
RenderSurfaceImpl* GetRenderSurface(LayerImpl* layer_impl);

class LayerTestCommon {
 public:
  static const char* quad_string;

  static void VerifyQuadsExactlyCoverRect(const viz::QuadList& quads,
                                          const gfx::Rect& rect);

  static void VerifyQuadsAreOccluded(const viz::QuadList& quads,
                                     const gfx::Rect& occluded,
                                     size_t* partially_occluded_count);

  class LayerImplTest {
   public:
    LayerImplTest();
    explicit LayerImplTest(
        std::unique_ptr<LayerTreeFrameSink> layer_tree_frame_sink);
    explicit LayerImplTest(const LayerTreeSettings& settings);
    LayerImplTest(const LayerTreeSettings& settings,
                  std::unique_ptr<LayerTreeFrameSink> layer_tree_frame_sink);
    ~LayerImplTest();

    template <typename T>
    T* AddChildToRoot() {
      std::unique_ptr<T> layer =
          T::Create(host_->host_impl()->active_tree(), layer_impl_id_++);
      T* ptr = layer.get();
      root_layer_for_testing()->test_properties()->AddChild(std::move(layer));
      return ptr;
    }

    template <typename T>
    T* AddChild(LayerImpl* parent) {
      std::unique_ptr<T> layer =
          T::Create(host_->host_impl()->active_tree(), layer_impl_id_++);
      T* ptr = layer.get();
      parent->test_properties()->AddChild(std::move(layer));
      return ptr;
    }

    template <typename T>
    T* AddMaskLayer(LayerImpl* origin) {
      std::unique_ptr<T> layer =
          T::Create(host_->host_impl()->active_tree(), layer_impl_id_++);
      T* ptr = layer.get();
      origin->test_properties()->SetMaskLayer(std::move(layer));
      return ptr;
    }

    template <typename T, typename A>
    T* AddChildToRoot(const A& a) {
      std::unique_ptr<T> layer =
          T::Create(host_->host_impl()->active_tree(), layer_impl_id_++, a);
      T* ptr = layer.get();
      root_layer_for_testing()->test_properties()->AddChild(std::move(layer));
      return ptr;
    }

    template <typename T, typename A, typename B>
    T* AddChildToRoot(const A& a, const B& b) {
      std::unique_ptr<T> layer =
          T::Create(host_->host_impl()->active_tree(), layer_impl_id_++, a, b);
      T* ptr = layer.get();
      root_layer_for_testing()->test_properties()->AddChild(std::move(layer));
      return ptr;
    }

    template <typename T, typename A, typename B, typename C>
    T* AddChildToRoot(const A& a, const B& b, const C& c) {
      std::unique_ptr<T> layer = T::Create(host_->host_impl()->active_tree(),
                                           layer_impl_id_++, a, b, c);
      T* ptr = layer.get();
      root_layer_for_testing()->test_properties()->AddChild(std::move(layer));
      return ptr;
    }

    template <typename T, typename A, typename B, typename C, typename D>
    T* AddChildToRoot(const A& a, const B& b, const C& c, const D& d) {
      std::unique_ptr<T> layer = T::Create(host_->host_impl()->active_tree(),
                                           layer_impl_id_++, a, b, c, d);
      T* ptr = layer.get();
      root_layer_for_testing()->test_properties()->AddChild(std::move(layer));
      return ptr;
    }

    template <typename T,
              typename A,
              typename B,
              typename C,
              typename D,
              typename E>
    T* AddChildToRoot(const A& a,
                      const B& b,
                      const C& c,
                      const D& d,
                      const E& e) {
      std::unique_ptr<T> layer = T::Create(host_->host_impl()->active_tree(),
                                           layer_impl_id_++, a, b, c, d, e);
      T* ptr = layer.get();
      root_layer_for_testing()->test_properties()->AddChild(std::move(layer));
      return ptr;
    }

    template <typename T,
              typename A,
              typename B,
              typename C,
              typename D,
              typename E>
    T* AddChild(LayerImpl* parent,
                const A& a,
                const B& b,
                const C& c,
                const D& d,
                const E& e) {
      std::unique_ptr<T> layer = T::Create(host_->host_impl()->active_tree(),
                                           layer_impl_id_++, a, b, c, d, e);
      T* ptr = layer.get();
      parent->test_properties()->AddChild(std::move(layer));
      return ptr;
    }

    void CalcDrawProps(const gfx::Size& viewport_size);
    void AppendQuadsWithOcclusion(LayerImpl* layer_impl,
                                  const gfx::Rect& occluded);
    void AppendQuadsForPassWithOcclusion(LayerImpl* layer_impl,
                                         viz::RenderPass* given_render_pass,
                                         const gfx::Rect& occluded);
    void AppendSurfaceQuadsWithOcclusion(RenderSurfaceImpl* surface_impl,
                                         const gfx::Rect& occluded);

    void RequestCopyOfOutput();

    LayerTreeFrameSink* layer_tree_frame_sink() const {
      return host_->host_impl()->layer_tree_frame_sink();
    }
    LayerTreeResourceProvider* resource_provider() const {
      return host_->host_impl()->resource_provider();
    }
    LayerImpl* root_layer_for_testing() const {
      return host_impl()->active_tree()->root_layer_for_testing();
    }
    FakeLayerTreeHost* host() { return host_.get(); }
    FakeLayerTreeHostImpl* host_impl() const { return host_->host_impl(); }
    TaskRunnerProvider* task_runner_provider() const {
      return host_->host_impl()->task_runner_provider();
    }
    const viz::QuadList& quad_list() const { return render_pass_->quad_list; }
    scoped_refptr<AnimationTimeline> timeline() { return timeline_; }
    scoped_refptr<AnimationTimeline> timeline_impl() { return timeline_impl_; }

    void BuildPropertyTreesForTesting() {
      host_impl()->active_tree()->BuildPropertyTreesForTesting();
    }
    void SetElementIdsForTesting() {
      host_impl()->active_tree()->SetElementIdsForTesting();
    }

   private:
    FakeLayerTreeHostClient client_;
    TestTaskGraphRunner task_graph_runner_;
    std::unique_ptr<LayerTreeFrameSink> layer_tree_frame_sink_;
    std::unique_ptr<AnimationHost> animation_host_;
    std::unique_ptr<FakeLayerTreeHost> host_;
    std::unique_ptr<viz::RenderPass> render_pass_;
    scoped_refptr<AnimationTimeline> timeline_;
    scoped_refptr<AnimationTimeline> timeline_impl_;
    int layer_impl_id_;
  };
};

}  // namespace cc

#endif  // CC_TEST_LAYER_TEST_COMMON_H_
