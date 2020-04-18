/*
    Copyright (C) 1998 Lars Knoll (knoll@mpi-hd.mpg.de)
    Copyright (C) 2001 Dirk Mueller <mueller@kde.org>
    Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
    Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc. All
    rights reserved.

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
*/

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_LOADER_FETCH_RESOURCE_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_LOADER_FETCH_RESOURCE_H_

#include <memory>
#include "base/auto_reset.h"
#include "base/optional.h"
#include "base/single_thread_task_runner.h"
#include "third_party/blink/public/platform/web_data_consumer_handle.h"
#include "third_party/blink/public/platform/web_scoped_virtual_time_pauser.h"
#include "third_party/blink/renderer/platform/instrumentation/tracing/web_process_memory_dump.h"
#include "third_party/blink/renderer/platform/loader/cors/cors_status.h"
#include "third_party/blink/renderer/platform/loader/fetch/cached_metadata_handler.h"
#include "third_party/blink/renderer/platform/loader/fetch/integrity_metadata.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_error.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_load_priority.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_loader_options.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_priority.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_request.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_response.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_status.h"
#include "third_party/blink/renderer/platform/loader/fetch/text_resource_decoder_options.h"
#include "third_party/blink/renderer/platform/loader/subresource_integrity.h"
#include "third_party/blink/renderer/platform/memory_coordinator.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/shared_buffer.h"
#include "third_party/blink/renderer/platform/timer.h"
#include "third_party/blink/renderer/platform/web_task_runner.h"
#include "third_party/blink/renderer/platform/wtf/allocator.h"
#include "third_party/blink/renderer/platform/wtf/hash_counted_set.h"
#include "third_party/blink/renderer/platform/wtf/hash_set.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string.h"
#include "third_party/blink/renderer/platform/wtf/text/text_encoding.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"
#include "third_party/blink/renderer/platform/wtf/time.h"

namespace blink {

class FetchParameters;
class ResourceClient;
class ResourceFetcher;
class ResourceFinishObserver;
class ResourceTimingInfo;
class ResourceLoader;
class SecurityOrigin;

// A callback for sending the serialized data of cached metadata back to the
// platform.
class CachedMetadataSender {
 public:
  virtual ~CachedMetadataSender() = default;
  virtual void Send(const char*, size_t) = 0;

  // IsServedFromCacheStorage is used to alter caching strategy to be more
  // aggressive. See ScriptController.cpp CacheOptions() for an example.
  virtual bool IsServedFromCacheStorage() = 0;
};

// A resource that is held in the cache. Classes who want to use this object
// should derive from ResourceClient, to get the function calls in case the
// requested data has arrived. This class also does the actual communication
// with the loader to obtain the resource from the network.
class PLATFORM_EXPORT Resource : public GarbageCollectedFinalized<Resource>,
                                 public MemoryCoordinatorClient {
  USING_GARBAGE_COLLECTED_MIXIN(Resource);
  WTF_MAKE_NONCOPYABLE(Resource);

 public:
  // |Type| enum values are used in UMAs, so do not change the values of
  // existing |Type|. When adding a new |Type|, append it at the end and update
  // |kLastResourceType|.
  enum Type : uint8_t {
    kMainResource,
    kImage,
    kCSSStyleSheet,
    kScript,
    kFont,
    kRaw,
    kSVGDocument,
    kXSLStyleSheet,
    kLinkPrefetch,
    kTextTrack,
    kImportResource,
    kAudio,
    kVideo,
    kManifest,
    kMock  // Only for testing
  };
  static const int kLastResourceType = kMock + 1;

  // Used by reloadIfLoFiOrPlaceholderImage().
  enum ReloadLoFiOrPlaceholderPolicy {
    kReloadIfNeeded,
    kReloadAlways,
  };

  ~Resource() override;

  void Trace(blink::Visitor*) override;

  virtual WTF::TextEncoding Encoding() const { return WTF::TextEncoding(); }
  virtual void AppendData(const char*, size_t);
  virtual void FinishAsError(const ResourceError&,
                             base::SingleThreadTaskRunner*);

  void SetLinkPreload(bool is_link_preload) { link_preload_ = is_link_preload; }
  bool IsLinkPreload() const { return link_preload_; }

  const ResourceError& GetResourceError() const {
    DCHECK(error_);
    return *error_;
  }

  void SetIdentifier(unsigned long identifier) { identifier_ = identifier; }
  unsigned long Identifier() const { return identifier_; }

  virtual bool ShouldIgnoreHTTPStatusCodeErrors() const { return false; }

  const ResourceRequest& GetResourceRequest() const {
    return resource_request_;
  }
  const ResourceRequest& LastResourceRequest() const;

  virtual void SetRevalidatingRequest(const ResourceRequest&);

  // This url can have a fragment, but it can match resources that differ by the
  // fragment only.
  const KURL& Url() const { return GetResourceRequest().Url(); }
  Type GetType() const { return static_cast<Type>(type_); }
  const ResourceLoaderOptions& Options() const { return options_; }
  ResourceLoaderOptions& MutableOptions() { return options_; }

  void DidChangePriority(ResourceLoadPriority, int intra_priority_value);
  virtual ResourcePriority PriorityFromObservers() {
    return ResourcePriority();
  }

  // If this Resource is already finished when AddClient is called, the
  // ResourceClient will be notified asynchronously by a task scheduled
  // on the given base::SingleThreadTaskRunner. Otherwise, the given
  // base::SingleThreadTaskRunner is unused.
  void AddClient(ResourceClient*, base::SingleThreadTaskRunner*);
  void RemoveClient(ResourceClient*);

  // If this Resource is already finished when AddFinishObserver is called, the
  // ResourceFinishObserver will be notified asynchronously by a task scheduled
  // on the given base::SingleThreadTaskRunner. Otherwise, the given
  // base::SingleThreadTaskRunner is unused.
  void AddFinishObserver(ResourceFinishObserver*,
                         base::SingleThreadTaskRunner*);
  void RemoveFinishObserver(ResourceFinishObserver*);

  bool IsUnusedPreload() const { return is_unused_preload_; }

  ResourceStatus GetStatus() const { return status_; }
  void SetStatus(ResourceStatus status) { status_ = status; }

  size_t size() const { return EncodedSize() + DecodedSize() + OverheadSize(); }

  // Returns the size of content (response body) before decoding. Adding a new
  // usage of this function is not recommended (See the TODO below).
  //
  // TODO(hiroshige): Now EncodedSize/DecodedSize states are inconsistent and
  // need to be refactored (crbug/643135).
  size_t EncodedSize() const { return encoded_size_; }

  // Returns the current memory usage for the encoded data. Adding a new usage
  // of this function is not recommended as the same reason as |EncodedSize()|.
  //
  // |EncodedSize()| and |EncodedSizeMemoryUsageForTesting()| can return
  // different values, e.g., when ImageResource purges encoded image data after
  // finishing loading.
  size_t EncodedSizeMemoryUsageForTesting() const {
    return encoded_size_memory_usage_;
  }

  size_t DecodedSize() const { return decoded_size_; }
  size_t OverheadSize() const { return overhead_size_; }

  bool IsLoaded() const { return status_ > ResourceStatus::kPending; }

  bool IsLoading() const { return status_ == ResourceStatus::kPending; }
  bool StillNeedsLoad() const { return status_ < ResourceStatus::kPending; }

  void SetLoader(ResourceLoader*);
  ResourceLoader* Loader() const { return loader_.Get(); }

  bool ShouldBlockLoadEvent() const;
  bool IsLoadEventBlockingResourceType() const;

  // Computes the status of an object after loading. Updates the expire date on
  // the cache entry file
  virtual void Finish(TimeTicks finish_time, base::SingleThreadTaskRunner*);
  void FinishForTest() { Finish(TimeTicks(), nullptr); }

  bool PassesAccessControlCheck(const SecurityOrigin&) const;

  virtual scoped_refptr<const SharedBuffer> ResourceBuffer() const {
    return data_;
  }
  void SetResourceBuffer(scoped_refptr<SharedBuffer>);

  virtual bool WillFollowRedirect(const ResourceRequest&,
                                  const ResourceResponse&);

  // Called when a redirect response was received but a decision has already
  // been made to not follow it.
  virtual void WillNotFollowRedirect() {}

  virtual void ResponseReceived(const ResourceResponse&,
                                std::unique_ptr<WebDataConsumerHandle>);
  void SetResponse(const ResourceResponse&);
  const ResourceResponse& GetResponse() const { return response_; }

  virtual void ReportResourceTimingToClients(const ResourceTimingInfo&) {}

  // Sets the serialized metadata retrieved from the platform's cache.
  // Subclasses of Resource that support cached metadata should override this
  // method with one that fills the current CachedMetadataHandler.
  virtual void SetSerializedCachedMetadata(const char*, size_t);

  AtomicString HttpContentType() const;

  bool WasCanceled() const { return error_ && error_->IsCancellation(); }
  bool ErrorOccurred() const {
    return status_ == ResourceStatus::kLoadError ||
           status_ == ResourceStatus::kDecodeError;
  }
  bool LoadFailedOrCanceled() const { return !!error_; }

  DataBufferingPolicy GetDataBufferingPolicy() const {
    return options_.data_buffering_policy;
  }
  void SetDataBufferingPolicy(DataBufferingPolicy);

  void MarkAsPreload();
  // Returns true if |this| resource is matched with the given parameters.
  virtual bool MatchPreload(const FetchParameters&,
                            base::SingleThreadTaskRunner*);

  bool CanReuseRedirectChain() const;
  bool MustRevalidateDueToCacheHeaders() const;
  virtual bool CanUseCacheValidator() const;
  bool IsCacheValidator() const { return is_revalidating_; }
  bool HasCacheControlNoStoreHeader() const;
  bool MustReloadDueToVaryHeader(const ResourceRequest& new_request) const;

  const IntegrityMetadataSet& IntegrityMetadata() const {
    return options_.integrity_metadata;
  }
  ResourceIntegrityDisposition IntegrityDisposition() const {
    return integrity_disposition_;
  }
  const SubresourceIntegrity::ReportInfo& IntegrityReportInfo() const {
    return integrity_report_info_;
  }
  bool MustRefetchDueToIntegrityMetadata(const FetchParameters&) const;

  bool IsAlive() const { return is_alive_; }

  CORSStatus GetCORSStatus() const { return cors_status_; }

  bool IsSameOriginOrCORSSuccessful() const {
    return cors_status_ == CORSStatus::kSameOrigin ||
           cors_status_ == CORSStatus::kSuccessful ||
           cors_status_ == CORSStatus::kServiceWorkerSuccessful;
  }

  void SetCacheIdentifier(const String& cache_identifier) {
    cache_identifier_ = cache_identifier;
  }
  String CacheIdentifier() const { return cache_identifier_; }

  void SetSourceOrigin(scoped_refptr<const SecurityOrigin> source_origin) {
    source_origin_ = source_origin;
  }

  virtual void DidSendData(unsigned long long /* bytesSent */,
                           unsigned long long /* totalBytesToBeSent */) {}
  virtual void DidDownloadData(int) {}
  virtual void DidDownloadToBlob(scoped_refptr<BlobDataHandle>) {}

  TimeTicks LoadFinishTime() const { return load_finish_time_; }

  void SetEncodedDataLength(int64_t value) {
    response_.SetEncodedDataLength(value);
  }
  void SetEncodedBodyLength(int value) {
    response_.SetEncodedBodyLength(value);
  }
  void SetDecodedBodyLength(int value) {
    response_.SetDecodedBodyLength(value);
  }

  virtual bool CanReuse(
      const FetchParameters&,
      scoped_refptr<const SecurityOrigin> new_source_origin) const;

  // If cache-aware loading is activated, this callback is called when the first
  // disk-cache-only request failed due to cache miss. After this callback,
  // cache-aware loading is deactivated and a reload with original request will
  // be triggered right away in ResourceLoader.
  virtual void WillReloadAfterDiskCacheMiss() {}

  // TODO(shaochuan): This is for saving back the actual ResourceRequest sent
  // in ResourceFetcher::StartLoad() for retry in cache-aware loading, remove
  // once ResourceRequest is not modified in StartLoad(). crbug.com/632580
  void SetResourceRequest(const ResourceRequest& resource_request) {
    resource_request_ = resource_request;
  }

  // Used by the MemoryCache to reduce the memory consumption of the entry.
  void Prune();

  virtual void OnMemoryDump(WebMemoryDumpLevelOfDetail,
                            WebProcessMemoryDump*) const;

  // If this Resource is ImageResource and has the Lo-Fi response headers or is
  // a placeholder, reload the full original image with the Lo-Fi state set to
  // off and optionally bypassing the cache.
  virtual void ReloadIfLoFiOrPlaceholderImage(ResourceFetcher*,
                                              ReloadLoFiOrPlaceholderPolicy) {}

  // Used to notify ImageResourceContent of the start of actual loading.
  // JavaScript calls or client/observer notifications are disallowed inside
  // NotifyStartLoad().
  virtual void NotifyStartLoad() {}

  static const char* ResourceTypeToString(
      Type,
      const AtomicString& fetch_initiator_name);

  class ProhibitAddRemoveClientInScope : public base::AutoReset<bool> {
   public:
    ProhibitAddRemoveClientInScope(Resource* resource)
        : AutoReset(&resource->is_add_remove_client_prohibited_, true) {}
  };

  class RevalidationStartForbiddenScope : public base::AutoReset<bool> {
   public:
    RevalidationStartForbiddenScope(Resource* resource)
        : AutoReset(&resource->is_revalidation_start_forbidden_, true) {}
  };

  WebScopedVirtualTimePauser& VirtualTimePauser() {
    return virtual_time_pauser_;
  }

 protected:
  Resource(const ResourceRequest&, Type, const ResourceLoaderOptions&);

  virtual void NotifyFinished();

  void MarkClientFinished(ResourceClient*);

  virtual bool HasClientsOrObservers() const {
    return !clients_.IsEmpty() || !clients_awaiting_callback_.IsEmpty() ||
           !finished_clients_.IsEmpty() || !finish_observers_.IsEmpty();
  }
  virtual void DestroyDecodedDataForFailedRevalidation() {}

  void SetEncodedSize(size_t);
  void SetDecodedSize(size_t);

  void FinishPendingClients();

  virtual void DidAddClient(ResourceClient*);
  void WillAddClientOrObserver();

  void DidRemoveClientOrObserver();
  virtual void AllClientsAndObserversRemoved();

  bool HasClient(ResourceClient* client) const {
    return clients_.Contains(client) ||
           clients_awaiting_callback_.Contains(client) ||
           finished_clients_.Contains(client);
  }

  struct RedirectPair {
    DISALLOW_NEW_EXCEPT_PLACEMENT_NEW();

   public:
    explicit RedirectPair(const ResourceRequest& request,
                          const ResourceResponse& redirect_response)
        : request_(request), redirect_response_(redirect_response) {}

    ResourceRequest request_;
    ResourceResponse redirect_response_;
  };
  const Vector<RedirectPair>& RedirectChain() const { return redirect_chain_; }

  virtual void DestroyDecodedDataIfPossible() {}

  // Returns the memory dump name used for tracing. See Resource::OnMemoryDump.
  String GetMemoryDumpName() const;

  const HeapHashCountedSet<WeakMember<ResourceClient>>& Clients() const {
    return clients_;
  }

  void SetCachePolicyBypassingCache();
  void SetPreviewsState(WebURLRequest::PreviewsState);
  void ClearRangeRequestHeader();

  SharedBuffer* Data() const { return data_.get(); }
  void ClearData();

  virtual void SetEncoding(const String&) {}

  // Create a handler for the cached metadata of this resource. Subclasses of
  // Resource that support cached metadata should override this method with one
  // that creates an appropriate CachedMetadataHandler implementation, and
  // override SetSerializedCachedMetadata with an implementation that fills the
  // cache handler.
  virtual CachedMetadataHandler* CreateCachedMetadataHandler(
      std::unique_ptr<CachedMetadataSender> send_callback) {
    return nullptr;
  }

  CachedMetadataHandler* CacheHandler() { return cache_handler_.Get(); }

 private:
  // To allow access to SetCORSStatus
  friend class ResourceLoader;
  friend class SubresourceIntegrityTest;

  void RevalidationSucceeded(const ResourceResponse&);
  void RevalidationFailed();

  size_t CalculateOverheadSize() const;

  String ReasonNotDeletable() const;

  void SetCORSStatus(const CORSStatus cors_status) {
    cors_status_ = cors_status;
  }

  // MemoryCoordinatorClient overrides:
  void OnPurgeMemory() override;

  void CheckResourceIntegrity();
  void TriggerNotificationForFinishObservers(base::SingleThreadTaskRunner*);

  // Helper for creating the send callback function for the cached metadata
  // handler.
  std::unique_ptr<CachedMetadataSender> CreateCachedMetadataSender() const;

  Type type_;
  ResourceStatus status_;

  // A SecurityOrigin representing the origin from which the loading of the
  // Resource was initiated. This is calculated and set on Resource creation.
  //
  // Unlike |security_origin| on |options_|, which:
  // - holds a SecurityOrigin to override the FetchContext's SecurityOrigin
  //   (in case of e.g. that the script initiated the loading is in an isolated
  //   world)
  //
  // Used for isolating resources for different origins in the MemoryCache.
  //
  // Note: A Resource returned from the memory cache has an origin for the first
  // initiator that fetched the Resource. It may be different from the origin
  // that you need for any runtime security check in Blink.
  scoped_refptr<const SecurityOrigin> source_origin_;

  CORSStatus cors_status_;

  Member<CachedMetadataHandler> cache_handler_;

  base::Optional<ResourceError> error_;

  TimeTicks load_finish_time_;

  unsigned long identifier_;

  size_t encoded_size_;
  size_t encoded_size_memory_usage_;
  size_t decoded_size_;

  // Resource::CalculateOverheadSize() is affected by changes in
  // |m_resourceRequest.url()|, but |m_overheadSize| is not updated after
  // initial |m_resourceRequest| is given, to reduce MemoryCache manipulation
  // and thus potential bugs. crbug.com/594644
  const size_t overhead_size_;

  String cache_identifier_;

  bool link_preload_;
  bool is_revalidating_;
  bool is_alive_;
  bool is_add_remove_client_prohibited_;
  bool is_revalidation_start_forbidden_ = false;
  bool is_unused_preload_ = false;

  ResourceIntegrityDisposition integrity_disposition_;
  SubresourceIntegrity::ReportInfo integrity_report_info_;

  // Ordered list of all redirects followed while fetching this resource.
  Vector<RedirectPair> redirect_chain_;

  HeapHashCountedSet<WeakMember<ResourceClient>> clients_;
  HeapHashCountedSet<WeakMember<ResourceClient>> clients_awaiting_callback_;
  HeapHashCountedSet<WeakMember<ResourceClient>> finished_clients_;
  HeapHashSet<WeakMember<ResourceFinishObserver>> finish_observers_;

  ResourceLoaderOptions options_;

  double response_timestamp_;

  TaskHandle async_finish_pending_clients_task_;

  ResourceRequest resource_request_;
  Member<ResourceLoader> loader_;
  ResourceResponse response_;

  scoped_refptr<SharedBuffer> data_;

  WebScopedVirtualTimePauser virtual_time_pauser_;
};

class ResourceFactory {
  STACK_ALLOCATED();

 public:
  virtual Resource* Create(const ResourceRequest&,
                           const ResourceLoaderOptions&,
                           const TextResourceDecoderOptions&) const = 0;

  Resource::Type GetType() const { return type_; }
  TextResourceDecoderOptions::ContentType ContentType() const {
    return content_type_;
  }

 protected:
  explicit ResourceFactory(Resource::Type type,
                           TextResourceDecoderOptions::ContentType content_type)
      : type_(type), content_type_(content_type) {}

  Resource::Type type_;
  TextResourceDecoderOptions::ContentType content_type_;
};

class NonTextResourceFactory : public ResourceFactory {
 protected:
  explicit NonTextResourceFactory(Resource::Type type)
      : ResourceFactory(type, TextResourceDecoderOptions::kPlainTextContent) {}

  virtual Resource* Create(const ResourceRequest&,
                           const ResourceLoaderOptions&) const = 0;

  Resource* Create(const ResourceRequest& request,
                   const ResourceLoaderOptions& options,
                   const TextResourceDecoderOptions&) const final {
    return Create(request, options);
  }
};

#define DEFINE_RESOURCE_TYPE_CASTS(typeName)                      \
  DEFINE_TYPE_CASTS(typeName##Resource, Resource, resource,       \
                    resource->GetType() == Resource::k##typeName, \
                    resource.GetType() == Resource::k##typeName);

}  // namespace blink

#endif
