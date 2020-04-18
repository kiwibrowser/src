// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEBAUTH_AUTHENTICATOR_IMPL_H_
#define CONTENT_BROWSER_WEBAUTH_AUTHENTICATOR_IMPL_H_

#include <stdint.h>

#include <memory>
#include <string>

#include "base/containers/flat_set.h"
#include "base/containers/span.h"
#include "base/macros.h"
#include "base/optional.h"
#include "content/common/content_export.h"
#include "content/public/browser/web_contents_observer.h"
#include "device/fido/authenticator_get_assertion_response.h"
#include "device/fido/authenticator_make_credential_response.h"
#include "device/fido/fido_constants.h"
#include "device/fido/fido_transport_protocol.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "third_party/blink/public/platform/modules/webauth/authenticator.mojom.h"
#include "url/origin.h"

namespace base {
class OneShotTimer;
}

namespace device {

class U2fRequest;
class FidoRequestHandlerBase;

enum class FidoReturnCode : uint8_t;

}  // namespace device

namespace service_manager {
class Connector;
}  // namespace service_manager

namespace url {
class Origin;
}

namespace content {

class RenderFrameHost;

namespace client_data {
// These enumerate the possible values for the `type` member of
// CollectedClientData. See
// https://w3c.github.io/webauthn/#dom-collectedclientdata-type
CONTENT_EXPORT extern const char kCreateType[];
CONTENT_EXPORT extern const char kGetType[];
}  // namespace client_data

// Implementation of the public Authenticator interface.
class CONTENT_EXPORT AuthenticatorImpl : public webauth::mojom::Authenticator,
                                         public WebContentsObserver {
 public:
  explicit AuthenticatorImpl(RenderFrameHost* render_frame_host);

  // Permits setting connector and timer for testing. Using this constructor
  // will also empty out the protocol set, since no device discovery will take
  // place during tests.
  AuthenticatorImpl(RenderFrameHost* render_frame_host,
                    service_manager::Connector*,
                    std::unique_ptr<base::OneShotTimer>);
  ~AuthenticatorImpl() override;

  // Creates a binding between this implementation and |request|.
  //
  // Note that one AuthenticatorImpl instance can be bound to exactly one
  // interface connection at a time, and disconnected when the frame navigates
  // to a new active document.
  void Bind(webauth::mojom::AuthenticatorRequest request);

 private:
  friend class AuthenticatorImplTest;

  // Enumerates whether or not to check that the WebContents has focus.
  enum class Focus {
    kDoCheck,
    kDontCheck,
  };

  // Builds the CollectedClientData[1] dictionary with the given values,
  // serializes it to JSON, and returns the resulting string.
  // [1] https://w3c.github.io/webauthn/#dictdef-collectedclientdata
  static std::string SerializeCollectedClientDataToJson(
      const std::string& type,
      const url::Origin& origin,
      base::span<const uint8_t> challenge,
      base::Optional<base::span<const uint8_t>> token_binding);

  // mojom:Authenticator
  void MakeCredential(
      webauth::mojom::PublicKeyCredentialCreationOptionsPtr options,
      MakeCredentialCallback callback) override;
  void GetAssertion(
      webauth::mojom::PublicKeyCredentialRequestOptionsPtr options,
      GetAssertionCallback callback) override;

  // WebContentsObserver:
  void DidFinishNavigation(NavigationHandle* navigation_handle) override;
  void RenderFrameDeleted(RenderFrameHost* render_frame_host) override;

  // Callback to handle the async response from a U2fDevice.
  void OnRegisterResponse(
      device::FidoReturnCode status_code,
      base::Optional<device::AuthenticatorMakeCredentialResponse>
          response_data);

  // Callback to complete the registration process once a decision about
  // whether or not to return attestation data has been made.
  void OnRegisterResponseAttestationDecided(
      device::AuthenticatorMakeCredentialResponse response_data,
      bool attestation_permitted);

  // Callback to handle the async response from a U2fDevice.
  void OnSignResponse(
      device::FidoReturnCode status_code,
      base::Optional<device::AuthenticatorGetAssertionResponse> response_data);

  // Runs when timer expires and cancels all issued requests to a U2fDevice.
  void OnTimeout();

  void InvokeCallbackAndCleanup(
      MakeCredentialCallback callback,
      webauth::mojom::AuthenticatorStatus status,
      webauth::mojom::MakeCredentialAuthenticatorResponsePtr response,
      Focus focus_check);
  void InvokeCallbackAndCleanup(
      GetAssertionCallback callback,
      webauth::mojom::AuthenticatorStatus status,
      webauth::mojom::GetAssertionAuthenticatorResponsePtr response);
  void Cleanup();

  RenderFrameHost* render_frame_host_;
  service_manager::Connector* connector_ = nullptr;
  base::flat_set<device::FidoTransportProtocol> protocols_;

  std::unique_ptr<device::U2fRequest> u2f_request_;
  std::unique_ptr<device::FidoRequestHandlerBase> ctap_request_;
  MakeCredentialCallback make_credential_response_callback_;
  GetAssertionCallback get_assertion_response_callback_;
  std::string client_data_json_;
  webauth::mojom::AttestationConveyancePreference attestation_preference_;
  std::string relying_party_id_;
  std::unique_ptr<base::OneShotTimer> timer_;

  // Whether or not a GetAssertion call should return a PublicKeyCredential
  // instance whose getClientExtensionResults() method yields a
  // AuthenticationExtensions dictionary that contains the `appid: true`
  // extension output.
  bool echo_appid_extension_ = false;

  // Owns pipes to this Authenticator from |render_frame_host_|.
  mojo::Binding<webauth::mojom::Authenticator> binding_;

  base::WeakPtrFactory<AuthenticatorImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(AuthenticatorImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_WEBAUTH_AUTHENTICATOR_IMPL_H_
