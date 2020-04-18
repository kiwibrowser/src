// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/certificate_provider/certificate_provider_api.h"

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <vector>

#include "base/logging.h"
#include "chrome/browser/chromeos/certificate_provider/certificate_provider_service.h"
#include "chrome/browser/chromeos/certificate_provider/certificate_provider_service_factory.h"
#include "chrome/common/extensions/api/certificate_provider.h"
#include "chrome/common/extensions/api/certificate_provider_internal.h"
#include "content/public/common/console_message_level.h"
#include "net/cert/x509_certificate.h"
#include "net/ssl/ssl_private_key.h"
#include "third_party/boringssl/src/include/openssl/ssl.h"

namespace api_cp = extensions::api::certificate_provider;
namespace api_cpi = extensions::api::certificate_provider_internal;

namespace {

chromeos::RequestPinView::RequestPinErrorType GetErrorTypeForView(
    api_cp::PinRequestErrorType error_type) {
  switch (error_type) {
    case api_cp::PinRequestErrorType::PIN_REQUEST_ERROR_TYPE_INVALID_PIN:
      return chromeos::RequestPinView::RequestPinErrorType::INVALID_PIN;
    case api_cp::PinRequestErrorType::PIN_REQUEST_ERROR_TYPE_INVALID_PUK:
      return chromeos::RequestPinView::RequestPinErrorType::INVALID_PUK;
    case api_cp::PinRequestErrorType::
        PIN_REQUEST_ERROR_TYPE_MAX_ATTEMPTS_EXCEEDED:
      return chromeos::RequestPinView::RequestPinErrorType::
          MAX_ATTEMPTS_EXCEEDED;
    case api_cp::PinRequestErrorType::PIN_REQUEST_ERROR_TYPE_UNKNOWN_ERROR:
      return chromeos::RequestPinView::RequestPinErrorType::UNKNOWN_ERROR;
    case api_cp::PinRequestErrorType::PIN_REQUEST_ERROR_TYPE_NONE:
      return chromeos::RequestPinView::RequestPinErrorType::NONE;
  }

  NOTREACHED();
  return chromeos::RequestPinView::RequestPinErrorType::NONE;
}

}  // namespace

namespace extensions {

namespace {

const char kErrorInvalidX509Cert[] =
    "Certificate is not a valid X.509 certificate.";
const char kErrorECDSANotSupported[] = "Key type ECDSA not supported.";
const char kErrorUnknownKeyType[] = "Key type unknown.";
const char kErrorAborted[] = "Request was aborted.";
const char kErrorTimeout[] = "Request timed out, reply rejected.";

// requestPin constants.
const char kNoActiveDialog[] = "No active dialog from extension.";
const char kInvalidId[] = "Invalid signRequestId";
const char kOtherFlowInProgress[] = "Other flow in progress";
const char kPreviousDialogActive[] = "Previous request not finished";
const char kNoUserInput[] = "No user input received";

}  // namespace

const int api::certificate_provider::kMaxClosedDialogsPer10Mins = 2;

CertificateProviderInternalReportCertificatesFunction::
    ~CertificateProviderInternalReportCertificatesFunction() {}

ExtensionFunction::ResponseAction
CertificateProviderInternalReportCertificatesFunction::Run() {
  std::unique_ptr<api_cpi::ReportCertificates::Params> params(
      api_cpi::ReportCertificates::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params);

  chromeos::CertificateProviderService* const service =
      chromeos::CertificateProviderServiceFactory::GetForBrowserContext(
          browser_context());
  DCHECK(service);

  if (!params->certificates) {
    // In the public API, the certificates parameter is mandatory. We only run
    // into this case, if the custom binding rejected the reply by the
    // extension.
    return RespondNow(Error(kErrorAborted));
  }

  chromeos::certificate_provider::CertificateInfoList cert_infos;
  std::vector<std::vector<char>> rejected_certificates;
  for (const api_cp::CertificateInfo& input_cert_info : *params->certificates) {
    chromeos::certificate_provider::CertificateInfo parsed_cert_info;

    if (ParseCertificateInfo(input_cert_info, &parsed_cert_info))
      cert_infos.push_back(parsed_cert_info);
    else
      rejected_certificates.push_back(input_cert_info.certificate);
  }

  if (service->SetCertificatesProvidedByExtension(
          extension_id(), params->request_id, cert_infos)) {
    return RespondNow(ArgumentList(
        api_cpi::ReportCertificates::Results::Create(rejected_certificates)));
  } else {
    // The custom binding already checks for multiple reports to the same
    // request. The only remaining case, why this reply can fail is that the
    // request timed out.
    return RespondNow(Error(kErrorTimeout));
  }
}

bool CertificateProviderInternalReportCertificatesFunction::
    ParseCertificateInfo(
        const api_cp::CertificateInfo& info,
        chromeos::certificate_provider::CertificateInfo* out_info) {
  const std::vector<char>& cert_der = info.certificate;
  if (cert_der.empty()) {
    WriteToConsole(content::CONSOLE_MESSAGE_LEVEL_ERROR, kErrorInvalidX509Cert);
    return false;
  }

  // Allow UTF-8 inside PrintableStrings in client certificates. See
  // crbug.com/770323 and crbug.com/788655.
  net::X509Certificate::UnsafeCreateOptions options;
  options.printable_string_is_utf8 = true;
  out_info->certificate = net::X509Certificate::CreateFromBytesUnsafeOptions(
      cert_der.data(), cert_der.size(), options);
  if (!out_info->certificate) {
    WriteToConsole(content::CONSOLE_MESSAGE_LEVEL_ERROR, kErrorInvalidX509Cert);
    return false;
  }

  size_t public_key_length_in_bits = 0;
  net::X509Certificate::PublicKeyType type =
      net::X509Certificate::kPublicKeyTypeUnknown;
  net::X509Certificate::GetPublicKeyInfo(out_info->certificate->cert_buffer(),
                                         &public_key_length_in_bits, &type);

  switch (type) {
    case net::X509Certificate::kPublicKeyTypeRSA:
      break;
    case net::X509Certificate::kPublicKeyTypeECDSA:
      WriteToConsole(content::CONSOLE_MESSAGE_LEVEL_ERROR,
                     kErrorECDSANotSupported);
      return false;
    default:
      WriteToConsole(content::CONSOLE_MESSAGE_LEVEL_ERROR,
                     kErrorUnknownKeyType);
      return false;
  }

  for (const api_cp::Hash hash : info.supported_hashes) {
    switch (hash) {
      case api_cp::HASH_MD5_SHA1:
        out_info->supported_algorithms.push_back(SSL_SIGN_RSA_PKCS1_MD5_SHA1);
        break;
      case api_cp::HASH_SHA1:
        out_info->supported_algorithms.push_back(SSL_SIGN_RSA_PKCS1_SHA1);
        break;
      case api_cp::HASH_SHA256:
        out_info->supported_algorithms.push_back(SSL_SIGN_RSA_PKCS1_SHA256);
        break;
      case api_cp::HASH_SHA384:
        out_info->supported_algorithms.push_back(SSL_SIGN_RSA_PKCS1_SHA384);
        break;
      case api_cp::HASH_SHA512:
        out_info->supported_algorithms.push_back(SSL_SIGN_RSA_PKCS1_SHA512);
        break;
      case api_cp::HASH_NONE:
        NOTREACHED();
        return false;
    }
  }
  return true;
}

CertificateProviderStopPinRequestFunction::
    ~CertificateProviderStopPinRequestFunction() {}

ExtensionFunction::ResponseAction
CertificateProviderStopPinRequestFunction::Run() {
  std::unique_ptr<api_cp::RequestPin::Params> params(
      api_cp::RequestPin::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params);

  chromeos::CertificateProviderService* const service =
      chromeos::CertificateProviderServiceFactory::GetForBrowserContext(
          browser_context());
  DCHECK(service);
  if (params->details.error_type ==
      api_cp::PinRequestErrorType::PIN_REQUEST_ERROR_TYPE_NONE) {
    bool dialog_closed =
        service->pin_dialog_manager()->CloseDialog(extension_id());
    if (!dialog_closed) {
      // This might happen if the user closed the dialog while extension was
      // processing the input.
      return RespondNow(Error(kNoActiveDialog));
    }

    return RespondNow(NoArguments());
  }

  // Extension provided an error, which means it intends to notify the user with
  // the error and not allow any more input.
  chromeos::RequestPinView::RequestPinErrorType error_type =
      GetErrorTypeForView(params->details.error_type);
  chromeos::PinDialogManager::StopPinRequestResponse update_response =
      service->pin_dialog_manager()->UpdatePinDialog(
          extension()->id(), error_type,
          false,  // Don't accept any input.
          base::Bind(&CertificateProviderStopPinRequestFunction::DialogClosed,
                     this));
  switch (update_response) {
    case chromeos::PinDialogManager::StopPinRequestResponse::NO_ACTIVE_DIALOG:
      return RespondNow(Error(kNoActiveDialog));
    case chromeos::PinDialogManager::StopPinRequestResponse::NO_USER_INPUT:
      return RespondNow(Error(kNoUserInput));
    case chromeos::PinDialogManager::StopPinRequestResponse::STOPPED:
      return RespondLater();
  }

  NOTREACHED();
  return RespondLater();
}

void CertificateProviderStopPinRequestFunction::DialogClosed(
    const base::string16& value) {
  chromeos::CertificateProviderService* const service =
      chromeos::CertificateProviderServiceFactory::GetForBrowserContext(
          browser_context());
  DCHECK(service);

  Respond(NoArguments());
  service->pin_dialog_manager()->OnPinDialogClosed();
}

CertificateProviderRequestPinFunction::
    ~CertificateProviderRequestPinFunction() {}

bool CertificateProviderRequestPinFunction::ShouldSkipQuotaLimiting() const {
  chromeos::CertificateProviderService* const service =
      chromeos::CertificateProviderServiceFactory::GetForBrowserContext(
          browser_context());
  DCHECK(service);

  return !service->pin_dialog_manager()->LastPinDialogClosed(extension_id());
}

void CertificateProviderRequestPinFunction::GetQuotaLimitHeuristics(
    extensions::QuotaLimitHeuristics* heuristics) const {
  QuotaLimitHeuristic::Config short_limit_config = {
      api::certificate_provider::kMaxClosedDialogsPer10Mins,
      base::TimeDelta::FromMinutes(10)};
  heuristics->push_back(std::make_unique<QuotaService::TimedLimit>(
      short_limit_config, new QuotaLimitHeuristic::SingletonBucketMapper(),
      "MAX_PIN_DIALOGS_CLOSED_PER_10_MINUTES"));
}

ExtensionFunction::ResponseAction CertificateProviderRequestPinFunction::Run() {
  std::unique_ptr<api_cp::RequestPin::Params> params(
      api_cp::RequestPin::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params);

  api_cp::PinRequestType pin_request_type =
      params->details.request_type ==
              api_cp::PinRequestType::PIN_REQUEST_TYPE_NONE
          ? api_cp::PinRequestType::PIN_REQUEST_TYPE_PIN
          : params->details.request_type;

  chromeos::RequestPinView::RequestPinErrorType error_type =
      GetErrorTypeForView(params->details.error_type);

  chromeos::RequestPinView::RequestPinCodeType code_type =
      (pin_request_type == api_cp::PinRequestType::PIN_REQUEST_TYPE_PIN)
          ? chromeos::RequestPinView::RequestPinCodeType::PIN
          : chromeos::RequestPinView::RequestPinCodeType::PUK;

  chromeos::CertificateProviderService* const service =
      chromeos::CertificateProviderServiceFactory::GetForBrowserContext(
          browser_context());
  DCHECK(service);

  int attempts_left =
      params->details.attempts_left ? *params->details.attempts_left : -1;
  chromeos::PinDialogManager::RequestPinResponse result =
      service->pin_dialog_manager()->ShowPinDialog(
          extension()->id(), extension()->name(),
          params->details.sign_request_id, code_type, error_type, attempts_left,
          base::Bind(&CertificateProviderRequestPinFunction::OnInputReceived,
                     this));
  switch (result) {
    case chromeos::PinDialogManager::RequestPinResponse::SUCCESS:
      return RespondLater();
    case chromeos::PinDialogManager::RequestPinResponse::INVALID_ID:
      return RespondNow(Error(kInvalidId));
    case chromeos::PinDialogManager::RequestPinResponse::OTHER_FLOW_IN_PROGRESS:
      return RespondNow(Error(kOtherFlowInProgress));
    case chromeos::PinDialogManager::RequestPinResponse::
        DIALOG_DISPLAYED_ALREADY:
      return RespondNow(Error(kPreviousDialogActive));
  }

  NOTREACHED();
  return RespondNow(Error(kPreviousDialogActive));
}

void CertificateProviderRequestPinFunction::OnInputReceived(
    const base::string16& value) {
  std::unique_ptr<base::ListValue> create_results(new base::ListValue());
  chromeos::CertificateProviderService* const service =
      chromeos::CertificateProviderServiceFactory::GetForBrowserContext(
          browser_context());
  DCHECK(service);
  if (!value.empty()) {
    api::certificate_provider::PinResponseDetails details;
    details.user_input.reset(new std::string(value.begin(), value.end()));
    create_results->Append(details.ToValue());
  }

  Respond(ArgumentList(std::move(create_results)));
}

CertificateProviderInternalReportSignatureFunction::
    ~CertificateProviderInternalReportSignatureFunction() {}

ExtensionFunction::ResponseAction
CertificateProviderInternalReportSignatureFunction::Run() {
  std::unique_ptr<api_cpi::ReportSignature::Params> params(
      api_cpi::ReportSignature::Params::Create(*args_));
  EXTENSION_FUNCTION_VALIDATE(params);

  chromeos::CertificateProviderService* const service =
      chromeos::CertificateProviderServiceFactory::GetForBrowserContext(
          browser_context());
  DCHECK(service);

  std::vector<uint8_t> signature;
  // If an error occurred, |signature| will not be set.
  if (params->signature)
    signature.assign(params->signature->begin(), params->signature->end());

  service->ReplyToSignRequest(extension_id(), params->request_id, signature);
  return RespondNow(NoArguments());
}

}  // namespace extensions
