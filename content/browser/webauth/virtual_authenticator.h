// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEBAUTH_VIRTUAL_AUTHENTICATOR_H_
#define CONTENT_BROWSER_WEBAUTH_VIRTUAL_AUTHENTICATOR_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "content/common/content_export.h"
#include "device/fido/fido_transport_protocol.h"
#include "device/fido/virtual_fido_device.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "third_party/blink/public/platform/modules/webauth/virtual_authenticator.mojom.h"

namespace content {

// Implements the Mojo interface representing a stateful virtual authenticator.
//
// This class has very little logic itself, it merely stores a unique ID and the
// state of the authenticator, whereas performing all cryptographic operations
// is delegated to the VirtualFidoDevice class.
class CONTENT_EXPORT VirtualAuthenticator
    : public webauth::test::mojom::VirtualAuthenticator {
 public:
  explicit VirtualAuthenticator(::device::FidoTransportProtocol transport);
  ~VirtualAuthenticator() override;

  void AddBinding(webauth::test::mojom::VirtualAuthenticatorRequest request);

  ::device::FidoTransportProtocol transport() const { return transport_; }
  const std::string& unique_id() const { return unique_id_; }

  // Constructs a VirtualFidoDevice instance that will perform cryptographic
  // operations on behalf of, and using the state stored in this virtual
  // authenticator.
  //
  // There is an N:1 relationship between VirtualFidoDevices and this class, so
  // this method can be called any number of times.
  std::unique_ptr<::device::FidoDevice> ConstructDevice();

 protected:
  // webauth::test::mojom::VirtualAuthenticator:
  void GetUniqueId(GetUniqueIdCallback callback) override;

  void GetRegistrations(GetRegistrationsCallback callback) override;
  void AddRegistration(webauth::test::mojom::RegisteredKeyPtr registration,
                       AddRegistrationCallback callback) override;
  void ClearRegistrations(ClearRegistrationsCallback callback) override;

  void SetUserPresence(bool present, SetUserPresenceCallback callback) override;
  void GetUserPresence(GetUserPresenceCallback callback) override;

 private:
  const ::device::FidoTransportProtocol transport_;
  const std::string unique_id_;
  scoped_refptr<::device::VirtualFidoDevice::State> state_;
  mojo::BindingSet<webauth::test::mojom::VirtualAuthenticator> binding_set_;

  DISALLOW_COPY_AND_ASSIGN(VirtualAuthenticator);
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEBAUTH_VIRTUAL_AUTHENTICATOR_H_
