/*
    Copyright (C) 1998 Lars Knoll (knoll@mpi-hd.mpg.de)
    Copyright (C) 2001 Dirk Mueller <mueller@kde.org>
    Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
    Copyright (C) 2004, 2005, 2006, 2007 Apple Inc. All rights reserved.

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

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_LOADER_FETCH_RAW_RESOURCE_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_LOADER_FETCH_RAW_RESOURCE_H_

#include <memory>

#include "base/optional.h"
#include "third_party/blink/public/platform/web_data_consumer_handle.h"
#include "third_party/blink/renderer/platform/loader/fetch/buffering_data_pipe_writer.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_client.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_loader_options.h"
#include "third_party/blink/renderer/platform/platform_export.h"

namespace blink {
class WebDataConsumerHandle;
class FetchParameters;
class RawResourceClient;
class ResourceFetcher;
class SubstituteData;
class SourceKeyedCachedMetadataHandler;

class PLATFORM_EXPORT RawResource final : public Resource {
 public:
  static RawResource* FetchSynchronously(FetchParameters&, ResourceFetcher*);
  static RawResource* Fetch(FetchParameters&,
                            ResourceFetcher*,
                            RawResourceClient*);
  static RawResource* FetchMainResource(FetchParameters&,
                                        ResourceFetcher*,
                                        RawResourceClient*,
                                        const SubstituteData&);
  static RawResource* FetchImport(FetchParameters&,
                                  ResourceFetcher*,
                                  RawResourceClient*);
  static RawResource* FetchMedia(FetchParameters&,
                                 ResourceFetcher*,
                                 RawResourceClient*);
  static RawResource* FetchTextTrack(FetchParameters&,
                                     ResourceFetcher*,
                                     RawResourceClient*);
  static RawResource* FetchManifest(FetchParameters&,
                                    ResourceFetcher*,
                                    RawResourceClient*);

  // Exposed for testing
  static RawResource* CreateForTest(ResourceRequest request, Type type) {
    ResourceLoaderOptions options;
    return new RawResource(request, type, options);
  }
  static RawResource* CreateForTest(const KURL& url, Type type) {
    ResourceRequest request(url);
    return CreateForTest(request, type);
  }
  static RawResource* CreateForTest(const char* url, Type type) {
    return CreateForTest(KURL(url), type);
  }

  // Resource implementation
  bool CanReuse(
      const FetchParameters&,
      scoped_refptr<const SecurityOrigin> new_source_origin) const override;
  bool WillFollowRedirect(const ResourceRequest&,
                          const ResourceResponse&) override;

  void SetSerializedCachedMetadata(const char*, size_t) override;

  // Used for code caching of scripts with source code inline in the HTML.
  // Returns a cache handler which can store multiple cache metadata entries,
  // keyed by the source code of the script.
  SourceKeyedCachedMetadataHandler* CacheHandler();

  base::Optional<int64_t> DownloadedFileLength() const {
    return downloaded_file_length_;
  }
  scoped_refptr<BlobDataHandle> DownloadedBlob() const {
    return downloaded_blob_;
  }

 protected:
  CachedMetadataHandler* CreateCachedMetadataHandler(
      std::unique_ptr<CachedMetadataSender> send_callback) override;

 private:
  class RawResourceFactory : public NonTextResourceFactory {
   public:
    explicit RawResourceFactory(Resource::Type type)
        : NonTextResourceFactory(type) {}

    Resource* Create(const ResourceRequest& request,
                     const ResourceLoaderOptions& options) const override {
      return new RawResource(request, type_, options);
    }
  };

  RawResource(const ResourceRequest&, Type, const ResourceLoaderOptions&);

  // Resource implementation
  void DidAddClient(ResourceClient*) override;
  void AppendData(const char*, size_t) override;
  bool ShouldIgnoreHTTPStatusCodeErrors() const override {
    return !IsLinkPreload();
  }
  void WillNotFollowRedirect() override;
  void ResponseReceived(const ResourceResponse&,
                        std::unique_ptr<WebDataConsumerHandle>) override;
  void DidSendData(unsigned long long bytes_sent,
                   unsigned long long total_bytes_to_be_sent) override;
  void DidDownloadData(int) override;
  void DidDownloadToBlob(scoped_refptr<BlobDataHandle>) override;
  void ReportResourceTimingToClients(const ResourceTimingInfo&) override;
  bool MatchPreload(const FetchParameters&,
                    base::SingleThreadTaskRunner*) override;
  void NotifyFinished() override;

  base::Optional<int64_t> downloaded_file_length_;
  scoped_refptr<BlobDataHandle> downloaded_blob_;

  // Used for preload matching.
  std::unique_ptr<BufferingDataPipeWriter> data_pipe_writer_;
  std::unique_ptr<WebDataConsumerHandle> data_consumer_handle_;
};

// TODO(yhirano): Recover #if ENABLE_SECURITY_ASSERT when we stop adding
// RawResources to MemoryCache.
inline bool IsRawResource(const Resource& resource) {
  Resource::Type type = resource.GetType();
  return type == Resource::kMainResource || type == Resource::kRaw ||
         type == Resource::kTextTrack || type == Resource::kAudio ||
         type == Resource::kVideo || type == Resource::kManifest ||
         type == Resource::kImportResource;
}
inline RawResource* ToRawResource(Resource* resource) {
  SECURITY_DCHECK(!resource || IsRawResource(*resource));
  return static_cast<RawResource*>(resource);
}

class PLATFORM_EXPORT RawResourceClient : public ResourceClient {
 public:
  static bool IsExpectedType(ResourceClient* client) {
    return client->GetResourceClientType() == kRawResourceType;
  }
  ResourceClientType GetResourceClientType() const final {
    return kRawResourceType;
  }

  // The order of the callbacks is as follows:
  // [Case 1] A successful load:
  // 0+  redirectReceived() and/or dataSent()
  // 1   responseReceived()
  // 0-1 setSerializedCachedMetadata()
  // 0+  dataReceived() or dataDownloaded(), but never both
  // 1   notifyFinished() with errorOccurred() = false
  // [Case 2] When redirect is blocked:
  // 0+  redirectReceived() and/or dataSent()
  // 1   redirectBlocked()
  // 1   notifyFinished() with errorOccurred() = true
  // [Case 3] Other failures:
  //     notifyFinished() with errorOccurred() = true is called at any time
  //     (unless notifyFinished() is already called).
  // In all cases:
  //     No callbacks are made after notifyFinished() or
  //     removeClient() is called.
  virtual void DataSent(Resource*,
                        unsigned long long /* bytesSent */,
                        unsigned long long /* totalBytesToBeSent */) {}
  virtual void ResponseReceived(Resource*,
                                const ResourceResponse&,
                                std::unique_ptr<WebDataConsumerHandle>) {}
  virtual void SetSerializedCachedMetadata(Resource*, const char*, size_t) {}
  virtual bool RedirectReceived(Resource*,
                                const ResourceRequest&,
                                const ResourceResponse&) {
    return true;
  }
  virtual void RedirectBlocked() {}
  virtual void DataDownloaded(Resource*, int) {}
  virtual void DidReceiveResourceTiming(Resource*, const ResourceTimingInfo&) {}
  // Called for requests that had DownloadToBlob set to true. Can be called with
  // null if creating the blob failed for some reason (but the download itself
  // otherwise succeeded). Could also not be called at all if the downloaded
  // resource ended up being zero bytes.
  virtual void DidDownloadToBlob(Resource*, scoped_refptr<BlobDataHandle>) {}
};

// Checks the sequence of callbacks of RawResourceClient. This can be used only
// when a RawResourceClient is added as a client to at most one RawResource.
class PLATFORM_EXPORT RawResourceClientStateChecker final {
 public:
  RawResourceClientStateChecker();
  ~RawResourceClientStateChecker();

  // Call before addClient()/removeClient() is called.
  void WillAddClient();
  void WillRemoveClient();

  // Call RawResourceClientStateChecker::f() at the beginning of
  // RawResourceClient::f().
  void RedirectReceived();
  void RedirectBlocked();
  void DataSent();
  void ResponseReceived();
  void SetSerializedCachedMetadata();
  void DataReceived();
  void DataDownloaded();
  void DidDownloadToBlob();
  void NotifyFinished(Resource*);

 private:
  enum State {
    kNotAddedAsClient,
    kStarted,
    kRedirectBlocked,
    kResponseReceived,
    kSetSerializedCachedMetadata,
    kDataReceived,
    kDataDownloaded,
    kDidDownloadToBlob,
    kNotifyFinished
  };
  State state_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_LOADER_FETCH_RAW_RESOURCE_H_
