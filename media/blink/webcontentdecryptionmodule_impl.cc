// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/blink/webcontentdecryptionmodule_impl.h"

#include <utility>

#include "base/bind.h"
#include "base/logging.h"
#include "base/numerics/safe_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "media/base/cdm_context.h"
#include "media/base/cdm_promise.h"
#include "media/base/content_decryption_module.h"
#include "media/base/key_systems.h"
#include "media/blink/cdm_result_promise.h"
#include "media/blink/cdm_session_adapter.h"
#include "media/blink/webcontentdecryptionmodulesession_impl.h"
#include "third_party/blink/public/platform/url_conversion.h"
#include "third_party/blink/public/platform/web_security_origin.h"
#include "third_party/blink/public/platform/web_string.h"
#include "url/origin.h"

namespace media {

namespace {

bool ConvertHdcpVersion(const blink::WebString& hdcp_version_string,
                        HdcpVersion* hdcp_version) {
  if (!hdcp_version_string.ContainsOnlyASCII())
    return false;

  std::string hdcp_version_ascii = hdcp_version_string.Ascii();

  // TODO(xhwang): This implementation assumes exact string match. Update this
  // when we have the string format speced in the spec/registry.
  if (hdcp_version_ascii.empty())
    *hdcp_version = HdcpVersion::kHdcpVersionNone;
  else if (hdcp_version_ascii == "hdcp-1.0")
    *hdcp_version = HdcpVersion::kHdcpVersion1_0;
  else if (hdcp_version_ascii == "hdcp-1.1")
    *hdcp_version = HdcpVersion::kHdcpVersion1_1;
  else if (hdcp_version_ascii == "hdcp-1.2")
    *hdcp_version = HdcpVersion::kHdcpVersion1_2;
  else if (hdcp_version_ascii == "hdcp-1.3")
    *hdcp_version = HdcpVersion::kHdcpVersion1_3;
  else if (hdcp_version_ascii == "hdcp-1.4")
    *hdcp_version = HdcpVersion::kHdcpVersion1_4;
  else if (hdcp_version_ascii == "hdcp-2.0")
    *hdcp_version = HdcpVersion::kHdcpVersion2_0;
  else if (hdcp_version_ascii == "hdcp-2.1")
    *hdcp_version = HdcpVersion::kHdcpVersion2_1;
  else if (hdcp_version_ascii == "hdcp-2.2")
    *hdcp_version = HdcpVersion::kHdcpVersion2_2;
  else
    return false;

  return true;
}

}  // namespace

void WebContentDecryptionModuleImpl::Create(
    media::CdmFactory* cdm_factory,
    const base::string16& key_system,
    const blink::WebSecurityOrigin& security_origin,
    const CdmConfig& cdm_config,
    std::unique_ptr<blink::WebContentDecryptionModuleResult> result) {
  DCHECK(!security_origin.IsNull());
  DCHECK(!key_system.empty());

  // TODO(ddorwin): Guard against this in supported types check and remove this.
  // Chromium only supports ASCII key systems.
  if (!base::IsStringASCII(key_system)) {
    NOTREACHED();
    result->CompleteWithError(
        blink::kWebContentDecryptionModuleExceptionNotSupportedError, 0,
        "Invalid keysystem.");
    return;
  }

  // TODO(ddorwin): This should be a DCHECK.
  std::string key_system_ascii = base::UTF16ToASCII(key_system);
  if (!media::KeySystems::GetInstance()->IsSupportedKeySystem(
          key_system_ascii)) {
    std::string message =
        "Keysystem '" + key_system_ascii + "' is not supported.";
    result->CompleteWithError(
        blink::kWebContentDecryptionModuleExceptionNotSupportedError, 0,
        blink::WebString::FromUTF8(message));
    return;
  }

  // If unique security origin, don't try to create the CDM.
  if (security_origin.IsUnique() || security_origin.ToString() == "null") {
    result->CompleteWithError(
        blink::kWebContentDecryptionModuleExceptionNotSupportedError, 0,
        "EME use is not allowed on unique origins.");
    return;
  }

  // CdmSessionAdapter::CreateCdm() will keep a reference to |adapter|. Then
  // if WebContentDecryptionModuleImpl is successfully created (returned in
  // |result|), it will keep a reference to |adapter|. Otherwise, |adapter| will
  // be destructed.
  scoped_refptr<CdmSessionAdapter> adapter(new CdmSessionAdapter());
  adapter->CreateCdm(cdm_factory, key_system_ascii, security_origin, cdm_config,
                     std::move(result));
}

WebContentDecryptionModuleImpl::WebContentDecryptionModuleImpl(
    scoped_refptr<CdmSessionAdapter> adapter)
    : adapter_(adapter) {
}

WebContentDecryptionModuleImpl::~WebContentDecryptionModuleImpl() = default;

std::unique_ptr<blink::WebContentDecryptionModuleSession>
WebContentDecryptionModuleImpl::CreateSession() {
  return adapter_->CreateSession();
}

void WebContentDecryptionModuleImpl::SetServerCertificate(
    const uint8_t* server_certificate,
    size_t server_certificate_length,
    blink::WebContentDecryptionModuleResult result) {
  DCHECK(server_certificate);
  adapter_->SetServerCertificate(
      std::vector<uint8_t>(server_certificate,
                           server_certificate + server_certificate_length),
      std::unique_ptr<SimpleCdmPromise>(
          new CdmResultPromise<>(result, std::string())));
}

void WebContentDecryptionModuleImpl::GetStatusForPolicy(
    const blink::WebString& min_hdcp_version_string,
    blink::WebContentDecryptionModuleResult result) {
  HdcpVersion min_hdcp_version;
  if (!ConvertHdcpVersion(min_hdcp_version_string, &min_hdcp_version)) {
    result.CompleteWithError(
        blink::kWebContentDecryptionModuleExceptionTypeError, 0,
        "Invalid HDCP version");
    return;
  }

  // TODO(xhwang): Enable UMA reporting for GetStatusForPolicy().
  adapter_->GetStatusForPolicy(
      min_hdcp_version, std::unique_ptr<KeyStatusCdmPromise>(
                            new CdmResultPromise<CdmKeyInformation::KeyStatus>(
                                result, std::string())));
}

std::unique_ptr<CdmContextRef>
WebContentDecryptionModuleImpl::GetCdmContextRef() {
  return adapter_->GetCdmContextRef();
}

}  // namespace media
