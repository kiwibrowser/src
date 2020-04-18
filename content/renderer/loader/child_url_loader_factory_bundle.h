// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_LOADER_CHILD_URL_LOADER_FACTORY_BUNDLE_H_
#define CONTENT_RENDERER_LOADER_CHILD_URL_LOADER_FACTORY_BUNDLE_H_

#include "base/callback.h"
#include "content/common/content_export.h"
#include "content/common/possibly_associated_interface_ptr.h"
#include "content/common/url_loader_factory_bundle.h"
#include "content/public/common/transferrable_url_loader.mojom.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"

namespace content {

// Holds the internal state of a ChildURLLoaderFactoryBundle in a form that is
// safe to pass across sequences.
class CONTENT_EXPORT ChildURLLoaderFactoryBundleInfo
    : public URLLoaderFactoryBundleInfo {
 public:
  using PossiblyAssociatedURLLoaderFactoryPtrInfo =
      PossiblyAssociatedInterfacePtrInfo<network::mojom::URLLoaderFactory>;

  ChildURLLoaderFactoryBundleInfo();
  explicit ChildURLLoaderFactoryBundleInfo(
      std::unique_ptr<URLLoaderFactoryBundleInfo> base_info);
  ChildURLLoaderFactoryBundleInfo(
      network::mojom::URLLoaderFactoryPtrInfo default_factory_info,
      std::map<std::string, network::mojom::URLLoaderFactoryPtrInfo>
          factories_info,
      PossiblyAssociatedURLLoaderFactoryPtrInfo direct_network_factory_info);
  ~ChildURLLoaderFactoryBundleInfo() override;

  PossiblyAssociatedURLLoaderFactoryPtrInfo& direct_network_factory_info() {
    return direct_network_factory_info_;
  }

 protected:
  // URLLoaderFactoryBundleInfo overrides.
  scoped_refptr<network::SharedURLLoaderFactory> CreateFactory() override;

  PossiblyAssociatedURLLoaderFactoryPtrInfo direct_network_factory_info_;

  DISALLOW_COPY_AND_ASSIGN(ChildURLLoaderFactoryBundleInfo);
};

// This class extends URLLoaderFactoryBundle to support a direct network loader
// factory, which bypasses custom overrides such as appcache or service worker.
// Besides, it also supports using callbacks to lazily initialize the blob and
// the direct network loader factories.
class CONTENT_EXPORT ChildURLLoaderFactoryBundle
    : public URLLoaderFactoryBundle {
 public:
  using PossiblyAssociatedURLLoaderFactoryPtr =
      PossiblyAssociatedInterfacePtr<network::mojom::URLLoaderFactory>;

  using FactoryGetterCallback =
      base::OnceCallback<network::mojom::URLLoaderFactoryPtr()>;
  using PossiblyAssociatedFactoryGetterCallback =
      base::OnceCallback<PossiblyAssociatedURLLoaderFactoryPtr()>;

  ChildURLLoaderFactoryBundle();

  explicit ChildURLLoaderFactoryBundle(
      std::unique_ptr<ChildURLLoaderFactoryBundleInfo> info);

  ChildURLLoaderFactoryBundle(
      PossiblyAssociatedFactoryGetterCallback direct_network_factory_getter,
      FactoryGetterCallback default_blob_factory_getter);

  // URLLoaderFactoryBundle overrides.
  network::mojom::URLLoaderFactory* GetFactoryForURL(const GURL& url) override;

  void CreateLoaderAndStart(network::mojom::URLLoaderRequest loader,
                            int32_t routing_id,
                            int32_t request_id,
                            uint32_t options,
                            const network::ResourceRequest& request,
                            network::mojom::URLLoaderClientPtr client,
                            const net::MutableNetworkTrafficAnnotationTag&
                                traffic_annotation) override;

  std::unique_ptr<network::SharedURLLoaderFactoryInfo> Clone() override;

  // Returns an info that omits this bundle's default factory, if any. This is
  // useful to make a clone that bypasses AppCache, for example.
  std::unique_ptr<network::SharedURLLoaderFactoryInfo>
  CloneWithoutDefaultFactory();

  std::unique_ptr<ChildURLLoaderFactoryBundleInfo> PassInterface();

  void Update(std::unique_ptr<ChildURLLoaderFactoryBundleInfo> info,
              base::Optional<std::vector<mojom::TransferrableURLLoaderPtr>>
                  subresource_overrides);

  virtual bool IsHostChildURLLoaderFactoryBundle() const;

 protected:
  ~ChildURLLoaderFactoryBundle() override;

 private:
  void InitDefaultBlobFactoryIfNecessary();
  void InitDirectNetworkFactoryIfNecessary();
  std::unique_ptr<network::SharedURLLoaderFactoryInfo> CloneInternal(
      bool include_default);

  PossiblyAssociatedFactoryGetterCallback direct_network_factory_getter_;
  PossiblyAssociatedURLLoaderFactoryPtr direct_network_factory_;

  std::map<GURL, mojom::TransferrableURLLoaderPtr> subresource_overrides_;

  FactoryGetterCallback default_blob_factory_getter_;
};

}  // namespace content

#endif  // CONTENT_RENDERER_LOADER_CHILD_URL_LOADER_FACTORY_BUNDLE_H_
