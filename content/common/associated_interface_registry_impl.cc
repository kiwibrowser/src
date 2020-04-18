// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/associated_interface_registry_impl.h"

#include <utility>

#include "base/logging.h"

namespace content {

AssociatedInterfaceRegistryImpl::AssociatedInterfaceRegistryImpl()
    : weak_factory_(this) {}

AssociatedInterfaceRegistryImpl::~AssociatedInterfaceRegistryImpl() {}

bool AssociatedInterfaceRegistryImpl::CanBindRequest(
    const std::string& interface_name) const {
  return interfaces_.find(interface_name) != interfaces_.end();
}

void AssociatedInterfaceRegistryImpl::BindRequest(
    const std::string& interface_name,
    mojo::ScopedInterfaceEndpointHandle handle) {
  auto it = interfaces_.find(interface_name);
  if (it == interfaces_.end())
    return;
  it->second.Run(std::move(handle));
}

void AssociatedInterfaceRegistryImpl::AddInterface(const std::string& name,
                                                   const Binder& binder) {
  auto result = interfaces_.insert(std::make_pair(name, binder));
  DCHECK(result.second);
}

void AssociatedInterfaceRegistryImpl::RemoveInterface(const std::string& name) {
  auto it = interfaces_.find(name);
  DCHECK(it != interfaces_.end());
  interfaces_.erase(it);
}

base::WeakPtr<AssociatedInterfaceRegistryImpl>
AssociatedInterfaceRegistryImpl::GetWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

}  // namespace content
