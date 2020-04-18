// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SERVICES_PDF_COMPOSITOR_PDF_COMPOSITOR_IMPL_H_
#define COMPONENTS_SERVICES_PDF_COMPOSITOR_PDF_COMPOSITOR_IMPL_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "base/macros.h"
#include "base/memory/read_only_shared_memory_region.h"
#include "base/memory/ref_counted_memory.h"
#include "base/memory/shared_memory.h"
#include "base/optional.h"
#include "components/services/pdf_compositor/public/cpp/pdf_service_mojo_types.h"
#include "components/services/pdf_compositor/public/interfaces/pdf_compositor.mojom.h"
#include "services/service_manager/public/cpp/service_context_ref.h"
#include "third_party/skia/include/core/SkPicture.h"
#include "third_party/skia/include/core/SkRefCnt.h"

namespace printing {

class PdfCompositorImpl : public mojom::PdfCompositor {
 public:
  PdfCompositorImpl(
      const std::string& creator,
      std::unique_ptr<service_manager::ServiceContextRef> service_ref);
  ~PdfCompositorImpl() override;

  // mojom::PdfCompositor
  void NotifyUnavailableSubframe(uint64_t frame_guid) override;
  void AddSubframeContent(
      uint64_t frame_guid,
      mojo::ScopedSharedBufferHandle serialized_content,
      const ContentToFrameMap& subframe_content_map) override;
  void CompositePageToPdf(
      uint64_t frame_guid,
      uint32_t page_num,
      mojo::ScopedSharedBufferHandle serialized_content,
      const ContentToFrameMap& subframe_content_map,
      mojom::PdfCompositor::CompositePageToPdfCallback callback) override;
  void CompositeDocumentToPdf(
      uint64_t frame_guid,
      mojo::ScopedSharedBufferHandle serialized_content,
      const ContentToFrameMap& subframe_content_map,
      mojom::PdfCompositor::CompositeDocumentToPdfCallback callback) override;
  void SetWebContentsURL(const GURL& url) override;

 protected:
  // This is the uniform underlying type for both
  // mojom::PdfCompositor::CompositePageToPdfCallback and
  // mojom::PdfCompositor::CompositeDocumentToPdfCallback.
  using CompositeToPdfCallback =
      base::OnceCallback<void(PdfCompositor::Status,
                              base::ReadOnlySharedMemoryRegion)>;

  // Make this function virtual so tests can override it.
  virtual void FulfillRequest(
      uint64_t frame_guid,
      base::Optional<uint32_t> page_num,
      std::unique_ptr<base::SharedMemory> serialized_content,
      const ContentToFrameMap& subframe_content_map,
      CompositeToPdfCallback callback);

 private:
  FRIEND_TEST_ALL_PREFIXES(PdfCompositorImplTest, IsReadyToComposite);
  FRIEND_TEST_ALL_PREFIXES(PdfCompositorImplTest, MultiLayerDependency);
  FRIEND_TEST_ALL_PREFIXES(PdfCompositorImplTest, DependencyLoop);

  // The map needed during content deserialization. It stores the mapping
  // between content id and its actual content.
  using DeserializationContext = base::flat_map<uint32_t, sk_sp<SkPicture>>;

  // Base structure to store a frame's content and its subframe
  // content information.
  struct FrameContentInfo {
    FrameContentInfo(std::unique_ptr<base::SharedMemory> content,
                     const ContentToFrameMap& map);
    FrameContentInfo();
    ~FrameContentInfo();

    // Serialized SkPicture content of this frame.
    std::unique_ptr<base::SharedMemory> serialized_content;

    // Frame content after composition with subframe content.
    sk_sp<SkPicture> content;

    // Subframe content id and its corresponding frame guid.
    ContentToFrameMap subframe_content_map;
  };

  // Other than content, it also stores the status during frame composition.
  struct FrameInfo : public FrameContentInfo {
    FrameInfo();
    ~FrameInfo();

    // The following fields are used for storing composition status.
    // Set to true when this frame's |serialized_content| is composed with
    // subframe content and the final result is stored in |content|.
    bool composited = false;
  };

  // Stores the mapping between frame's global unique ids and their
  // corresponding frame information.
  using FrameMap = base::flat_map<uint64_t, std::unique_ptr<FrameInfo>>;

  // Stores the page or document's request information.
  struct RequestInfo : public FrameContentInfo {
    RequestInfo(uint64_t frame_guid,
                base::Optional<uint32_t> page_num,
                std::unique_ptr<base::SharedMemory> content,
                const ContentToFrameMap& content_info,
                const base::flat_set<uint64_t>& pending_subframes,
                CompositeToPdfCallback callback);
    ~RequestInfo();

    uint64_t frame_guid;
    base::Optional<uint32_t> page_number;

    // All pending frame ids whose content is not available but needed
    // for composition.
    base::flat_set<uint64_t> pending_subframes;

    CompositeToPdfCallback callback;
  };

  // Check whether any request is waiting for the specific subframe, if so,
  // update its dependecy with the subframe's pending child frames.
  void UpdateRequestsWithSubframeInfo(
      uint64_t frame_guid,
      const std::vector<uint64_t>& pending_subframes);

  // Check whether the frame with a list of subframe content is ready to
  // composite. If not, return all unavailable frames' ids in
  // |pending_subframes|.
  bool IsReadyToComposite(uint64_t frame_guid,
                          const ContentToFrameMap& subframe_content_map,
                          base::flat_set<uint64_t>* pending_subframes);

  // Recursive check all the frames |frame_guid| depends on and put those
  // not ready in |pending_subframes|.
  void CheckFramesForReadiness(uint64_t frame_guid,
                               const ContentToFrameMap& subframe_content_map,
                               base::flat_set<uint64_t>* pending_subframes,
                               base::flat_set<uint64_t>* visited);

  // The internal implementation for handling page and documentation composition
  // requests.
  void HandleCompositionRequest(
      uint64_t frame_guid,
      base::Optional<uint32_t> page_num,
      mojo::ScopedSharedBufferHandle serialized_content,
      const ContentToFrameMap& subframe_content_ids,
      CompositeToPdfCallback callback);

  // The core function for content composition and conversion to a pdf file.
  mojom::PdfCompositor::Status CompositeToPdf(
      uint64_t frame_guid,
      base::Optional<uint32_t> page_num,
      std::unique_ptr<base::SharedMemory> shared_mem,
      const ContentToFrameMap& subframe_content_map,
      base::ReadOnlySharedMemoryRegion* region);

  // Composite the content of a subframe.
  sk_sp<SkPicture> CompositeSubframe(uint64_t frame_guid);

  bool CheckForPendingFrame(uint64_t frame_guid,
                            base::flat_set<uint64_t> visited_frames);

  DeserializationContext GetDeserializationContext(
      const ContentToFrameMap& subframe_content_map);

  const std::unique_ptr<service_manager::ServiceContextRef> service_ref_;
  const std::string creator_;

  // Keep track of all frames' information indexed by frame id.
  FrameMap frame_info_map_;

  std::vector<std::unique_ptr<RequestInfo>> requests_;

  DISALLOW_COPY_AND_ASSIGN(PdfCompositorImpl);
};

}  // namespace printing

#endif  // COMPONENTS_SERVICES_PDF_COMPOSITOR_PDF_COMPOSITOR_IMPL_H_
