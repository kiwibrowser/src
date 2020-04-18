/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/platform/weborigin/origin_access_entry.h"

#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/web_public_suffix_list.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/weborigin/security_origin.h"
#include "url/third_party/mozilla/url_parse.h"
#include "url/url_canon.h"

namespace blink {

namespace {

// TODO(mkwst): This basically replicates GURL::HostIsIPAddress. If/when
// we re-evaluate everything after merging the Blink and Chromium
// repositories, perhaps we can just use that directly.
bool HostIsIPAddress(const String& host) {
  if (host.IsEmpty())
    return false;

  String protocol("https://");
  KURL url(NullURL(), protocol + host + "/");
  if (!url.IsValid())
    return false;

  url::RawCanonOutputT<char, 128> ignored_output;
  url::CanonHostInfo host_info;
  url::Component host_component(0,
                                static_cast<int>(url.Host().Utf8().length()));
  url::CanonicalizeIPAddress(url.Host().Utf8().data(), host_component,
                             &ignored_output, &host_info);
  return host_info.IsIPAddress();
}

bool IsSubdomainOfHost(const String& subdomain, const String& host) {
  if (subdomain.length() <= host.length())
    return false;

  if (subdomain[subdomain.length() - host.length() - 1] != '.')
    return false;

  if (!subdomain.EndsWith(host))
    return false;

  return true;
}
}  // namespace

OriginAccessEntry::OriginAccessEntry(const String& protocol,
                                     const String& host,
                                     SubdomainSetting subdomain_setting)
    : protocol_(protocol),
      host_(host),
      subdomain_settings_(subdomain_setting),
      host_is_public_suffix_(false) {
  DCHECK(subdomain_setting >= kAllowSubdomains ||
         subdomain_setting <= kDisallowSubdomains);

  host_is_ip_address_ = blink::HostIsIPAddress(host);

  // Look for top-level domains, either with or without an additional dot.
  if (!host_is_ip_address_) {
    WebPublicSuffixList* suffix_list = Platform::Current()->PublicSuffixList();
    if (!suffix_list)
      return;

    size_t public_suffix_length = suffix_list->GetPublicSuffixLength(host_);
    if (host_.length() <= public_suffix_length + 1) {
      host_is_public_suffix_ = true;
    } else if (subdomain_setting == kAllowRegisterableDomains &&
               public_suffix_length) {
      // The "2" in the next line is 1 for the '.', plus a 1-char minimum label
      // length.
      const size_t dot =
          host_.ReverseFind('.', host_.length() - public_suffix_length - 2);
      if (dot == kNotFound)
        registerable_domain_ = host;
      else
        registerable_domain_ = host.Substring(dot + 1);
    }
  }
}

OriginAccessEntry::MatchResult OriginAccessEntry::MatchesOrigin(
    const SecurityOrigin& origin) const {
  if (protocol_ != origin.Protocol())
    return kDoesNotMatchOrigin;

  return MatchesDomain(origin);
}

OriginAccessEntry::MatchResult OriginAccessEntry::MatchesDomain(
    const SecurityOrigin& origin) const {
  // Special case: Include subdomains and empty host means "all hosts, including
  // ip addresses".
  if (subdomain_settings_ != kDisallowSubdomains && host_.IsEmpty())
    return kMatchesOrigin;

  // Exact match.
  if (host_ == origin.Host())
    return kMatchesOrigin;

  // Don't try to do subdomain matching on IP addresses.
  if (host_is_ip_address_)
    return kDoesNotMatchOrigin;

  // Match subdomains.
  switch (subdomain_settings_) {
    case kDisallowSubdomains:
      return kDoesNotMatchOrigin;

    case kAllowSubdomains:
      if (!IsSubdomainOfHost(origin.Host(), host_))
        return kDoesNotMatchOrigin;
      break;

    case kAllowRegisterableDomains:
      // Fall back to a simple subdomain check if no registerable domain could
      // be found:
      if (registerable_domain_.IsEmpty()) {
        if (!IsSubdomainOfHost(origin.Host(), host_))
          return kDoesNotMatchOrigin;
      } else if (registerable_domain_ != origin.Host() &&
                 !IsSubdomainOfHost(origin.Host(), registerable_domain_)) {
        return kDoesNotMatchOrigin;
      }
      break;
  };

  if (host_is_public_suffix_)
    return kMatchesOriginButIsPublicSuffix;

  return kMatchesOrigin;
}

}  // namespace blink
