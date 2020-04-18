/*
 * Copyright (C) 2007 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/platform/weborigin/security_origin.h"

#include <stdint.h>
#include <memory>

#include "net/base/url_util.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"
#include "third_party/blink/renderer/platform/weborigin/known_ports.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/weborigin/scheme_registry.h"
#include "third_party/blink/renderer/platform/weborigin/security_policy.h"
#include "third_party/blink/renderer/platform/weborigin/url_security_origin_map.h"
#include "third_party/blink/renderer/platform/wtf/hex_number.h"
#include "third_party/blink/renderer/platform/wtf/not_found.h"
#include "third_party/blink/renderer/platform/wtf/std_lib_extras.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder.h"
#include "third_party/blink/renderer/platform/wtf/text/string_utf8_adaptor.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"
#include "url/url_canon.h"
#include "url/url_canon_ip.h"

namespace blink {

const uint16_t kInvalidPort = 0;

static URLSecurityOriginMap* g_url_origin_map = nullptr;

static SecurityOrigin* GetOriginFromMap(const KURL& url) {
  if (g_url_origin_map)
    return g_url_origin_map->GetOrigin(url);
  return nullptr;
}

bool SecurityOrigin::ShouldUseInnerURL(const KURL& url) {
  // FIXME: Blob URLs don't have inner URLs. Their form is
  // "blob:<inner-origin>/<UUID>", so treating the part after "blob:" as a URL
  // is incorrect.
  if (url.ProtocolIs("blob"))
    return true;
  if (url.ProtocolIs("filesystem"))
    return true;
  return false;
}

// In general, extracting the inner URL varies by scheme. It just so happens
// that all the URL schemes we currently support that use inner URLs for their
// security origin can be parsed using this algorithm.
KURL SecurityOrigin::ExtractInnerURL(const KURL& url) {
  if (url.InnerURL())
    return *url.InnerURL();
  // FIXME: Update this callsite to use the innerURL member function when
  // we finish implementing it.
  return KURL(url.GetPath());
}

void SecurityOrigin::SetMap(URLSecurityOriginMap* map) {
  g_url_origin_map = map;
}

static bool ShouldTreatAsOpaqueOrigin(const KURL& url) {
  if (!url.IsValid())
    return true;

  // FIXME: Do we need to unwrap the URL further?
  KURL relevant_url;
  if (SecurityOrigin::ShouldUseInnerURL(url)) {
    relevant_url = SecurityOrigin::ExtractInnerURL(url);
    if (!relevant_url.IsValid())
      return true;
  } else {
    relevant_url = url;
  }

  // URLs with schemes that require an authority, but which don't have one,
  // will have failed the isValid() test; e.g. valid HTTP URLs must have a
  // host.
  DCHECK(!((relevant_url.ProtocolIsInHTTPFamily() ||
            relevant_url.ProtocolIs("ftp")) &&
           relevant_url.Host().IsEmpty()));

  if (SchemeRegistry::ShouldTreatURLSchemeAsNoAccess(relevant_url.Protocol()))
    return true;

  // This is the common case.
  return false;
}

SecurityOrigin::SecurityOrigin(const KURL& url)
    : protocol_(url.Protocol()),
      host_(url.Host()),
      port_(url.Port()),
      effective_port_(url.Port()),
      is_opaque_(false),
      universal_access_(false),
      domain_was_set_in_dom_(false),
      block_local_access_from_local_origin_(false),
      is_opaque_origin_potentially_trustworthy_(false) {
  if (protocol_.IsNull())
    protocol_ = g_empty_string;
  if (host_.IsNull())
    host_ = g_empty_string;

  if (!effective_port_)
    effective_port_ = DefaultPortForProtocol(protocol_);

  // document.domain starts as m_host, but can be set by the DOM.
  domain_ = host_;

  if (IsDefaultPortForProtocol(port_, protocol_))
    port_ = kInvalidPort;

  // By default, only local SecurityOrigins can load local resources.
  can_load_local_resources_ = IsLocal();
}

SecurityOrigin::SecurityOrigin()
    : protocol_(g_empty_string),
      host_(g_empty_string),
      domain_(g_empty_string),
      port_(kInvalidPort),
      effective_port_(kInvalidPort),
      is_opaque_(true),
      universal_access_(false),
      domain_was_set_in_dom_(false),
      can_load_local_resources_(false),
      block_local_access_from_local_origin_(false),
      is_opaque_origin_potentially_trustworthy_(false) {}

SecurityOrigin::SecurityOrigin(const SecurityOrigin* other)
    : protocol_(other->protocol_.IsolatedCopy()),
      host_(other->host_.IsolatedCopy()),
      domain_(other->domain_.IsolatedCopy()),
      port_(other->port_),
      effective_port_(other->effective_port_),
      is_opaque_(other->is_opaque_),
      universal_access_(other->universal_access_),
      domain_was_set_in_dom_(other->domain_was_set_in_dom_),
      can_load_local_resources_(other->can_load_local_resources_),
      block_local_access_from_local_origin_(
          other->block_local_access_from_local_origin_),
      is_opaque_origin_potentially_trustworthy_(
          other->is_opaque_origin_potentially_trustworthy_) {}

scoped_refptr<SecurityOrigin> SecurityOrigin::Create(const KURL& url) {
  if (scoped_refptr<SecurityOrigin> origin = GetOriginFromMap(url))
    return origin;

  if (ShouldTreatAsOpaqueOrigin(url))
    return base::AdoptRef(new SecurityOrigin());

  if (ShouldUseInnerURL(url))
    return base::AdoptRef(new SecurityOrigin(ExtractInnerURL(url)));

  return base::AdoptRef(new SecurityOrigin(url));
}

scoped_refptr<SecurityOrigin> SecurityOrigin::CreateUniqueOpaque() {
  scoped_refptr<SecurityOrigin> origin = base::AdoptRef(new SecurityOrigin());
  DCHECK(origin->IsOpaque());
  return origin;
}

scoped_refptr<SecurityOrigin> SecurityOrigin::CreateFromUrlOrigin(
    const url::Origin& origin) {
  if (origin.unique())
    return CreateUniqueOpaque();

  DCHECK(String::FromUTF8(origin.scheme().c_str()).ContainsOnlyASCII());
  DCHECK(String::FromUTF8(origin.host().c_str()).ContainsOnlyASCII());

  return Create(String::FromUTF8(origin.scheme().c_str()),
                String::FromUTF8(origin.host().c_str()), origin.port());
}

url::Origin SecurityOrigin::ToUrlOrigin() const {
  return IsOpaque()
             ? url::Origin()
             : url::Origin::CreateFromNormalizedTuple(
                   StringUTF8Adaptor(protocol_).AsStringPiece().as_string(),
                   StringUTF8Adaptor(host_).AsStringPiece().as_string(),
                   effective_port_);
}

scoped_refptr<SecurityOrigin> SecurityOrigin::IsolatedCopy() const {
  return base::AdoptRef(new SecurityOrigin(this));
}

void SecurityOrigin::SetDomainFromDOM(const String& new_domain) {
  domain_was_set_in_dom_ = true;
  domain_ = new_domain;
}

bool SecurityOrigin::IsSecure(const KURL& url) {
  if (SchemeRegistry::ShouldTreatURLSchemeAsSecure(url.Protocol()))
    return true;

  // URLs that wrap inner URLs are secure if those inner URLs are secure.
  if (ShouldUseInnerURL(url) && SchemeRegistry::ShouldTreatURLSchemeAsSecure(
                                    ExtractInnerURL(url).Protocol()))
    return true;

  if (SecurityPolicy::IsUrlWhiteListedTrustworthy(url))
    return true;

  return false;
}

bool SecurityOrigin::SerializesAsNull() const {
  if (IsOpaque())
    return true;

  if (IsLocal() && block_local_access_from_local_origin_)
    return true;

  return false;
}

bool SecurityOrigin::CanAccess(const SecurityOrigin* other) const {
  if (universal_access_)
    return true;

  if (this == other)
    return true;

  if (IsOpaque() || other->IsOpaque())
    return false;

  // document.domain handling, as per
  // https://html.spec.whatwg.org/multipage/browsers.html#dom-document-domain:
  //
  // 1) Neither document has set document.domain. In this case, we insist
  //    that the scheme, host, and port of the URLs match.
  //
  // 2) Both documents have set document.domain. In this case, we insist
  //    that the documents have set document.domain to the same value and
  //    that the scheme of the URLs match. Ports do not need to match.
  bool can_access = false;
  if (protocol_ == other->protocol_) {
    if (!domain_was_set_in_dom_ && !other->domain_was_set_in_dom_) {
      if (host_ == other->host_ && port_ == other->port_)
        can_access = true;
    } else if (domain_was_set_in_dom_ && other->domain_was_set_in_dom_) {
      // TODO(mkwst): If/when we ship this behavior, change this to check
      // IsNull() rather than relying on string comparison.
      // https://crbug.com/733150
      if (domain_ == other->domain_ && domain_ != "null") {
        can_access = true;
      }
    }
  }

  if (can_access && IsLocal())
    can_access = PassesFileCheck(other);

  return can_access;
}

bool SecurityOrigin::PassesFileCheck(const SecurityOrigin* other) const {
  DCHECK(IsLocal());
  DCHECK(other->IsLocal());

  return !block_local_access_from_local_origin_ &&
         !other->block_local_access_from_local_origin_;
}

bool SecurityOrigin::CanRequest(const KURL& url) const {
  if (universal_access_)
    return true;

  if (GetOriginFromMap(url) == this)
    return true;

  if (IsOpaque())
    return false;

  scoped_refptr<const SecurityOrigin> target_origin =
      SecurityOrigin::Create(url);

  if (target_origin->IsOpaque())
    return false;

  // We call isSameSchemeHostPort here instead of canAccess because we want
  // to ignore document.domain effects.
  if (IsSameSchemeHostPort(target_origin.get()))
    return true;

  if (SecurityPolicy::IsAccessWhiteListed(this, target_origin.get()))
    return true;

  return false;
}

bool SecurityOrigin::CanReadContent(const KURL& url) const {
  if (CanRequest(url))
    return true;

  // This function exists because we treat data URLs as having a unique opaque
  // origin, see https://fetch.spec.whatwg.org/#main-fetch.
  // TODO(dcheng): If we plumb around the 'precursor' origin, then maybe we
  // don't need this?
  if (url.ProtocolIsData())
    return true;

  return false;
}

bool SecurityOrigin::CanDisplay(const KURL& url) const {
  if (universal_access_)
    return true;

  String protocol = url.Protocol();
  if (SchemeRegistry::CanDisplayOnlyIfCanRequest(protocol))
    return CanRequest(url);

  if (SchemeRegistry::ShouldTreatURLSchemeAsDisplayIsolated(protocol))
    return protocol_ == protocol ||
           SecurityPolicy::IsAccessToURLWhiteListed(this, url);

  if (SchemeRegistry::ShouldTreatURLSchemeAsLocal(protocol))
    return CanLoadLocalResources() ||
           SecurityPolicy::IsAccessToURLWhiteListed(this, url);

  return true;
}

bool SecurityOrigin::IsPotentiallyTrustworthy() const {
  DCHECK_NE(protocol_, "data");
  if (IsOpaque())
    return is_opaque_origin_potentially_trustworthy_;

  if (SchemeRegistry::ShouldTreatURLSchemeAsSecure(protocol_) || IsLocal() ||
      IsLocalhost())
    return true;

  if (SecurityPolicy::IsOriginWhiteListedTrustworthy(*this))
    return true;

  return false;
}

// static
String SecurityOrigin::IsPotentiallyTrustworthyErrorMessage() {
  return "Only secure origins are allowed (see: https://goo.gl/Y0ZkNV).";
}

void SecurityOrigin::GrantLoadLocalResources() {
  // Granting privileges to some, but not all, documents in a SecurityOrigin
  // is a security hazard because the documents without the privilege can
  // obtain the privilege by injecting script into the documents that have
  // been granted the privilege.
  can_load_local_resources_ = true;
}

void SecurityOrigin::GrantUniversalAccess() {
  universal_access_ = true;
}

void SecurityOrigin::BlockLocalAccessFromLocalOrigin() {
  DCHECK(IsLocal());
  block_local_access_from_local_origin_ = true;
}

bool SecurityOrigin::IsLocal() const {
  return SchemeRegistry::ShouldTreatURLSchemeAsLocal(protocol_);
}

bool SecurityOrigin::IsLocalhost() const {
  // We special-case "[::1]" here because `net::HostStringIsLocalhost` expects a
  // canonicalization that excludes the braces; a simple string comparison is
  // simpler than trying to adjust Blink's canonicalization.
  return host_ == "[::1]" || net::HostStringIsLocalhost(host_.Ascii().data());
}

String SecurityOrigin::ToString() const {
  if (SerializesAsNull())
    return "null";
  return ToRawString();
}

AtomicString SecurityOrigin::ToAtomicString() const {
  if (SerializesAsNull())
    return AtomicString("null");

  if (protocol_ == "file")
    return AtomicString("file://");

  StringBuilder result;
  BuildRawString(result);
  return result.ToAtomicString();
}

String SecurityOrigin::ToRawString() const {
  if (protocol_ == "file")
    return "file://";

  StringBuilder result;
  BuildRawString(result);
  return result.ToString();
}

void SecurityOrigin::BuildRawString(StringBuilder& builder) const {
  builder.Append(protocol_);
  builder.Append("://");
  builder.Append(host_);

  if (port_) {
    builder.Append(':');
    builder.AppendNumber(port_);
  }
}

scoped_refptr<SecurityOrigin> SecurityOrigin::CreateFromString(
    const String& origin_string) {
  return SecurityOrigin::Create(KURL(NullURL(), origin_string));
}

scoped_refptr<SecurityOrigin> SecurityOrigin::Create(const String& protocol,
                                                     const String& host,
                                                     uint16_t port) {
  DCHECK_EQ(host, DecodeURLEscapeSequences(host));

  String port_part = port ? ":" + String::Number(port) : String();
  return Create(KURL(NullURL(), protocol + "://" + host + port_part + "/"));
}

bool SecurityOrigin::IsSameSchemeHostPort(const SecurityOrigin* other) const {
  if (this == other)
    return true;

  if (IsOpaque() || other->IsOpaque())
    return false;

  if (host_ != other->host_)
    return false;

  if (protocol_ != other->protocol_)
    return false;

  if (port_ != other->port_)
    return false;

  if (IsLocal() && !PassesFileCheck(other))
    return false;

  return true;
}

bool SecurityOrigin::AreSameSchemeHostPort(const KURL& a, const KURL& b) {
  scoped_refptr<const SecurityOrigin> origin_a = SecurityOrigin::Create(a);
  scoped_refptr<const SecurityOrigin> origin_b = SecurityOrigin::Create(b);
  return origin_b->IsSameSchemeHostPort(origin_a.get());
}

const KURL& SecurityOrigin::UrlWithUniqueOpaqueOrigin() {
  DCHECK(IsMainThread());
  DEFINE_STATIC_LOCAL(const KURL, url, ("data:,"));
  return url;
}

std::unique_ptr<SecurityOrigin::PrivilegeData>
SecurityOrigin::CreatePrivilegeData() const {
  std::unique_ptr<PrivilegeData> privilege_data =
      std::make_unique<PrivilegeData>();
  privilege_data->universal_access_ = universal_access_;
  privilege_data->can_load_local_resources_ = can_load_local_resources_;
  privilege_data->block_local_access_from_local_origin_ =
      block_local_access_from_local_origin_;
  return privilege_data;
}

void SecurityOrigin::TransferPrivilegesFrom(
    std::unique_ptr<PrivilegeData> privilege_data) {
  universal_access_ = privilege_data->universal_access_;
  can_load_local_resources_ = privilege_data->can_load_local_resources_;
  block_local_access_from_local_origin_ =
      privilege_data->block_local_access_from_local_origin_;
}

void SecurityOrigin::SetOpaqueOriginIsPotentiallyTrustworthy(
    bool is_opaque_origin_potentially_trustworthy) {
  DCHECK(!is_opaque_origin_potentially_trustworthy || IsOpaque());
  is_opaque_origin_potentially_trustworthy_ =
      is_opaque_origin_potentially_trustworthy;
}

String SecurityOrigin::CanonicalizeHost(const String& host, bool* success) {
  url::Component out_host;
  url::RawCanonOutputT<char> canon_output;
  if (host.Is8Bit()) {
    StringUTF8Adaptor utf8(host);
    *success =
        url::CanonicalizeHost(utf8.Data(), url::Component(0, utf8.length()),
                              &canon_output, &out_host);
  } else {
    *success = url::CanonicalizeHost(host.Characters16(),
                                     url::Component(0, host.length()),
                                     &canon_output, &out_host);
  }
  return String::FromUTF8(canon_output.data(), canon_output.length());
}

}  // namespace blink
