// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/services/pdf_compositor/pdf_compositor_impl.h"

#include <algorithm>
#include <tuple>
#include <utility>

#include "base/logging.h"
#include "base/memory/shared_memory_handle.h"
#include "base/stl_util.h"
#include "components/crash/core/common/crash_key.h"
#include "components/services/pdf_compositor/public/cpp/pdf_service_mojo_types.h"
#include "components/services/pdf_compositor/public/cpp/pdf_service_mojo_utils.h"
#include "mojo/public/cpp/system/platform_handle.h"
#include "printing/common/pdf_metafile_utils.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkDocument.h"
#include "third_party/skia/include/core/SkSerialProcs.h"
#include "third_party/skia/src/utils/SkMultiPictureDocument.h"

namespace printing {

PdfCompositorImpl::PdfCompositorImpl(
    const std::string& creator,
    std::unique_ptr<service_manager::ServiceContextRef> service_ref)
    : service_ref_(std::move(service_ref)), creator_(creator) {}

PdfCompositorImpl::~PdfCompositorImpl() = default;

void PdfCompositorImpl::NotifyUnavailableSubframe(uint64_t frame_guid) {
  // Add this frame into the map.
  DCHECK(!base::ContainsKey(frame_info_map_, frame_guid));
  auto& frame_info =
      frame_info_map_.emplace(frame_guid, std::make_unique<FrameInfo>())
          .first->second;
  frame_info->composited = true;
  // Set content to be nullptr so it will be replaced by an empty picture during
  // deserialization of its parent.
  frame_info->content = nullptr;

  // Update the requests in case any of them might be waiting for this frame.
  UpdateRequestsWithSubframeInfo(frame_guid, std::vector<uint64_t>());
}

void PdfCompositorImpl::AddSubframeContent(
    uint64_t frame_guid,
    mojo::ScopedSharedBufferHandle serialized_content,
    const ContentToFrameMap& subframe_content_map) {
  // Add this frame and its serialized content.
  DCHECK(!base::ContainsKey(frame_info_map_, frame_guid));
  auto& frame_info =
      frame_info_map_.emplace(frame_guid, std::make_unique<FrameInfo>())
          .first->second;
  frame_info->serialized_content =
      GetShmFromMojoHandle(std::move(serialized_content));

  // Copy the subframe content information.
  frame_info->subframe_content_map = subframe_content_map;

  // If there is no request, we do nothing more.
  // Otherwise, we need to check whether any request actually waits on this
  // frame content.
  if (requests_.empty())
    return;

  // Get the pending list which is a list of subframes this frame needs
  // but are still unavailable.
  std::vector<uint64_t> pending_subframes;
  for (auto& subframe_content : subframe_content_map) {
    auto subframe_guid = subframe_content.second;
    if (!base::ContainsKey(frame_info_map_, subframe_guid))
      pending_subframes.push_back(subframe_guid);
  }

  // Update the requests in case any of them is waiting for this frame.
  UpdateRequestsWithSubframeInfo(frame_guid, pending_subframes);
}

void PdfCompositorImpl::CompositePageToPdf(
    uint64_t frame_guid,
    uint32_t page_num,
    mojo::ScopedSharedBufferHandle serialized_content,
    const ContentToFrameMap& subframe_content_map,
    mojom::PdfCompositor::CompositePageToPdfCallback callback) {
  HandleCompositionRequest(frame_guid, page_num, std::move(serialized_content),
                           subframe_content_map, std::move(callback));
}

void PdfCompositorImpl::CompositeDocumentToPdf(
    uint64_t frame_guid,
    mojo::ScopedSharedBufferHandle serialized_content,
    const ContentToFrameMap& subframe_content_map,
    mojom::PdfCompositor::CompositeDocumentToPdfCallback callback) {
  HandleCompositionRequest(frame_guid, base::nullopt,
                           std::move(serialized_content), subframe_content_map,
                           std::move(callback));
}

void PdfCompositorImpl::SetWebContentsURL(const GURL& url) {
  // Record the most recent url we tried to print. This should be sufficient
  // for users using print preview by default.
  static crash_reporter::CrashKeyString<1024> crash_key("main-frame-url");
  crash_key.Set(url.spec());
}

void PdfCompositorImpl::UpdateRequestsWithSubframeInfo(
    uint64_t frame_guid,
    const std::vector<uint64_t>& pending_subframes) {
  // Check for each request's pending list.
  for (auto it = requests_.begin(); it != requests_.end();) {
    auto& request = *it;
    // If the request needs this frame, we can remove the dependency, but
    // update with this frame's pending list.
    auto& pending_list = request->pending_subframes;
    if (pending_list.erase(frame_guid)) {
      std::copy(pending_subframes.begin(), pending_subframes.end(),
                std::inserter(pending_list, pending_list.end()));
      if (pending_list.empty()) {
        // If the request isn't waiting on any subframes then it is ready.
        // Fulfill the request now.
        FulfillRequest(request->frame_guid, request->page_number,
                       std::move(request->serialized_content),
                       request->subframe_content_map,
                       std::move(request->callback));
        it = requests_.erase(it);
        continue;
      }
    }
    // If the request still has pending frames, keep waiting.
    ++it;
  }
}

bool PdfCompositorImpl::IsReadyToComposite(
    uint64_t frame_guid,
    const ContentToFrameMap& subframe_content_map,
    base::flat_set<uint64_t>* pending_subframes) {
  pending_subframes->clear();
  base::flat_set<uint64_t> visited_frames;
  visited_frames.insert(frame_guid);
  CheckFramesForReadiness(frame_guid, subframe_content_map, pending_subframes,
                          &visited_frames);
  return pending_subframes->empty();
}

void PdfCompositorImpl::CheckFramesForReadiness(
    uint64_t frame_guid,
    const ContentToFrameMap& subframe_content_map,
    base::flat_set<uint64_t>* pending_subframes,
    base::flat_set<uint64_t>* visited) {
  for (auto& subframe_content : subframe_content_map) {
    auto subframe_guid = subframe_content.second;
    // If this frame has been checked, skip it.
    auto result = visited->insert(subframe_guid);
    if (!result.second)
      continue;

    auto iter = frame_info_map_.find(subframe_guid);
    if (iter == frame_info_map_.end()) {
      pending_subframes->insert(subframe_guid);
    } else {
      CheckFramesForReadiness(subframe_guid, iter->second->subframe_content_map,
                              pending_subframes, visited);
    }
  }
}

void PdfCompositorImpl::HandleCompositionRequest(
    uint64_t frame_guid,
    base::Optional<uint32_t> page_num,
    mojo::ScopedSharedBufferHandle serialized_content,
    const ContentToFrameMap& subframe_content_map,
    CompositeToPdfCallback callback) {
  base::flat_set<uint64_t> pending_subframes;
  if (IsReadyToComposite(frame_guid, subframe_content_map,
                         &pending_subframes)) {
    FulfillRequest(frame_guid, page_num,
                   GetShmFromMojoHandle(std::move(serialized_content)),
                   subframe_content_map, std::move(callback));
    return;
  }

  // When it is not ready yet, keep its information and
  // wait until all dependent subframes are ready.
  auto iter = frame_info_map_.find(frame_guid);
  if (iter == frame_info_map_.end())
    frame_info_map_[frame_guid] = std::make_unique<FrameInfo>();

  requests_.push_back(std::make_unique<RequestInfo>(
      frame_guid, page_num, GetShmFromMojoHandle(std::move(serialized_content)),
      subframe_content_map, std::move(pending_subframes), std::move(callback)));
}

mojom::PdfCompositor::Status PdfCompositorImpl::CompositeToPdf(
    uint64_t frame_guid,
    base::Optional<uint32_t> page_num,
    std::unique_ptr<base::SharedMemory> shared_mem,
    const ContentToFrameMap& subframe_content_map,
    base::ReadOnlySharedMemoryRegion* region) {
  DeserializationContext subframes =
      GetDeserializationContext(subframe_content_map);

  // Read in content and convert it into pdf.
  SkMemoryStream stream(shared_mem->memory(), shared_mem->mapped_size());
  int page_count = SkMultiPictureDocumentReadPageCount(&stream);
  if (!page_count) {
    DLOG(ERROR) << "CompositeToPdf: No page is read.";
    return mojom::PdfCompositor::Status::CONTENT_FORMAT_ERROR;
  }

  std::vector<SkDocumentPage> pages(page_count);
  SkDeserialProcs procs = DeserializationProcs(&subframes);
  if (!SkMultiPictureDocumentRead(&stream, pages.data(), page_count, &procs)) {
    DLOG(ERROR) << "CompositeToPdf: Page reading failed.";
    return mojom::PdfCompositor::Status::CONTENT_FORMAT_ERROR;
  }

  SkDynamicMemoryWStream wstream;
  sk_sp<SkDocument> doc = MakePdfDocument(creator_, &wstream);

  for (const auto& page : pages) {
    SkCanvas* canvas = doc->beginPage(page.fSize.width(), page.fSize.height());
    canvas->drawPicture(page.fPicture);
    doc->endPage();
  }
  doc->close();

  base::MappedReadOnlyRegion region_mapping =
      CreateReadOnlySharedMemoryRegion(wstream.bytesWritten());
  if (!region_mapping.IsValid()) {
    DLOG(ERROR) << "CompositeToPdf: Cannot create new shared memory region.";
    return mojom::PdfCompositor::Status::HANDLE_MAP_ERROR;
  }

  wstream.copyToAndReset(region_mapping.mapping.memory());
  *region = std::move(region_mapping.region);
  return mojom::PdfCompositor::Status::SUCCESS;
}

sk_sp<SkPicture> PdfCompositorImpl::CompositeSubframe(uint64_t frame_guid) {
  // The content of this frame should be available.
  auto iter = frame_info_map_.find(frame_guid);
  DCHECK(iter != frame_info_map_.end());

  std::unique_ptr<FrameInfo>& frame_info = iter->second;
  frame_info->composited = true;

  // Composite subframes first.
  DeserializationContext subframes =
      GetDeserializationContext(frame_info->subframe_content_map);

  // Composite the entire frame.
  SkMemoryStream stream(iter->second->serialized_content->memory(),
                        iter->second->serialized_content->mapped_size());
  SkDeserialProcs procs = DeserializationProcs(&subframes);
  iter->second->content = SkPicture::MakeFromStream(&stream, &procs);
  return iter->second->content;
}

PdfCompositorImpl::DeserializationContext
PdfCompositorImpl::GetDeserializationContext(
    const ContentToFrameMap& subframe_content_map) {
  DeserializationContext subframes;
  for (auto& content_info : subframe_content_map) {
    uint32_t content_id = content_info.first;
    uint64_t frame_guid = content_info.second;
    auto frame_iter = frame_info_map_.find(frame_guid);
    if (frame_iter == frame_info_map_.end())
      continue;

    if (frame_iter->second->composited)
      subframes[content_id] = frame_iter->second->content;
    else
      subframes[content_id] = CompositeSubframe(frame_iter->first);
  }
  return subframes;
}

void PdfCompositorImpl::FulfillRequest(
    uint64_t frame_guid,
    base::Optional<uint32_t> page_num,
    std::unique_ptr<base::SharedMemory> serialized_content,
    const ContentToFrameMap& subframe_content_map,
    CompositeToPdfCallback callback) {
  base::ReadOnlySharedMemoryRegion region;
  auto status =
      CompositeToPdf(frame_guid, page_num, std::move(serialized_content),
                     subframe_content_map, &region);
  std::move(callback).Run(status, std::move(region));
}

PdfCompositorImpl::FrameContentInfo::FrameContentInfo(
    std::unique_ptr<base::SharedMemory> content,
    const ContentToFrameMap& map)
    : serialized_content(std::move(content)), subframe_content_map(map) {}

PdfCompositorImpl::FrameContentInfo::FrameContentInfo() {}

PdfCompositorImpl::FrameContentInfo::~FrameContentInfo() {}

PdfCompositorImpl::FrameInfo::FrameInfo() {}

PdfCompositorImpl::FrameInfo::~FrameInfo() {}

PdfCompositorImpl::RequestInfo::RequestInfo(
    uint64_t frame_guid,
    base::Optional<uint32_t> page_num,
    std::unique_ptr<base::SharedMemory> content,
    const ContentToFrameMap& content_info,
    const base::flat_set<uint64_t>& pending_subframes,
    mojom::PdfCompositor::CompositePageToPdfCallback callback)
    : FrameContentInfo(std::move(content), content_info),
      frame_guid(frame_guid),
      page_number(page_num),
      pending_subframes(pending_subframes),
      callback(std::move(callback)) {}

PdfCompositorImpl::RequestInfo::~RequestInfo() {}

}  // namespace printing
