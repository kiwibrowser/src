// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CC_RESOURCES_DISPLAY_RESOURCE_PROVIDER_H_
#define CC_RESOURCES_DISPLAY_RESOURCE_PROVIDER_H_

#include "build/build_config.h"
#include "cc/output/overlay_candidate.h"
#include "cc/resources/resource_provider.h"

namespace viz {
class SharedBitmapManager;
}  // namespace viz

namespace cc {

// This class is not thread-safe and can only be called from the thread it was
// created on.
class CC_EXPORT DisplayResourceProvider : public ResourceProvider {
 public:
  DisplayResourceProvider(
      viz::ContextProvider* compositor_context_provider,
      viz::SharedBitmapManager* shared_bitmap_manager,
      gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager,
      bool delegated_sync_points_required,
      const viz::ResourceSettings& resource_settings);
  ~DisplayResourceProvider() override;

#if defined(OS_ANDROID)
  // Send an overlay promotion hint to all resources that requested it via
  // |wants_promotion_hints_set_|.  |promotable_hints| contains all the
  // resources that should be told that they're promotable.  Others will be told
  // that they're not promotable right now.
  void SendPromotionHints(
      const OverlayCandidateList::PromotionHintInfoMap& promotion_hints);
#endif

  // The following lock classes are part of the DisplayResourceProvider API and
  // are needed to read the resource contents. The user must ensure that they
  // only use GL locks on GL resources, etc, and this is enforced by assertions.
  class CC_EXPORT ScopedReadLockGL {
   public:
    ScopedReadLockGL(DisplayResourceProvider* resource_provider,
                     viz::ResourceId resource_id);
    ~ScopedReadLockGL();

    GLuint texture_id() const { return texture_id_; }
    GLenum target() const { return target_; }
    const gfx::Size& size() const { return size_; }
    const gfx::ColorSpace& color_space() const { return color_space_; }

   private:
    DisplayResourceProvider* const resource_provider_;
    const viz::ResourceId resource_id_;

    GLuint texture_id_;
    GLenum target_;
    gfx::Size size_;
    gfx::ColorSpace color_space_;

    DISALLOW_COPY_AND_ASSIGN(ScopedReadLockGL);
  };

  class CC_EXPORT ScopedSamplerGL {
   public:
    ScopedSamplerGL(DisplayResourceProvider* resource_provider,
                    viz::ResourceId resource_id,
                    GLenum filter);
    ScopedSamplerGL(DisplayResourceProvider* resource_provider,
                    viz::ResourceId resource_id,
                    GLenum unit,
                    GLenum filter);
    ~ScopedSamplerGL();

    GLuint texture_id() const { return resource_lock_.texture_id(); }
    GLenum target() const { return target_; }
    const gfx::ColorSpace& color_space() const {
      return resource_lock_.color_space();
    }

   private:
    const ScopedReadLockGL resource_lock_;
    const GLenum unit_;
    const GLenum target_;

    DISALLOW_COPY_AND_ASSIGN(ScopedSamplerGL);
  };

  class CC_EXPORT ScopedReadLockSkImage {
   public:
    ScopedReadLockSkImage(DisplayResourceProvider* resource_provider,
                          viz::ResourceId resource_id);
    ~ScopedReadLockSkImage();

    const SkImage* sk_image() const { return sk_image_.get(); }

    bool valid() const { return !!sk_image_; }

   private:
    DisplayResourceProvider* const resource_provider_;
    const viz::ResourceId resource_id_;
    sk_sp<SkImage> sk_image_;

    DISALLOW_COPY_AND_ASSIGN(ScopedReadLockSkImage);
  };

  class CC_EXPORT ScopedReadLockSoftware {
   public:
    ScopedReadLockSoftware(DisplayResourceProvider* resource_provider,
                           viz::ResourceId resource_id);
    ~ScopedReadLockSoftware();

    const SkBitmap* sk_bitmap() const {
      DCHECK(valid());
      return &sk_bitmap_;
    }

    bool valid() const { return !!sk_bitmap_.getPixels(); }

   private:
    DisplayResourceProvider* const resource_provider_;
    const viz::ResourceId resource_id_;
    SkBitmap sk_bitmap_;

    DISALLOW_COPY_AND_ASSIGN(ScopedReadLockSoftware);
  };

  // All resources that are returned to children while an instance of this
  // class exists will be stored and returned when the instance is destroyed.
  class CC_EXPORT ScopedBatchReturnResources {
   public:
    explicit ScopedBatchReturnResources(
        DisplayResourceProvider* resource_provider);
    ~ScopedBatchReturnResources();

   private:
    DisplayResourceProvider* const resource_provider_;

    DISALLOW_COPY_AND_ASSIGN(ScopedBatchReturnResources);
  };

  // Sets the current read fence. If a resource is locked for read
  // and has read fences enabled, the resource will not allow writes
  // until this fence has passed.
  void SetReadLockFence(Fence* fence) { current_read_lock_fence_ = fence; }

  // Creates accounting for a child. Returns a child ID.
  int CreateChild(const ReturnCallback& return_callback);

  // Destroys accounting for the child, deleting all accounted resources.
  void DestroyChild(int child);

  // Sets whether resources need sync points set on them when returned to this
  // child. Defaults to true.
  void SetChildNeedsSyncTokens(int child, bool needs_sync_tokens);

  // Gets the child->parent resource ID map.
  const ResourceIdMap& GetChildToParentMap(int child) const;

  // Receives resources from a child, moving them from mailboxes. Resource IDs
  // passed are in the child namespace, and will be translated to the parent
  // namespace, added to the child->parent map.
  // This adds the resources to the working set in the ResourceProvider without
  // declaring which resources are in use. Use DeclareUsedResourcesFromChild
  // after calling this method to do that. All calls to ReceiveFromChild should
  // be followed by a DeclareUsedResourcesFromChild.
  // NOTE: if the sync_token is set on any viz::TransferableResource, this will
  // wait on it.
  void ReceiveFromChild(
      int child,
      const std::vector<viz::TransferableResource>& transferable_resources);

  // Once a set of resources have been received, they may or may not be used.
  // This declares what set of resources are currently in use from the child,
  // releasing any other resources back to the child.
  void DeclareUsedResourcesFromChild(
      int child,
      const viz::ResourceIdSet& resources_from_child);

 private:
  friend class ScopedBatchReturnResources;

  const Resource* LockForRead(viz::ResourceId id);
  void UnlockForRead(viz::ResourceId id);

  struct Child {
    Child();
    Child(const Child& other);
    ~Child();

    ResourceIdMap child_to_parent_map;
    ReturnCallback return_callback;
    bool marked_for_deletion;
    bool needs_sync_tokens;
  };
  using ChildMap = std::unordered_map<int, Child>;

  void DeleteAndReturnUnusedResourcesToChild(ChildMap::iterator child_it,
                                             DeleteStyle style,
                                             const ResourceIdArray& unused);
  void DestroyChildInternal(ChildMap::iterator it, DeleteStyle style);

  void SetBatchReturnResources(bool aggregate);

  scoped_refptr<Fence> current_read_lock_fence_;
  ChildMap children_;
  base::flat_map<viz::ResourceId, sk_sp<SkImage>> resource_sk_image_;

  DISALLOW_COPY_AND_ASSIGN(DisplayResourceProvider);
};

}  // namespace cc

#endif  // CC_RESOURCES_DISPLAY_RESOURCE_PROVIDER_H_
