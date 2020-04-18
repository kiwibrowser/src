// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_URL_LOADER_FACTORY_BUNDLE_H_
#define CONTENT_COMMON_URL_LOADER_FACTORY_BUNDLE_H_

#include <map>
#include <string>

#include "base/macros.h"
#include "content/common/content_export.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"

class GURL;

namespace content {

// Holds the internal state of a URLLoaderFactoryBundle in a form that is safe
// to pass across sequences.
class CONTENT_EXPORT URLLoaderFactoryBundleInfo
    : public network::SharedURLLoaderFactoryInfo {
 public:
  URLLoaderFactoryBundleInfo();
  URLLoaderFactoryBundleInfo(
      network::mojom::URLLoaderFactoryPtrInfo default_factory_info,
      std::map<std::string, network::mojom::URLLoaderFactoryPtrInfo>
          factories_info);
  ~URLLoaderFactoryBundleInfo() override;

  network::mojom::URLLoaderFactoryPtrInfo& default_factory_info() {
    return default_factory_info_;
  }

  std::map<std::string, network::mojom::URLLoaderFactoryPtrInfo>&
  factories_info() {
    return factories_info_;
  }

 protected:
  // SharedURLLoaderFactoryInfo implementation.
  scoped_refptr<network::SharedURLLoaderFactory> CreateFactory() override;

  network::mojom::URLLoaderFactoryPtrInfo default_factory_info_;
  std::map<std::string, network::mojom::URLLoaderFactoryPtrInfo>
      factories_info_;

  DISALLOW_COPY_AND_ASSIGN(URLLoaderFactoryBundleInfo);
};

// Encapsulates a collection of URLLoaderFactoryPtrs which can be usd to acquire
// loaders for various types of resource requests.
class CONTENT_EXPORT URLLoaderFactoryBundle
    : public network::SharedURLLoaderFactory {
 public:
  URLLoaderFactoryBundle();

  explicit URLLoaderFactoryBundle(
      std::unique_ptr<URLLoaderFactoryBundleInfo> info);

  // Sets the default factory to use when no registered factories match a given
  // |url|.
  void SetDefaultFactory(network::mojom::URLLoaderFactoryPtr factory);

  // Registers a new factory to handle requests matching scheme |scheme|.
  void RegisterFactory(const base::StringPiece& scheme,
                       network::mojom::URLLoaderFactoryPtr factory);

  // Returns a factory which can be used to acquire a loader for |url|. If no
  // registered factory matches |url|'s scheme, the default factory is used. It
  // is undefined behavior to call this when no default factory is set.
  virtual network::mojom::URLLoaderFactory* GetFactoryForURL(const GURL& url);

  // SharedURLLoaderFactory implementation.
  void CreateLoaderAndStart(network::mojom::URLLoaderRequest loader,
                            int32_t routing_id,
                            int32_t request_id,
                            uint32_t options,
                            const network::ResourceRequest& request,
                            network::mojom::URLLoaderClientPtr client,
                            const net::MutableNetworkTrafficAnnotationTag&
                                traffic_annotation) override;
  void Clone(network::mojom::URLLoaderFactoryRequest request) override;
  std::unique_ptr<network::SharedURLLoaderFactoryInfo> Clone() override;

  // The |info| contains replacement factories for a subset of the existing
  // bundle.
  void Update(std::unique_ptr<URLLoaderFactoryBundleInfo> info);

 protected:
  ~URLLoaderFactoryBundle() override;

  network::mojom::URLLoaderFactoryPtr default_factory_;
  std::map<std::string, network::mojom::URLLoaderFactoryPtr> factories_;
};

}  // namespace content

#endif  // CONTENT_COMMON_URL_LOADER_FACTORY_BUNDLE_H_
