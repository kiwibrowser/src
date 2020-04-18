/*
    Copyright (C) 1998 Lars Knoll (knoll@mpi-hd.mpg.de)
    Copyright (C) 2001 Dirk Mueller (mueller@kde.org)
    Copyright (C) 2002 Waldo Bastian (bastian@kde.org)
    Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
    Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

    This class provides all functionality needed for loading images, style
    sheets and html pages from the web. It has a memory cache for these objects.
*/

#include "third_party/blink/renderer/core/loader/resource/script_resource.h"

#include "services/network/public/mojom/request_context_frame_type.mojom-blink.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/loader/subresource_integrity_helper.h"
#include "third_party/blink/renderer/platform/instrumentation/tracing/web_memory_allocator_dump.h"
#include "third_party/blink/renderer/platform/instrumentation/tracing/web_process_memory_dump.h"
#include "third_party/blink/renderer/platform/loader/fetch/cached_metadata.h"
#include "third_party/blink/renderer/platform/loader/fetch/fetch_parameters.h"
#include "third_party/blink/renderer/platform/loader/fetch/integrity_metadata.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_client_walker.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_fetcher.h"
#include "third_party/blink/renderer/platform/loader/fetch/text_resource_decoder_options.h"
#include "third_party/blink/renderer/platform/loader/subresource_integrity.h"
#include "third_party/blink/renderer/platform/network/mime/mime_type_registry.h"
#include "third_party/blink/renderer/platform/shared_buffer.h"

namespace blink {

namespace {

// Returns true if the given request context is a script-like destination
// defined in the Fetch spec:
// https://fetch.spec.whatwg.org/#request-destination-script-like
bool IsRequestContextSupported(WebURLRequest::RequestContext request_context) {
  // TODO(nhiroki): Support |kRequestContextSharedWorker| for module loading for
  // shared workers (https://crbug.com/824646).
  // TODO(nhiroki): Support |kRequestContextServiceWorker| for module loading
  // for service workers (https://crbug.com/824647).
  // TODO(nhiroki): Support "audioworklet" and "paintworklet" destinations.
  switch (request_context) {
    case WebURLRequest::kRequestContextScript:
    case WebURLRequest::kRequestContextWorker:
      return true;
    default:
      break;
  }
  NOTREACHED() << "Incompatible request context type: " << request_context;
  return false;
}

}  // namespace

// SingleCachedMetadataHandlerImpl should be created when a response is
// received, and can be used independently from Resource. - It doesn't have any
// references to Resource. Necessary data are captured
//   from Resource when the handler is created.
// - It is not affected by Resource's revalidation on MemoryCache.
//   The validity of the handler is solely checked by |response_url_| and
//   |response_time_| (not by Resource) by the browser process, and the cached
//   metadata written to the handler is rejected if e.g. the disk cache entry
//   has been updated and the handler refers to an older response.
class ScriptResource::SingleCachedMetadataHandlerImpl final
    : public SingleCachedMetadataHandler {
 public:
  SingleCachedMetadataHandlerImpl(const WTF::TextEncoding&,
                                  std::unique_ptr<CachedMetadataSender>);
  ~SingleCachedMetadataHandlerImpl() override = default;
  void Trace(blink::Visitor*) override;
  void SetCachedMetadata(uint32_t, const char*, size_t, CacheType) override;
  void ClearCachedMetadata(CacheType) override;
  scoped_refptr<CachedMetadata> GetCachedMetadata(uint32_t) const override;

  // This returns the encoding at the time of ResponseReceived().
  // Therefore this does NOT reflect encoding detection from body contents,
  // but the final encoding after the encoding detection can be determined
  // uniquely from Encoding(), provided the body content is the same,
  // as we can assume the encoding detection will results in the same final
  // encoding.
  // TODO(hiroshige): Make this semantics cleaner.
  String Encoding() const override { return String(encoding_.GetName()); }

  bool IsServedFromCacheStorage() const override {
    return sender_->IsServedFromCacheStorage();
  }

  // Sets the serialized metadata retrieved from the platform's cache.
  void SetSerializedCachedMetadata(const char*, size_t);

 private:
  void SendToPlatform();

  scoped_refptr<CachedMetadata> cached_metadata_;
  std::unique_ptr<CachedMetadataSender> sender_;

  const WTF::TextEncoding encoding_;
};

ScriptResource::SingleCachedMetadataHandlerImpl::
    SingleCachedMetadataHandlerImpl(
        const WTF::TextEncoding& encoding,
        std::unique_ptr<CachedMetadataSender> sender)
    : sender_(std::move(sender)), encoding_(encoding) {}

void ScriptResource::SingleCachedMetadataHandlerImpl::Trace(
    blink::Visitor* visitor) {
  CachedMetadataHandler::Trace(visitor);
}

void ScriptResource::SingleCachedMetadataHandlerImpl::SetCachedMetadata(
    uint32_t data_type_id,
    const char* data,
    size_t size,
    CachedMetadataHandler::CacheType cache_type) {
  // Currently, only one type of cached metadata per resource is supported. If
  // the need arises for multiple types of metadata per resource this could be
  // enhanced to store types of metadata in a map.
  DCHECK(!cached_metadata_);
  cached_metadata_ = CachedMetadata::Create(data_type_id, data, size);
  if (cache_type == CachedMetadataHandler::kSendToPlatform)
    SendToPlatform();
}

void ScriptResource::SingleCachedMetadataHandlerImpl::ClearCachedMetadata(
    CachedMetadataHandler::CacheType cache_type) {
  cached_metadata_ = nullptr;
  if (cache_type == CachedMetadataHandler::kSendToPlatform)
    SendToPlatform();
}

scoped_refptr<CachedMetadata>
ScriptResource::SingleCachedMetadataHandlerImpl::GetCachedMetadata(
    uint32_t data_type_id) const {
  if (!cached_metadata_ || cached_metadata_->DataTypeID() != data_type_id)
    return nullptr;
  return cached_metadata_;
}

void ScriptResource::SingleCachedMetadataHandlerImpl::
    SetSerializedCachedMetadata(const char* data, size_t size) {
  // We only expect to receive cached metadata from the platform once. If this
  // triggers, it indicates an efficiency problem which is most likely
  // unexpected in code designed to improve performance.
  DCHECK(!cached_metadata_);
  cached_metadata_ = CachedMetadata::CreateFromSerializedData(data, size);
}

void ScriptResource::SingleCachedMetadataHandlerImpl::SendToPlatform() {
  if (cached_metadata_) {
    const Vector<char>& serialized_data = cached_metadata_->SerializedData();
    sender_->Send(serialized_data.data(), serialized_data.size());
  } else {
    sender_->Send(nullptr, 0);
  }
}

ScriptResource* ScriptResource::Fetch(FetchParameters& params,
                                      ResourceFetcher* fetcher,
                                      ResourceClient* client) {
  DCHECK_EQ(params.GetResourceRequest().GetFrameType(),
            network::mojom::RequestContextFrameType::kNone);
  DCHECK(IsRequestContextSupported(
      params.GetResourceRequest().GetRequestContext()));
  return ToScriptResource(
      fetcher->RequestResource(params, ScriptResourceFactory(), client));
}

ScriptResource::ScriptResource(
    const ResourceRequest& resource_request,
    const ResourceLoaderOptions& options,
    const TextResourceDecoderOptions& decoder_options)
    : TextResource(resource_request, kScript, options, decoder_options) {}

ScriptResource::~ScriptResource() = default;

void ScriptResource::OnMemoryDump(WebMemoryDumpLevelOfDetail level_of_detail,
                                  WebProcessMemoryDump* memory_dump) const {
  Resource::OnMemoryDump(level_of_detail, memory_dump);
  const String name = GetMemoryDumpName() + "/decoded_script";
  auto* dump = memory_dump->CreateMemoryAllocatorDump(name);
  dump->AddScalar("size", "bytes", source_text_.CharactersSizeInBytes());
  memory_dump->AddSuballocation(
      dump->Guid(), String(WTF::Partitions::kAllocatedObjectPoolName));
}

const String& ScriptResource::SourceText() {
  DCHECK(IsLoaded());

  if (source_text_.IsNull() && Data()) {
    String source_text = DecodedText();
    ClearData();
    SetDecodedSize(source_text.CharactersSizeInBytes());
    source_text_ = AtomicString(source_text);
  }

  return source_text_;
}

SingleCachedMetadataHandler* ScriptResource::CacheHandler() {
  return static_cast<SingleCachedMetadataHandler*>(Resource::CacheHandler());
}

CachedMetadataHandler* ScriptResource::CreateCachedMetadataHandler(
    std::unique_ptr<CachedMetadataSender> send_callback) {
  return new SingleCachedMetadataHandlerImpl(Encoding(),
                                             std::move(send_callback));
}

void ScriptResource::SetSerializedCachedMetadata(const char* data,
                                                 size_t size) {
  Resource::SetSerializedCachedMetadata(data, size);
  SingleCachedMetadataHandlerImpl* cache_handler =
      static_cast<SingleCachedMetadataHandlerImpl*>(Resource::CacheHandler());
  if (cache_handler) {
    cache_handler->SetSerializedCachedMetadata(data, size);
  }
}

void ScriptResource::DestroyDecodedDataForFailedRevalidation() {
  source_text_ = AtomicString();
  SetDecodedSize(0);
}

AccessControlStatus ScriptResource::CalculateAccessControlStatus(
    const SecurityOrigin* security_origin) const {
  if (GetResponse().WasFetchedViaServiceWorker()) {
    if (GetCORSStatus() == CORSStatus::kServiceWorkerOpaque)
      return kOpaqueResource;
    return kSharableCrossOrigin;
  }

  if (security_origin && PassesAccessControlCheck(*security_origin))
    return kSharableCrossOrigin;

  return kNotSharableCrossOrigin;
}

bool ScriptResource::CanUseCacheValidator() const {
  // Do not revalidate until ClassicPendingScript is removed, i.e. the script
  // content is retrieved in ScriptLoader::ExecuteScriptBlock().
  // crbug.com/692856
  if (HasClientsOrObservers())
    return false;

  return Resource::CanUseCacheValidator();
}

}  // namespace blink
