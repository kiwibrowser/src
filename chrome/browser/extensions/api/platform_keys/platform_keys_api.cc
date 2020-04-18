// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/platform_keys/platform_keys_api.h"

#include <stddef.h>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/values.h"
#include "chrome/browser/chromeos/platform_keys/platform_keys.h"
#include "chrome/browser/chromeos/platform_keys/platform_keys_service.h"
#include "chrome/browser/chromeos/platform_keys/platform_keys_service_factory.h"
#include "chrome/browser/extensions/api/platform_keys/verify_trust_api.h"
#include "chrome/common/extensions/api/platform_keys_internal.h"
#include "components/web_modal/web_contents_modal_dialog_manager.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/net_errors.h"
#include "net/cert/x509_certificate.h"
#include "net/cert/x509_util.h"

namespace extensions {

namespace api_pk = api::platform_keys;
namespace api_pki = api::platform_keys_internal;

namespace {

const char kErrorAlgorithmNotSupported[] = "Algorithm not supported.";
const char kErrorAlgorithmNotPermittedByCertificate[] =
    "The requested Algorithm is not permitted by the certificate.";
const char kErrorInteractiveCallFromBackground[] =
    "Interactive calls must happen in the context of a browser tab or a "
    "window.";

const char kWebCryptoRSASSA_PKCS1_v1_5[] = "RSASSA-PKCS1-v1_5";

struct PublicKeyInfo {
  // The X.509 Subject Public Key Info of the key in DER encoding.
  std::string public_key_spki_der;

  // The type of the key.
  net::X509Certificate::PublicKeyType key_type =
      net::X509Certificate::kPublicKeyTypeUnknown;

  // The size of the key in bits.
  size_t key_size_bits = 0;
};

// Builds a partial WebCrypto Algorithm object from the parameters available in
// |key_info|, which must the info of an RSA key. This doesn't include sign/hash
// parameters and thus isn't complete.
// platform_keys::GetPublicKey() enforced the public exponent 65537.
void BuildWebCryptoRSAAlgorithmDictionary(const PublicKeyInfo& key_info,
                                          base::DictionaryValue* algorithm) {
  CHECK_EQ(net::X509Certificate::kPublicKeyTypeRSA, key_info.key_type);
  algorithm->SetKey("name", base::Value(kWebCryptoRSASSA_PKCS1_v1_5));
  algorithm->SetKey("modulusLength",
                    base::Value(static_cast<int>(key_info.key_size_bits)));

  // Equals 65537.
  const unsigned char defaultPublicExponent[] = {0x01, 0x00, 0x01};
  algorithm->SetWithoutPathExpansion(
      "publicExponent",
      base::Value::CreateWithCopiedBuffer(
          reinterpret_cast<const char*>(defaultPublicExponent),
          arraysize(defaultPublicExponent)));
}

const struct NameValuePair {
  const char* const name;
  const int value;
} kCertStatusErrors[] = {
#define CERT_STATUS_FLAG(name, value) \
  { #name, value }                    \
  ,
#include "net/cert/cert_status_flags_list.h"
#undef CERT_STATUS_FLAG
};

}  // namespace

namespace platform_keys {

const char kErrorInvalidToken[] = "The token is not valid.";
const char kErrorInvalidX509Cert[] =
    "Certificate is not a valid X.509 certificate.";
const char kTokenIdUser[] = "user";
const char kTokenIdSystem[] = "system";

// Returns whether |token_id| references a known Token.
bool ValidateToken(const std::string& token_id,
                   std::string* platform_keys_token_id) {
  platform_keys_token_id->clear();
  if (token_id == kTokenIdUser) {
    *platform_keys_token_id = chromeos::platform_keys::kTokenIdUser;
    return true;
  }
  if (token_id == kTokenIdSystem) {
    *platform_keys_token_id = chromeos::platform_keys::kTokenIdSystem;
    return true;
  }
  return false;
}

std::string PlatformKeysTokenIdToApiId(
    const std::string& platform_keys_token_id) {
  if (platform_keys_token_id == chromeos::platform_keys::kTokenIdUser)
    return kTokenIdUser;
  if (platform_keys_token_id == chromeos::platform_keys::kTokenIdSystem)
    return kTokenIdSystem;

  return std::string();
}

}  // namespace platform_keys

PlatformKeysInternalGetPublicKeyFunction::
    ~PlatformKeysInternalGetPublicKeyFunction() {
}

ExtensionFunction::ResponseAction
PlatformKeysInternalGetPublicKeyFunction::Run() {
  std::unique_ptr<api_pki::GetPublicKey::Params> params(
      api_pki::GetPublicKey::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params);

  const std::vector<char>& cert_der = params->certificate;
  if (cert_der.empty())
    return RespondNow(Error(platform_keys::kErrorInvalidX509Cert));
  // Allow UTF-8 inside PrintableStrings in client certificates. See
  // crbug.com/770323 and crbug.com/788655.
  net::X509Certificate::UnsafeCreateOptions options;
  options.printable_string_is_utf8 = true;
  scoped_refptr<net::X509Certificate> cert_x509 =
      net::X509Certificate::CreateFromBytesUnsafeOptions(
          cert_der.data(), cert_der.size(), options);
  if (!cert_x509)
    return RespondNow(Error(platform_keys::kErrorInvalidX509Cert));

  PublicKeyInfo key_info;
  key_info.public_key_spki_der =
      chromeos::platform_keys::GetSubjectPublicKeyInfo(cert_x509);
  if (!chromeos::platform_keys::GetPublicKey(cert_x509, &key_info.key_type,
                                             &key_info.key_size_bits) ||
      key_info.key_type != net::X509Certificate::kPublicKeyTypeRSA) {
    return RespondNow(Error(kErrorAlgorithmNotSupported));
  }

  // Currently, the only supported combination is:
  //   A certificate declaring rsaEncryption in the SubjectPublicKeyInfo used
  //   with the RSASSA-PKCS1-v1.5 algorithm.
  if (params->algorithm_name != kWebCryptoRSASSA_PKCS1_v1_5) {
    return RespondNow(Error(kErrorAlgorithmNotPermittedByCertificate));
  }

  api_pki::GetPublicKey::Results::Algorithm algorithm;
  BuildWebCryptoRSAAlgorithmDictionary(key_info,
                                       &algorithm.additional_properties);

  return RespondNow(ArgumentList(api_pki::GetPublicKey::Results::Create(
      std::vector<char>(key_info.public_key_spki_der.begin(),
                        key_info.public_key_spki_der.end()),
      algorithm)));
}

PlatformKeysInternalSelectClientCertificatesFunction::
    ~PlatformKeysInternalSelectClientCertificatesFunction() {
}

ExtensionFunction::ResponseAction
PlatformKeysInternalSelectClientCertificatesFunction::Run() {
  std::unique_ptr<api_pki::SelectClientCertificates::Params> params(
      api_pki::SelectClientCertificates::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params);

  chromeos::PlatformKeysService* service =
      chromeos::PlatformKeysServiceFactory::GetForBrowserContext(
          browser_context());
  DCHECK(service);

  chromeos::platform_keys::ClientCertificateRequest request;
  for (const std::vector<char>& cert_authority :
       params->details.request.certificate_authorities) {
    request.certificate_authorities.push_back(
        std::string(cert_authority.begin(), cert_authority.end()));
  }
  for (const api_pk::ClientCertificateType& cert_type :
       params->details.request.certificate_types) {
    switch (cert_type) {
      case api_pk::CLIENT_CERTIFICATE_TYPE_ECDSASIGN:
        request.certificate_key_types.push_back(
            net::X509Certificate::kPublicKeyTypeECDSA);
        break;
      case api_pk::CLIENT_CERTIFICATE_TYPE_RSASIGN:
        request.certificate_key_types.push_back(
            net::X509Certificate::kPublicKeyTypeRSA);
        break;
      case api_pk::CLIENT_CERTIFICATE_TYPE_NONE:
        NOTREACHED();
    }
  }

  std::unique_ptr<net::CertificateList> client_certs;
  if (params->details.client_certs) {
    client_certs.reset(new net::CertificateList);
    for (const std::vector<char>& client_cert_der :
         *params->details.client_certs) {
      if (client_cert_der.empty())
        return RespondNow(Error(platform_keys::kErrorInvalidX509Cert));
      // Allow UTF-8 inside PrintableStrings in client certificates. See
      // crbug.com/770323 and crbug.com/788655.
      net::X509Certificate::UnsafeCreateOptions options;
      options.printable_string_is_utf8 = true;
      scoped_refptr<net::X509Certificate> client_cert_x509 =
          net::X509Certificate::CreateFromBytesUnsafeOptions(
              client_cert_der.data(), client_cert_der.size(), options);
      if (!client_cert_x509)
        return RespondNow(Error(platform_keys::kErrorInvalidX509Cert));
      client_certs->push_back(client_cert_x509);
    }
  }

  content::WebContents* web_contents = nullptr;
  if (params->details.interactive) {
    web_contents = GetSenderWebContents();

    // Ensure that this function is called in a context that allows opening
    // dialogs.
    if (!web_contents ||
        !web_modal::WebContentsModalDialogManager::FromWebContents(
            web_contents)) {
      return RespondNow(Error(kErrorInteractiveCallFromBackground));
    }
  }

  service->SelectClientCertificates(
      request, std::move(client_certs), params->details.interactive,
      extension_id(),
      base::Bind(&PlatformKeysInternalSelectClientCertificatesFunction::
                     OnSelectedCertificates,
                 this),
      web_contents);
  return RespondLater();
}

void PlatformKeysInternalSelectClientCertificatesFunction::
    OnSelectedCertificates(std::unique_ptr<net::CertificateList> matches,
                           const std::string& error_message) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (!error_message.empty()) {
    Respond(Error(error_message));
    return;
  }
  DCHECK(matches);
  std::vector<api_pk::Match> result_matches;
  for (const scoped_refptr<net::X509Certificate>& match : *matches) {
    PublicKeyInfo key_info;
    key_info.public_key_spki_der =
        chromeos::platform_keys::GetSubjectPublicKeyInfo(match);
    if (!chromeos::platform_keys::GetPublicKey(match, &key_info.key_type,
                                               &key_info.key_size_bits)) {
      LOG(ERROR) << "Could not retrieve public key info.";
      continue;
    }
    if (key_info.key_type != net::X509Certificate::kPublicKeyTypeRSA) {
      LOG(ERROR) << "Skipping unsupported certificate with non-RSA key.";
      continue;
    }

    api_pk::Match result_match;
    base::StringPiece der_encoded_cert =
        net::x509_util::CryptoBufferAsStringPiece(match->cert_buffer());
    result_match.certificate.assign(der_encoded_cert.begin(),
                                    der_encoded_cert.end());

    BuildWebCryptoRSAAlgorithmDictionary(
        key_info, &result_match.key_algorithm.additional_properties);
    result_matches.push_back(std::move(result_match));
  }
  Respond(ArgumentList(
      api_pki::SelectClientCertificates::Results::Create(result_matches)));
}

PlatformKeysInternalSignFunction::~PlatformKeysInternalSignFunction() {
}

ExtensionFunction::ResponseAction PlatformKeysInternalSignFunction::Run() {
  std::unique_ptr<api_pki::Sign::Params> params(
      api_pki::Sign::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params);
  std::string platform_keys_token_id;
  if (!params->token_id.empty() &&
      !platform_keys::ValidateToken(params->token_id,
                                    &platform_keys_token_id)) {
    return RespondNow(Error(platform_keys::kErrorInvalidToken));
  }

  chromeos::PlatformKeysService* service =
      chromeos::PlatformKeysServiceFactory::GetForBrowserContext(
          browser_context());
  DCHECK(service);

  if (params->hash_algorithm_name == "none") {
    service->SignRSAPKCS1Raw(
        platform_keys_token_id,
        std::string(params->data.begin(), params->data.end()),
        std::string(params->public_key.begin(), params->public_key.end()),
        extension_id(),
        base::Bind(&PlatformKeysInternalSignFunction::OnSigned, this));
  } else {
    chromeos::platform_keys::HashAlgorithm hash_algorithm;
    if (params->hash_algorithm_name == "SHA-1") {
      hash_algorithm = chromeos::platform_keys::HASH_ALGORITHM_SHA1;
    } else if (params->hash_algorithm_name == "SHA-256") {
      hash_algorithm = chromeos::platform_keys::HASH_ALGORITHM_SHA256;
    } else if (params->hash_algorithm_name == "SHA-384") {
      hash_algorithm = chromeos::platform_keys::HASH_ALGORITHM_SHA384;
    } else if (params->hash_algorithm_name == "SHA-512") {
      hash_algorithm = chromeos::platform_keys::HASH_ALGORITHM_SHA512;
    } else {
      return RespondNow(Error(kErrorAlgorithmNotSupported));
    }
    service->SignRSAPKCS1Digest(
        platform_keys_token_id,
        std::string(params->data.begin(), params->data.end()),
        std::string(params->public_key.begin(), params->public_key.end()),
        hash_algorithm, extension_id(),
        base::Bind(&PlatformKeysInternalSignFunction::OnSigned, this));
  }

  return RespondLater();
}

void PlatformKeysInternalSignFunction::OnSigned(
    const std::string& signature,
    const std::string& error_message) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (error_message.empty())
    Respond(ArgumentList(api_pki::Sign::Results::Create(
        std::vector<char>(signature.begin(), signature.end()))));
  else
    Respond(Error(error_message));
}

PlatformKeysVerifyTLSServerCertificateFunction::
    ~PlatformKeysVerifyTLSServerCertificateFunction() {
}

ExtensionFunction::ResponseAction
PlatformKeysVerifyTLSServerCertificateFunction::Run() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  std::unique_ptr<api_pk::VerifyTLSServerCertificate::Params> params(
      api_pk::VerifyTLSServerCertificate::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params.get());

  VerifyTrustAPI::GetFactoryInstance()
      ->Get(browser_context())
      ->Verify(std::move(params), extension_id(),
               base::Bind(&PlatformKeysVerifyTLSServerCertificateFunction::
                              FinishedVerification,
                          this));

  return RespondLater();
}

void PlatformKeysVerifyTLSServerCertificateFunction::FinishedVerification(
    const std::string& error,
    int verify_result,
    int cert_status) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (!error.empty()) {
    Respond(Error(error));
    return;
  }

  api_pk::VerificationResult result;
  result.trusted = verify_result == net::OK;
  if (net::IsCertificateError(verify_result)) {
    // Only report errors, not internal informational statuses.
    const int masked_cert_status = cert_status & net::CERT_STATUS_ALL_ERRORS;
    for (size_t i = 0; i < arraysize(kCertStatusErrors); ++i) {
      if ((masked_cert_status & kCertStatusErrors[i].value) ==
          kCertStatusErrors[i].value) {
        result.debug_errors.push_back(kCertStatusErrors[i].name);
      }
    }
  }

  Respond(ArgumentList(
      api_pk::VerifyTLSServerCertificate::Results::Create(result)));
}

}  // namespace extensions
