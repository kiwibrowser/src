// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_ASSOCIATED_INTERFACE_REGISTRY_IMPL_H_
#define CONTENT_COMMON_ASSOCIATED_INTERFACE_REGISTRY_IMPL_H_

#include <map>
#include <string>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "mojo/public/cpp/bindings/scoped_interface_endpoint_handle.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_registry.h"

namespace content {

class AssociatedInterfaceRegistryImpl
    : public blink::AssociatedInterfaceRegistry {
 public:
  AssociatedInterfaceRegistryImpl();
  ~AssociatedInterfaceRegistryImpl() override;

  bool CanBindRequest(const std::string& interface_name) const;
  void BindRequest(const std::string& interface_name,
                   mojo::ScopedInterfaceEndpointHandle handle);

  // AssociatedInterfaceRegistry:
  void AddInterface(const std::string& name, const Binder& binder) override;
  void RemoveInterface(const std::string& name) override;

  base::WeakPtr<AssociatedInterfaceRegistryImpl> GetWeakPtr();

 private:
  std::map<std::string, Binder> interfaces_;
  base::WeakPtrFactory<AssociatedInterfaceRegistryImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(AssociatedInterfaceRegistryImpl);
};

}  // namespace content

#endif  // CONTENT_COMMON_ASSOCIATED_INTERFACE_REGISTRY_IMPL_H_
