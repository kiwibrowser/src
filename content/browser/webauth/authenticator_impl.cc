// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webauth/authenticator_impl.h"

#include <string>
#include <utility>
#include <vector>

#include "base/base64url.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/rand_util.h"
#include "base/timer/timer.h"
#include "build/build_config.h"
#include "content/browser/bad_message.h"
#include "content/browser/webauth/authenticator_type_converters.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_client.h"
#include "content/public/common/content_features.h"
#include "content/public/common/origin_util.h"
#include "content/public/common/service_manager_connection.h"
#include "crypto/sha2.h"
#include "device/fido/authenticator_selection_criteria.h"
#include "device/fido/ctap_get_assertion_request.h"
#include "device/fido/ctap_make_credential_request.h"
#include "device/fido/fido_transport_protocol.h"
#include "device/fido/get_assertion_request_handler.h"
#include "device/fido/make_credential_request_handler.h"
#include "device/fido/public_key_credential_descriptor.h"
#include "device/fido/public_key_credential_params.h"
#include "device/fido/u2f_register.h"
#include "device/fido/u2f_request.h"
#include "device/fido/u2f_sign.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"
#include "services/service_manager/public/cpp/connector.h"
#include "url/url_constants.h"
#include "url/url_util.h"

namespace content {

namespace client_data {
const char kCreateType[] = "webauthn.create";
const char kGetType[] = "webauthn.get";
}  // namespace client_data

namespace {
constexpr int32_t kCoseEs256 = -7;

// Ensure that the origin's effective domain is a valid domain.
// Only the domain format of host is valid.
// Reference https://url.spec.whatwg.org/#valid-domain-string and
// https://html.spec.whatwg.org/multipage/origin.html#concept-origin-effective-domain.
bool HasValidEffectiveDomain(url::Origin caller_origin) {
  return !caller_origin.unique() &&
         !url::HostIsIPAddress(caller_origin.host()) &&
         content::IsOriginSecure(caller_origin.GetURL()) &&
         // Additionally, the scheme is required to be HTTP(S). Other schemes
         // may be supported in the future but the webauthn relying party is
         // just the domain of the origin so we would have to define how the
         // authority part of other schemes maps to a "domain" without
         // collisions. Given the |IsOriginSecure| check, just above, HTTP is
         // effectively restricted to just "localhost".
         (caller_origin.scheme() == url::kHttpScheme ||
          caller_origin.scheme() == url::kHttpsScheme);
}

// Ensure the relying party ID is a registrable domain suffix of or equal
// to the origin's effective domain. Reference:
// https://html.spec.whatwg.org/multipage/origin.html#is-a-registrable-domain-suffix-of-or-is-equal-to.
bool IsRelyingPartyIdValid(const std::string& relying_party_id,
                           url::Origin caller_origin) {
  if (relying_party_id.empty())
    return false;

  if (caller_origin.host() == relying_party_id)
    return true;

  if (!caller_origin.DomainIs(relying_party_id))
    return false;
  if (!net::registry_controlled_domains::HostHasRegistryControlledDomain(
          caller_origin.host(),
          net::registry_controlled_domains::INCLUDE_UNKNOWN_REGISTRIES,
          net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES))
    return false;
  if (!net::registry_controlled_domains::HostHasRegistryControlledDomain(
          relying_party_id,
          net::registry_controlled_domains::INCLUDE_UNKNOWN_REGISTRIES,
          net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES))
    // TODO(crbug.com/803414): Accept corner-case situations like the following
    // origin: "https://login.awesomecompany",
    // relying_party_id: "awesomecompany".
    return false;
  return true;
}

bool IsAppIdAllowedForOrigin(const GURL& appid, const url::Origin& origin) {
  // See
  // https://fidoalliance.org/specs/fido-u2f-v1.2-ps-20170411/fido-appid-and-facets-v1.2-ps-20170411.html#determining-if-a-caller-s-facetid-is-authorized-for-an-appid

  // Step 1: "If the AppID is not an HTTPS URL, and matches the FacetID of the
  // caller, no additional processing is necessary and the operation may
  // proceed."

  // Webauthn is only supported on secure origins and |HasValidEffectiveDomain|
  // has already checked this property of |origin| before this call. Thus this
  // step is moot.
  DCHECK(content::IsOriginSecure(origin.GetURL()));

  // Step 2: "If the AppID is null or empty, the client must set the AppID to be
  // the FacetID of the caller, and the operation may proceed without additional
  // processing."

  // This step is handled before calling this function.

  // Step 3: "If the caller's FacetID is an https:// Origin sharing the same
  // host as the AppID, (e.g. if an application hosted at
  // https://fido.example.com/myApp set an AppID of
  // https://fido.example.com/myAppId), no additional processing is necessary
  // and the operation may proceed."
  if (origin.scheme() != url::kHttpsScheme ||
      appid.scheme_piece() != origin.scheme()) {
    return false;
  }

  // This check is repeated inside |SameDomainOrHost|, just after this. However
  // it's cheap and mirrors the structure of the spec.
  if (appid.host_piece() == origin.host()) {
    return true;
  }

  // At this point we diverge from the specification in order to avoid the
  // complexity of making a network request which isn't believed to be
  // neccessary in practice. See also
  // https://bugzilla.mozilla.org/show_bug.cgi?id=1244959#c8
  if (net::registry_controlled_domains::SameDomainOrHost(
          appid, origin,
          net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES)) {
    return true;
  }

  // As a compatibility hack, sites within google.com are allowed to assert two
  // special-case AppIDs. Firefox also does this:
  // https://groups.google.com/forum/#!msg/mozilla.dev.platform/Uiu3fwnA2xw/201ynAiPAQAJ
  const GURL kGstatic1 =
      GURL("https://www.gstatic.com/securitykey/origins.json");
  const GURL kGstatic2 =
      GURL("https://www.gstatic.com/securitykey/a/google.com/origins.json");
  DCHECK(kGstatic1.is_valid() && kGstatic2.is_valid());

  if (origin.DomainIs("google.com") && !appid.has_ref() &&
      (appid.EqualsIgnoringRef(kGstatic1) ||
       appid.EqualsIgnoringRef(kGstatic2))) {
    return true;
  }

  return false;
}

// Check that at least one of the cryptographic parameters is supported.
// Only ES256 is currently supported by U2F_V2 (CTAP 1.0).
bool IsAlgorithmSupportedByU2fAuthenticators(
    const std::vector<webauth::mojom::PublicKeyCredentialParametersPtr>&
        parameters) {
  for (const auto& params : parameters) {
    if (params->algorithm_identifier == kCoseEs256)
      return true;
  }
  return false;
}

// Verify that the request doesn't contain parameters that U2F authenticators
// cannot fulfill.
bool AreOptionsSupportedByU2fAuthenticators(
    const webauth::mojom::PublicKeyCredentialCreationOptionsPtr& options) {
  if (options->authenticator_selection) {
    if (options->authenticator_selection->user_verification ==
            webauth::mojom::UserVerificationRequirement::REQUIRED ||
        options->authenticator_selection->require_resident_key ||
        options->authenticator_selection->authenticator_attachment ==
            webauth::mojom::AuthenticatorAttachment::PLATFORM)
      return false;
  }
  return true;
}

std::vector<std::vector<uint8_t>> FilterCredentialList(
    const std::vector<webauth::mojom::PublicKeyCredentialDescriptorPtr>&
        descriptors) {
  std::vector<std::vector<uint8_t>> handles;
  for (const auto& credential_descriptor : descriptors) {
    if (credential_descriptor->type ==
        webauth::mojom::PublicKeyCredentialType::PUBLIC_KEY) {
      handles.push_back(credential_descriptor->id);
    }
  }
  return handles;
}

device::CtapMakeCredentialRequest CreateCtapMakeCredentialRequest(
    std::vector<uint8_t> client_data_hash,
    const webauth::mojom::PublicKeyCredentialCreationOptionsPtr& options) {
  auto credential_params = mojo::ConvertTo<
      std::vector<device::PublicKeyCredentialParams::CredentialInfo>>(
      options->public_key_parameters);

  device::CtapMakeCredentialRequest make_credential_param(
      std::move(client_data_hash),
      mojo::ConvertTo<device::PublicKeyCredentialRpEntity>(
          options->relying_party),
      mojo::ConvertTo<device::PublicKeyCredentialUserEntity>(options->user),
      device::PublicKeyCredentialParams(std::move(credential_params)));

  auto exclude_list =
      mojo::ConvertTo<std::vector<device::PublicKeyCredentialDescriptor>>(
          options->exclude_credentials);

  make_credential_param.SetExcludeList(std::move(exclude_list));
  return make_credential_param;
}

device::CtapGetAssertionRequest CreateCtapGetAssertionRequest(
    std::vector<uint8_t> client_data_hash,
    const webauth::mojom::PublicKeyCredentialRequestOptionsPtr& options) {
  device::CtapGetAssertionRequest request_parameter(
      options->relying_party_id, std::move(client_data_hash));

  auto allowed_list =
      mojo::ConvertTo<std::vector<device::PublicKeyCredentialDescriptor>>(
          options->allow_credentials);

  request_parameter.SetAllowList(std::move(allowed_list));
  request_parameter.SetUserVerification(
      mojo::ConvertTo<device::UserVerificationRequirement>(
          options->user_verification));

  if (!options->cable_authentication_data.empty()) {
    request_parameter.SetCableExtension(
        mojo::ConvertTo<
            std::vector<device::FidoCableDiscovery::CableDiscoveryData>>(
            options->cable_authentication_data));
  }
  return request_parameter;
}

std::vector<uint8_t> ConstructClientDataHash(const std::string& client_data) {
  // SHA-256 hash of the JSON data structure.
  std::vector<uint8_t> client_data_hash(crypto::kSHA256Length);
  crypto::SHA256HashString(client_data, client_data_hash.data(),
                           client_data_hash.size());
  return client_data_hash;
}

// The application parameter is the SHA-256 hash of the UTF-8 encoding of
// the application identity (i.e. relying_party_id) of the application
// requesting the registration.
std::vector<uint8_t> CreateApplicationParameter(
    const std::string& relying_party_id) {
  std::vector<uint8_t> application_parameter(crypto::kSHA256Length);
  crypto::SHA256HashString(relying_party_id, application_parameter.data(),
                           application_parameter.size());
  return application_parameter;
}

base::Optional<std::vector<uint8_t>> ProcessAppIdExtension(
    std::string appid,
    const url::Origin& caller_origin) {
  if (appid.empty()) {
    // See step two in the comments in |IsAppIdAllowedForOrigin|.
    appid = caller_origin.Serialize() + "/";
  }

  GURL appid_url = GURL(appid);
  if (!(appid_url.is_valid() &&
        IsAppIdAllowedForOrigin(appid_url, caller_origin))) {
    return base::nullopt;
  }

  return CreateApplicationParameter(appid);
}

webauth::mojom::MakeCredentialAuthenticatorResponsePtr
CreateMakeCredentialResponse(
    const std::string& client_data_json,
    device::AuthenticatorMakeCredentialResponse response_data) {
  auto response = webauth::mojom::MakeCredentialAuthenticatorResponse::New();
  auto common_info = webauth::mojom::CommonCredentialInfo::New();
  common_info->client_data_json.assign(client_data_json.begin(),
                                       client_data_json.end());
  common_info->raw_id = response_data.raw_credential_id();
  common_info->id = response_data.GetId();
  response->info = std::move(common_info);
  response->attestation_object =
      response_data.GetCBOREncodedAttestationObject();
  return response;
}

webauth::mojom::GetAssertionAuthenticatorResponsePtr CreateGetAssertionResponse(
    const std::string& client_data_json,
    device::AuthenticatorGetAssertionResponse response_data,
    bool echo_appid_extension) {
  auto response = webauth::mojom::GetAssertionAuthenticatorResponse::New();
  auto common_info = webauth::mojom::CommonCredentialInfo::New();
  common_info->client_data_json.assign(client_data_json.begin(),
                                       client_data_json.end());
  common_info->raw_id = response_data.raw_credential_id();
  common_info->id = response_data.GetId();
  response->info = std::move(common_info);
  response->authenticator_data =
      response_data.auth_data().SerializeToByteArray();
  response->signature = response_data.signature();
  response->echo_appid_extension = echo_appid_extension;
  response_data.user_entity()
      ? response->user_handle.emplace(response_data.user_entity()->user_id())
      : response->user_handle.emplace();
  return response;
}

std::string Base64UrlEncode(const base::span<const uint8_t> input) {
  std::string ret;
  base::Base64UrlEncode(
      base::StringPiece(reinterpret_cast<const char*>(input.data()),
                        input.size()),
      base::Base64UrlEncodePolicy::OMIT_PADDING, &ret);
  return ret;
}

}  // namespace

AuthenticatorImpl::AuthenticatorImpl(RenderFrameHost* render_frame_host)
    : AuthenticatorImpl(render_frame_host,
                        nullptr /* connector */,
                        std::make_unique<base::OneShotTimer>()) {}

AuthenticatorImpl::AuthenticatorImpl(RenderFrameHost* render_frame_host,
                                     service_manager::Connector* connector,
                                     std::unique_ptr<base::OneShotTimer> timer)
    : WebContentsObserver(WebContents::FromRenderFrameHost(render_frame_host)),
      render_frame_host_(render_frame_host),
      connector_(connector),
      timer_(std::move(timer)),
      binding_(this),
      weak_factory_(this) {
  DCHECK(render_frame_host_);
  DCHECK(timer_);

  protocols_.insert(device::FidoTransportProtocol::kUsbHumanInterfaceDevice);
  if (base::FeatureList::IsEnabled(features::kWebAuthBle)) {
    protocols_.insert(device::FidoTransportProtocol::kBluetoothLowEnergy);
  }

  if (base::FeatureList::IsEnabled(features::kWebAuthCable)) {
    protocols_.insert(
        device::FidoTransportProtocol::kCloudAssistedBluetoothLowEnergy);
  }
#if defined(OS_MACOSX)
  if (base::FeatureList::IsEnabled(features::kWebAuthTouchId)) {
    protocols_.insert(device::FidoTransportProtocol::kInternal);
  }
#endif
}

AuthenticatorImpl::~AuthenticatorImpl() {}

void AuthenticatorImpl::Bind(webauth::mojom::AuthenticatorRequest request) {
  DCHECK(!binding_.is_bound());
  binding_.Bind(std::move(request));
}

// static
std::string AuthenticatorImpl::SerializeCollectedClientDataToJson(
    const std::string& type,
    const url::Origin& origin,
    base::span<const uint8_t> challenge,
    base::Optional<base::span<const uint8_t>> token_binding) {
  static constexpr char kTypeKey[] = "type";
  static constexpr char kChallengeKey[] = "challenge";
  static constexpr char kOriginKey[] = "origin";
  static constexpr char kTokenBindingKey[] = "tokenBinding";

  base::DictionaryValue client_data;
  client_data.SetKey(kTypeKey, base::Value(type));
  client_data.SetKey(kChallengeKey, base::Value(Base64UrlEncode(challenge)));
  client_data.SetKey(kOriginKey, base::Value(origin.Serialize()));

  if (token_binding) {
    base::DictionaryValue token_binding_dict;
    static constexpr char kTokenBindingStatusKey[] = "status";
    static constexpr char kTokenBindingIdKey[] = "id";
    static constexpr char kTokenBindingSupportedStatus[] = "supported";
    static constexpr char kTokenBindingPresentStatus[] = "present";

    if (token_binding->empty()) {
      token_binding_dict.SetKey(kTokenBindingStatusKey,
                                base::Value(kTokenBindingSupportedStatus));
    } else {
      token_binding_dict.SetKey(kTokenBindingStatusKey,
                                base::Value(kTokenBindingPresentStatus));
      token_binding_dict.SetKey(kTokenBindingIdKey,
                                base::Value(Base64UrlEncode(*token_binding)));
    }

    client_data.SetKey(kTokenBindingKey, std::move(token_binding_dict));
  }

  if (base::RandDouble() < 0.2) {
    // An extra key is sometimes added to ensure that RPs do not make
    // unreasonably specific assumptions about the clientData JSON. This is
    // done in the fashion of
    // https://tools.ietf.org/html/draft-davidben-tls-grease-01
    client_data.SetKey("new_keys_may_be_added_here",
                       base::Value("do not compare clientDataJSON against a "
                                   "template. See https://goo.gl/yabPex"));
  }

  std::string json;
  base::JSONWriter::Write(client_data, &json);
  return json;
}

// mojom::Authenticator
void AuthenticatorImpl::MakeCredential(
    webauth::mojom::PublicKeyCredentialCreationOptionsPtr options,
    MakeCredentialCallback callback) {
  if (u2f_request_ || ctap_request_) {
    std::move(callback).Run(
        webauth::mojom::AuthenticatorStatus::PENDING_REQUEST, nullptr);
    return;
  }

  if (!GetContentClient()->browser()->IsFocused(
          WebContents::FromRenderFrameHost(render_frame_host_))) {
    std::move(callback).Run(webauth::mojom::AuthenticatorStatus::NOT_FOCUSED,
                            nullptr);
    return;
  }

  url::Origin caller_origin = render_frame_host_->GetLastCommittedOrigin();
  relying_party_id_ = options->relying_party->id;

  if (!HasValidEffectiveDomain(caller_origin)) {
    bad_message::ReceivedBadMessage(render_frame_host_->GetProcess(),
                                    bad_message::AUTH_INVALID_EFFECTIVE_DOMAIN);
    InvokeCallbackAndCleanup(
        std::move(callback),
        webauth::mojom::AuthenticatorStatus::INVALID_DOMAIN, nullptr,
        Focus::kDontCheck);
    return;
  }

  if (!IsRelyingPartyIdValid(relying_party_id_, caller_origin)) {
    bad_message::ReceivedBadMessage(render_frame_host_->GetProcess(),
                                    bad_message::AUTH_INVALID_RELYING_PARTY);
    InvokeCallbackAndCleanup(
        std::move(callback),
        webauth::mojom::AuthenticatorStatus::INVALID_DOMAIN, nullptr,
        Focus::kDontCheck);
    return;
  }

  // Verify that the request doesn't contain parameters that U2F authenticators
  // cannot fulfill.
  // TODO(crbug.com/819256): Improve messages for "Not Allowed" errors.
  if (!base::FeatureList::IsEnabled(features::kWebAuthCtap2) &&
      !AreOptionsSupportedByU2fAuthenticators(options)) {
    InvokeCallbackAndCleanup(
        std::move(callback),
        webauth::mojom::AuthenticatorStatus::AUTHENTICATOR_CRITERIA_UNSUPPORTED,
        nullptr, Focus::kDontCheck);
    return;
  }

  if (!base::FeatureList::IsEnabled(features::kWebAuthCtap2) &&
      !IsAlgorithmSupportedByU2fAuthenticators(
          options->public_key_parameters)) {
    InvokeCallbackAndCleanup(
        std::move(callback),
        webauth::mojom::AuthenticatorStatus::ALGORITHM_UNSUPPORTED, nullptr,
        Focus::kDontCheck);
    return;
  }

  DCHECK(make_credential_response_callback_.is_null());
  make_credential_response_callback_ = std::move(callback);

  timer_->Start(
      FROM_HERE, options->adjusted_timeout,
      base::Bind(&AuthenticatorImpl::OnTimeout, base::Unretained(this)));
  if (!connector_)
    connector_ = ServiceManagerConnection::GetForProcess()->GetConnector();

  // Extract list of credentials to exclude.
  std::vector<std::vector<uint8_t>> registered_keys;
  for (const auto& credential : options->exclude_credentials) {
    registered_keys.push_back(credential->id);
  }

  // Save client data to return with the authenticator response.
  // TODO(kpaulhamus): Fetch and add the Token Binding ID public key used to
  // communicate with the origin.
  client_data_json_ = SerializeCollectedClientDataToJson(
      client_data::kCreateType, caller_origin, std::move(options->challenge),
      base::nullopt);

  const bool individual_attestation =
      GetContentClient()
          ->browser()
          ->ShouldPermitIndividualAttestationForWebauthnRPID(
              render_frame_host_->GetProcess()->GetBrowserContext(),
              relying_party_id_);

  attestation_preference_ = options->attestation;

  // Communication using Cable protocol is only supported for GetAssertion
  // request on CTAP2 devices.
  protocols_.erase(
      device::FidoTransportProtocol::kCloudAssistedBluetoothLowEnergy);

  if (base::FeatureList::IsEnabled(features::kWebAuthCtap2)) {
    auto authenticator_selection_criteria =
        options->authenticator_selection
            ? mojo::ConvertTo<device::AuthenticatorSelectionCriteria>(
                  options->authenticator_selection)
            : device::AuthenticatorSelectionCriteria();

    ctap_request_ = std::make_unique<device::MakeCredentialRequestHandler>(
        connector_, protocols_,
        CreateCtapMakeCredentialRequest(
            ConstructClientDataHash(client_data_json_), options),
        std::move(authenticator_selection_criteria),
        base::BindOnce(&AuthenticatorImpl::OnRegisterResponse,
                       weak_factory_.GetWeakPtr()));
  } else {
    // TODO(kpaulhamus): Mock U2fRegister for unit tests.
    // http://crbug.com/785955.
    // Per fido-u2f-raw-message-formats:
    // The challenge parameter is the SHA-256 hash of the Client Data,
    // Among other things, the Client Data contains the challenge from the
    // relying party (hence the name of the parameter).
    u2f_request_ = device::U2fRegister::TryRegistration(
        connector_, protocols_, registered_keys,
        ConstructClientDataHash(client_data_json_),
        CreateApplicationParameter(relying_party_id_), individual_attestation,
        base::BindOnce(&AuthenticatorImpl::OnRegisterResponse,
                       weak_factory_.GetWeakPtr()));
  }
}

// mojom:Authenticator
void AuthenticatorImpl::GetAssertion(
    webauth::mojom::PublicKeyCredentialRequestOptionsPtr options,
    GetAssertionCallback callback) {
  if (u2f_request_ || ctap_request_) {
    std::move(callback).Run(
        webauth::mojom::AuthenticatorStatus::PENDING_REQUEST, nullptr);
    return;
  }

  url::Origin caller_origin = render_frame_host_->GetLastCommittedOrigin();

  if (!HasValidEffectiveDomain(caller_origin)) {
    bad_message::ReceivedBadMessage(render_frame_host_->GetProcess(),
                                    bad_message::AUTH_INVALID_EFFECTIVE_DOMAIN);
    InvokeCallbackAndCleanup(
        std::move(callback),
        webauth::mojom::AuthenticatorStatus::INVALID_DOMAIN, nullptr);
    return;
  }

  if (!IsRelyingPartyIdValid(options->relying_party_id, caller_origin)) {
    bad_message::ReceivedBadMessage(render_frame_host_->GetProcess(),
                                    bad_message::AUTH_INVALID_RELYING_PARTY);
    InvokeCallbackAndCleanup(
        std::move(callback),
        webauth::mojom::AuthenticatorStatus::INVALID_DOMAIN, nullptr);
    return;
  }

  // To use U2F, the relying party must not require user verification.
  if (!base::FeatureList::IsEnabled(features::kWebAuthCtap2) &&
      options->user_verification ==
          webauth::mojom::UserVerificationRequirement::REQUIRED) {
    InvokeCallbackAndCleanup(
        std::move(callback),
        webauth::mojom::AuthenticatorStatus::USER_VERIFICATION_UNSUPPORTED,
        nullptr);
    return;
  }

  std::vector<uint8_t> application_parameter =
      CreateApplicationParameter(options->relying_party_id);

  base::Optional<std::vector<uint8_t>> alternative_application_parameter;
  if (options->appid) {
    auto appid_hash = ProcessAppIdExtension(*options->appid, caller_origin);
    if (!appid_hash) {
      std::move(callback).Run(
          webauth::mojom::AuthenticatorStatus::INVALID_DOMAIN, nullptr);
      return;
    }

    alternative_application_parameter = std::move(appid_hash);
    // TODO(agl): needs a test once a suitable, mock U2F device exists.
    echo_appid_extension_ = true;
  }

  // Pass along valid keys from allow_list.
  std::vector<std::vector<uint8_t>> handles =
      FilterCredentialList(std::move(options->allow_credentials));

  // There are two different descriptions of what should happen when
  // "allowCredentials" is empty for U2F.
  // a) WebAuthN 6.2.3 step 6[1] implies "NotAllowedError".
  // b) CTAP step 7.2 step 2[2] says the device should error out with
  // "CTAP2_ERR_OPTION_NOT_SUPPORTED". This also resolves to "NotAllowedError".
  // The behavior in both cases is consistent with the current implementation.
  // When CTAP2 is enabled, however, this check is done by handlers in
  // fido/device on a per-device basis.

  // [1] https://w3c.github.io/webauthn/#authenticatorgetassertion
  // [2]
  // https://fidoalliance.org/specs/fido-v2.0-ps-20170927/fido-client-to-authenticator-protocol-v2.0-ps-20170927.html
  if (!base::FeatureList::IsEnabled(features::kWebAuthCtap2) &&
      handles.empty()) {
    InvokeCallbackAndCleanup(
        std::move(callback),
        webauth::mojom::AuthenticatorStatus::EMPTY_ALLOW_CREDENTIALS, nullptr);
    return;
  }

  DCHECK(get_assertion_response_callback_.is_null());
  get_assertion_response_callback_ = std::move(callback);

  timer_->Start(
      FROM_HERE, options->adjusted_timeout,
      base::Bind(&AuthenticatorImpl::OnTimeout, base::Unretained(this)));

  if (!connector_)
    connector_ = ServiceManagerConnection::GetForProcess()->GetConnector();

  // Save client data to return with the authenticator response.
  // TODO(kpaulhamus): Fetch and add the Token Binding ID public key used to
  // communicate with the origin.
  client_data_json_ = SerializeCollectedClientDataToJson(
      client_data::kGetType, caller_origin, std::move(options->challenge),
      base::nullopt);

  if (base::FeatureList::IsEnabled(features::kWebAuthCtap2)) {
    ctap_request_ = std::make_unique<device::GetAssertionRequestHandler>(
        connector_, protocols_,
        CreateCtapGetAssertionRequest(
            ConstructClientDataHash(client_data_json_), options),
        base::BindOnce(&AuthenticatorImpl::OnSignResponse,
                       weak_factory_.GetWeakPtr()));
  } else {
    // Communication using Cable protocol is only supported for CTAP2 devices.
    protocols_.erase(
        device::FidoTransportProtocol::kCloudAssistedBluetoothLowEnergy);

    u2f_request_ = device::U2fSign::TrySign(
        connector_, protocols_, handles,
        ConstructClientDataHash(client_data_json_), application_parameter,
        alternative_application_parameter,
        base::BindOnce(&AuthenticatorImpl::OnSignResponse,
                       weak_factory_.GetWeakPtr()));
  }
}

void AuthenticatorImpl::DidFinishNavigation(
    NavigationHandle* navigation_handle) {
  if (!navigation_handle->HasCommitted() ||
      navigation_handle->IsSameDocument() ||
      navigation_handle->GetRenderFrameHost() != render_frame_host_) {
    return;
  }

  binding_.Close();
  Cleanup();
}

void AuthenticatorImpl::RenderFrameDeleted(RenderFrameHost* render_frame_host) {
  // In tests, the AuthenticatorImpl may outlive the RenderFrameHost, although
  // this cannot happen in a non-test context because, normally,
  // AuthenticatorImpl is owned by RenderFrameHost.
  if (render_frame_host != render_frame_host_) {
    return;
  }

  binding_.Close();
  Cleanup();
}

// Callback to handle the async registration response from a U2fDevice.
void AuthenticatorImpl::OnRegisterResponse(
    device::FidoReturnCode status_code,
    base::Optional<device::AuthenticatorMakeCredentialResponse> response_data) {
  if (!u2f_request_ && !ctap_request_) {
    // Either the callback has been called immediately (in which case
    // |u2f_request_| / |ctap_request_| won't have been set yet), or
    // |RenderFrameDeleted| / |DidFinishNavigation| noticed that this object has
    // been orphaned.
    return;
  }

  switch (status_code) {
    case device::FidoReturnCode::kUserConsentButCredentialExcluded:
      // Duplicate registration: the new credential would be created on an
      // authenticator that already contains one of the credentials in
      // |exclude_credentials|.
      InvokeCallbackAndCleanup(
          std::move(make_credential_response_callback_),
          webauth::mojom::AuthenticatorStatus::CREDENTIAL_EXCLUDED, nullptr,
          Focus::kDoCheck);
      return;
    case device::FidoReturnCode::kAuthenticatorResponseInvalid:
      // The response from the authenticator was corrupted.
      InvokeCallbackAndCleanup(
          std::move(make_credential_response_callback_),
          webauth::mojom::AuthenticatorStatus::NOT_ALLOWED_ERROR, nullptr,
          Focus::kDoCheck);
      return;
    case device::FidoReturnCode::kUserConsentButCredentialNotRecognized:
      NOTREACHED();
      return;
    case device::FidoReturnCode::kSuccess:
      DCHECK(response_data.has_value());

      if (attestation_preference_ !=
          webauth::mojom::AttestationConveyancePreference::NONE) {
        // Check for focus before (potentially) showing a permissions bubble
        // that might take focus.
        if (!GetContentClient()->browser()->IsFocused(
                WebContents::FromRenderFrameHost(render_frame_host_))) {
          InvokeCallbackAndCleanup(
              std::move(make_credential_response_callback_),
              webauth::mojom::AuthenticatorStatus::NOT_FOCUSED, nullptr,
              Focus::kDontCheck);
          return;
        }

        GetContentClient()->browser()->ShouldReturnAttestationForWebauthnRPID(
            render_frame_host_, relying_party_id_,
            render_frame_host_->GetLastCommittedOrigin(),
            base::BindOnce(
                &AuthenticatorImpl::OnRegisterResponseAttestationDecided,
                weak_factory_.GetWeakPtr(), std::move(*response_data)));
        return;
      }

      response_data->EraseAttestationStatement();
      InvokeCallbackAndCleanup(
          std::move(make_credential_response_callback_),
          webauth::mojom::AuthenticatorStatus::SUCCESS,
          CreateMakeCredentialResponse(std::move(client_data_json_),
                                       std::move(*response_data)),
          Focus::kDoCheck);
      return;
  }
  NOTREACHED();
}

void AuthenticatorImpl::OnRegisterResponseAttestationDecided(
    device::AuthenticatorMakeCredentialResponse response_data,
    bool attestation_permitted) {
  DCHECK(attestation_preference_ !=
         webauth::mojom::AttestationConveyancePreference::NONE);

  if (!u2f_request_ && !ctap_request_) {
    // |DidFinishNavigation| / |RenderFrameDeleted| noticed that this object has
    // been orphaned.
    return;
  }

  // At this point, the final focus check has already been done because it's
  // possible that a permissions bubble might have focus and thus, if we did a
  // focus check, it would (incorrectly) fail.

  if (!attestation_permitted) {
    InvokeCallbackAndCleanup(
        std::move(make_credential_response_callback_),
        webauth::mojom::AuthenticatorStatus::NOT_ALLOWED_ERROR, nullptr,
        Focus::kDontCheck);
    return;
  }

  // The check for IsAttestationCertificateInappropriatelyIdentifying is
  // performed after the permissions prompt, even though we know the answer
  // before, because this still effectively discloses the make & model of the
  // authenticator: If an RP sees a "none" attestation from Chrome after
  // requesting direct attestation then it knows that it was one of the tokens
  // with inappropriate certs.
  if (response_data.IsAttestationCertificateInappropriatelyIdentifying() &&
      !GetContentClient()
           ->browser()
           ->ShouldPermitIndividualAttestationForWebauthnRPID(
               render_frame_host_->GetProcess()->GetBrowserContext(),
               relying_party_id_)) {
    // The attestation response is incorrectly individually identifiable, but
    // the consent is for make & model information about a token, not for
    // individually-identifiable information. Erase the attestation to stop it
    // begin a tracking signal.

    // The only way to get the underlying attestation will be to list the RP ID
    // in the enterprise policy, because that enables the individual attestation
    // bit in the register request and permits individual attestation generally.
    response_data.EraseAttestationStatement();
  }

  InvokeCallbackAndCleanup(
      std::move(make_credential_response_callback_),
      webauth::mojom::AuthenticatorStatus::SUCCESS,
      CreateMakeCredentialResponse(std::move(client_data_json_),
                                   std::move(response_data)),
      Focus::kDontCheck);
}

void AuthenticatorImpl::OnSignResponse(
    device::FidoReturnCode status_code,
    base::Optional<device::AuthenticatorGetAssertionResponse> response_data) {
  if (!u2f_request_ && !ctap_request_) {
    // Either the callback has been called immediately (in which case
    // |u2f_request_| / |ctap_request_| won't have been set yet), or
    // |DidFinishNavigation| / |RenderFrameDeleted| noticed that this object has
    // been orphaned.
    return;
  }

  switch (status_code) {
    case device::FidoReturnCode::kUserConsentButCredentialNotRecognized:
      // No authenticators contained the credential.
      InvokeCallbackAndCleanup(
          std::move(get_assertion_response_callback_),
          webauth::mojom::AuthenticatorStatus::CREDENTIAL_NOT_RECOGNIZED,
          nullptr);
      return;
    case device::FidoReturnCode::kAuthenticatorResponseInvalid:
      // The response from the authenticator was corrupted.
      InvokeCallbackAndCleanup(
          std::move(get_assertion_response_callback_),
          webauth::mojom::AuthenticatorStatus::NOT_ALLOWED_ERROR, nullptr);
      return;
    case device::FidoReturnCode::kUserConsentButCredentialExcluded:
      NOTREACHED();
      return;
    case device::FidoReturnCode::kSuccess:
      DCHECK(response_data.has_value());
      InvokeCallbackAndCleanup(
          std::move(get_assertion_response_callback_),
          webauth::mojom::AuthenticatorStatus::SUCCESS,
          CreateGetAssertionResponse(std::move(client_data_json_),
                                     std::move(*response_data),
                                     echo_appid_extension_));
      return;
  }
  NOTREACHED();
}

void AuthenticatorImpl::OnTimeout() {
  // TODO(crbug.com/814418): Add layout tests to verify timeouts are
  // indistinguishable from NOT_ALLOWED_ERROR cases.
  DCHECK(make_credential_response_callback_ ||
         get_assertion_response_callback_);
  if (make_credential_response_callback_) {
    InvokeCallbackAndCleanup(
        std::move(make_credential_response_callback_),
        webauth::mojom::AuthenticatorStatus::NOT_ALLOWED_ERROR, nullptr,
        Focus::kDontCheck);
  } else if (get_assertion_response_callback_) {
    InvokeCallbackAndCleanup(
        std::move(get_assertion_response_callback_),
        webauth::mojom::AuthenticatorStatus::NOT_ALLOWED_ERROR, nullptr);
  }
}

void AuthenticatorImpl::InvokeCallbackAndCleanup(
    MakeCredentialCallback callback,
    webauth::mojom::AuthenticatorStatus status,
    webauth::mojom::MakeCredentialAuthenticatorResponsePtr response,
    Focus check_focus) {
  if (check_focus != Focus::kDontCheck &&
      !GetContentClient()->browser()->IsFocused(
          WebContents::FromRenderFrameHost(render_frame_host_))) {
    std::move(callback).Run(webauth::mojom::AuthenticatorStatus::NOT_FOCUSED,
                            nullptr);
  } else {
    std::move(callback).Run(status, std::move(response));
  }

  Cleanup();
}

void AuthenticatorImpl::InvokeCallbackAndCleanup(
    GetAssertionCallback callback,
    webauth::mojom::AuthenticatorStatus status,
    webauth::mojom::GetAssertionAuthenticatorResponsePtr response) {
  std::move(callback).Run(status, std::move(response));
  Cleanup();
}

void AuthenticatorImpl::Cleanup() {
  timer_->Stop();
  u2f_request_.reset();
  ctap_request_.reset();
  make_credential_response_callback_.Reset();
  get_assertion_response_callback_.Reset();
  client_data_json_.clear();
  echo_appid_extension_ = false;
}

}  // namespace content
