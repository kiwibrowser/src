// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_LOADER_TESTING_MOCK_RESOURCE_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_LOADER_TESTING_MOCK_RESOURCE_H_

#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource.h"
#include "third_party/blink/renderer/platform/wtf/ref_counted.h"

namespace blink {

class FetchParameters;
class ResourceFetcher;
struct ResourceLoaderOptions;

// Mocked cache handler class used by MockResource to test the caching behaviour
// of Resource.
class MockCacheHandler : public CachedMetadataHandler {
 public:
  MockCacheHandler(std::unique_ptr<CachedMetadataSender> send_callback);

  void Set(const char* data, size_t);
  void ClearCachedMetadata(CachedMetadataHandler::CacheType) override;
  void Send();

  String Encoding() const override { return "mock encoding"; }
  bool IsServedFromCacheStorage() const override { return false; }

 private:
  std::unique_ptr<CachedMetadataSender> send_callback_;
  base::Optional<Vector<char>> data_;
};

// Mocked Resource sub-class for testing. MockResource class can pretend a type
// of Resource sub-class in a simple way. You should not expect anything
// complicated to emulate actual sub-resources, but you may be able to use this
// class to verify classes that consume Resource sub-classes in a simple way.
class MockResource final : public Resource {
 public:
  static MockResource* Fetch(FetchParameters&,
                             ResourceFetcher*,
                             ResourceClient*);
  static MockResource* Create(const ResourceRequest&);
  static MockResource* Create(const KURL&);
  MockResource(const ResourceRequest&, const ResourceLoaderOptions&);

  CachedMetadataHandler* CreateCachedMetadataHandler(
      std::unique_ptr<CachedMetadataSender> send_callback) override;
  void SetSerializedCachedMetadata(const char*, size_t) override;

  MockCacheHandler* CacheHandler();

  void SendCachedMetadata(const char*, size_t);
};

}  // namespace blink

#endif
