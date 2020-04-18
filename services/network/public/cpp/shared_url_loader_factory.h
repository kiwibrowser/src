// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NETWORK_PUBLIC_CPP_SHARED_URL_LOADER_FACTORY_H_
#define SERVICES_NETWORK_PUBLIC_CPP_SHARED_URL_LOADER_FACTORY_H_

#include <memory>

#include "base/component_export.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"

namespace network {

class SharedURLLoaderFactoryInfo;

// This is a ref-counted C++ interface to replace raw mojom::URLLoaderFactory
// pointers and various factory getters.
// A SharedURLLoaderFactory instance is supposed to be used on a single
// sequence. To use it on a different sequence, use Clone() and pass the
// resulting SharedURLLoaderFactoryInfo instance to the target sequence. On the
// target sequence, call SharedURLLoaderFactory::Create() to convert the info
// instance to a new SharedURLLoaderFactory.
class COMPONENT_EXPORT(NETWORK_CPP) SharedURLLoaderFactory
    : public base::RefCounted<SharedURLLoaderFactory>,
      public network::mojom::URLLoaderFactory {
 public:
  static scoped_refptr<SharedURLLoaderFactory> Create(
      std::unique_ptr<SharedURLLoaderFactoryInfo> info);

  // From network::mojom::URLLoaderFactory:
  void Clone(network::mojom::URLLoaderFactoryRequest request) override = 0;

  virtual std::unique_ptr<SharedURLLoaderFactoryInfo> Clone() = 0;

 protected:
  friend class base::RefCounted<SharedURLLoaderFactory>;
  ~SharedURLLoaderFactory() override;
};

// SharedURLLoaderFactoryInfo contains necessary information to construct a
// SharedURLLoaderFactory. It is not sequence safe but can be passed across
// sequences. Please see the comments of SharedURLLoaderFactory for how this
// class is used.
class COMPONENT_EXPORT(NETWORK_CPP) SharedURLLoaderFactoryInfo {
 public:
  SharedURLLoaderFactoryInfo();
  virtual ~SharedURLLoaderFactoryInfo();

 protected:
  friend class SharedURLLoaderFactory;

  // Creates a SharedURLLoaderFactory. It should only be called by
  // SharedURLLoaderFactory::Create(), which makes sense that CreateFactory() is
  // never called multiple times for each SharedURLLoaderFactoryInfo instance.
  virtual scoped_refptr<SharedURLLoaderFactory> CreateFactory() = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(SharedURLLoaderFactoryInfo);
};

}  // namespace network

#endif  // SERVICES_NETWORK_PUBLIC_CPP_SHARED_URL_LOADER_FACTORY_H_
