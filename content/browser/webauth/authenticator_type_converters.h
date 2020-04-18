// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_WEBAUTH_AUTHENTICATOR_TYPE_CONVERTERS_H_
#define CONTENT_BROWSER_WEBAUTH_AUTHENTICATOR_TYPE_CONVERTERS_H_

#include <vector>

#include "device/fido/authenticator_selection_criteria.h"
#include "device/fido/fido_cable_discovery.h"
#include "device/fido/fido_transport_protocol.h"
#include "device/fido/public_key_credential_descriptor.h"
#include "device/fido/public_key_credential_params.h"
#include "device/fido/public_key_credential_rp_entity.h"
#include "device/fido/public_key_credential_user_entity.h"
#include "mojo/public/cpp/bindings/type_converter.h"
#include "third_party/blink/public/platform/modules/webauth/authenticator.mojom.h"

// TODO(hongjunchoi): Remove type converters and instead expose mojo interface
// directly from device/fido service.
// See: https://crbug.com/831209
namespace mojo {

template <>
struct TypeConverter<::device::FidoTransportProtocol,
                     ::webauth::mojom::AuthenticatorTransport> {
  static ::device::FidoTransportProtocol Convert(
      const ::webauth::mojom::AuthenticatorTransport& input);
};

template <>
struct TypeConverter<::device::CredentialType,
                     ::webauth::mojom::PublicKeyCredentialType> {
  static ::device::CredentialType Convert(
      const ::webauth::mojom::PublicKeyCredentialType& input);
};

template <>
struct TypeConverter<
    std::vector<::device::PublicKeyCredentialParams::CredentialInfo>,
    std::vector<::webauth::mojom::PublicKeyCredentialParametersPtr>> {
  static std::vector<::device::PublicKeyCredentialParams::CredentialInfo>
  Convert(const std::vector<::webauth::mojom::PublicKeyCredentialParametersPtr>&
              input);
};

template <>
struct TypeConverter<
    std::vector<::device::PublicKeyCredentialDescriptor>,
    std::vector<::webauth::mojom::PublicKeyCredentialDescriptorPtr>> {
  static std::vector<::device::PublicKeyCredentialDescriptor> Convert(
      const std::vector<::webauth::mojom::PublicKeyCredentialDescriptorPtr>&
          input);
};

template <>
struct TypeConverter<
    ::device::AuthenticatorSelectionCriteria::AuthenticatorAttachment,
    ::webauth::mojom::AuthenticatorAttachment> {
  static ::device::AuthenticatorSelectionCriteria::AuthenticatorAttachment
  Convert(const ::webauth::mojom::AuthenticatorAttachment& input);
};

template <>
struct TypeConverter<::device::UserVerificationRequirement,
                     ::webauth::mojom::UserVerificationRequirement> {
  static ::device::UserVerificationRequirement Convert(
      const ::webauth::mojom::UserVerificationRequirement& input);
};

template <>
struct TypeConverter<::device::AuthenticatorSelectionCriteria,
                     ::webauth::mojom::AuthenticatorSelectionCriteriaPtr> {
  static ::device::AuthenticatorSelectionCriteria Convert(
      const ::webauth::mojom::AuthenticatorSelectionCriteriaPtr& input);
};

template <>
struct TypeConverter<::device::PublicKeyCredentialRpEntity,
                     ::webauth::mojom::PublicKeyCredentialRpEntityPtr> {
  static ::device::PublicKeyCredentialRpEntity Convert(
      const ::webauth::mojom::PublicKeyCredentialRpEntityPtr& input);
};

template <>
struct TypeConverter<::device::PublicKeyCredentialUserEntity,
                     ::webauth::mojom::PublicKeyCredentialUserEntityPtr> {
  static ::device::PublicKeyCredentialUserEntity Convert(
      const ::webauth::mojom::PublicKeyCredentialUserEntityPtr& input);
};

template <>
struct TypeConverter<
    std::vector<::device::FidoCableDiscovery::CableDiscoveryData>,
    std::vector<::webauth::mojom::CableAuthenticationPtr>> {
  static std::vector<::device::FidoCableDiscovery::CableDiscoveryData> Convert(
      const std::vector<::webauth::mojom::CableAuthenticationPtr>& input);
};

}  // namespace mojo

#endif  // CONTENT_BROWSER_WEBAUTH_AUTHENTICATOR_TYPE_CONVERTERS_H_
