// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_COMMON_ASSOCIATED_INTERFACES_ASSOCIATED_INTERFACE_REGISTRY_H_
#define THIRD_PARTY_BLINK_PUBLIC_COMMON_ASSOCIATED_INTERFACES_ASSOCIATED_INTERFACE_REGISTRY_H_

#include <string>

#include "base/bind.h"
#include "base/callback.h"
#include "mojo/public/cpp/bindings/associated_interface_request.h"
#include "mojo/public/cpp/bindings/scoped_interface_endpoint_handle.h"

namespace blink {

// An AssociatedInterfaceRegistry is a collection of associated interface-
// binding callbacks mapped by interface name.
//
// This is used to register binding callbacks for interfaces which must be
// associated with some IPC::ChannelProxy, meaning that messages on the
// interface retain FIFO with respect to legacy Chrome IPC messages sent or
// dispatched on the channel.
//
// The channel with which a registered interface is associated depends on the
// configuration of the specific AssociatedInterfaceRegistry instance. For
// example, RenderFrame exposes an instance of this class for which all
// interfaces are associated with the IPC::SyncChannel to the browser.
class AssociatedInterfaceRegistry {
 public:
  using Binder =
      base::RepeatingCallback<void(mojo::ScopedInterfaceEndpointHandle)>;

  virtual ~AssociatedInterfaceRegistry() {}

  // Adds an interface binder to the registry.
  virtual void AddInterface(const std::string& name, const Binder& binder) = 0;

  // Removes an interface binder from the registry.
  virtual void RemoveInterface(const std::string& name) = 0;

  template <typename Interface>
  using InterfaceBinder = base::RepeatingCallback<void(
      mojo::AssociatedInterfaceRequest<Interface>)>;

  // Templated helper for AddInterface() above.
  template <typename Interface>
  void AddInterface(const InterfaceBinder<Interface>& binder) {
    AddInterface(Interface::Name_,
                 base::BindRepeating(&BindInterface<Interface>, binder));
  }

 private:
  template <typename Interface>
  static void BindInterface(const InterfaceBinder<Interface>& binder,
                            mojo::ScopedInterfaceEndpointHandle handle) {
    binder.Run(mojo::AssociatedInterfaceRequest<Interface>(std::move(handle)));
  }
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_COMMON_ASSOCIATED_INTERFACES_ASSOCIATED_INTERFACE_REGISTRY_H_
