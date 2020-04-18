// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_LOADER_BASE_FETCH_CONTEXT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_LOADER_BASE_FETCH_CONTEXT_H_

#include "base/optional.h"
#include "third_party/blink/public/mojom/net/ip_address_space.mojom-blink.h"
#include "third_party/blink/public/platform/web_url_request.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/frame/csp/content_security_policy.h"
#include "third_party/blink/renderer/core/frame/web_feature_forward.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/loader/fetch/fetch_context.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_request.h"
#include "third_party/blink/renderer/platform/weborigin/referrer_policy.h"

namespace blink {

class ConsoleMessage;
class KURL;
class SecurityOrigin;
class SubresourceFilter;
class WebSocketHandshakeThrottle;

// A core-level implementaiton of FetchContext that does not depend on
// Frame. This class provides basic default implementation for some methods.
class CORE_EXPORT BaseFetchContext : public FetchContext {
 public:
  void AddAdditionalRequestHeaders(ResourceRequest&,
                                   FetchResourceType) override;
  base::Optional<ResourceRequestBlockedReason> CanRequest(
      Resource::Type,
      const ResourceRequest&,
      const KURL&,
      const ResourceLoaderOptions&,
      SecurityViolationReportingPolicy,
      FetchParameters::OriginRestriction,
      ResourceRequest::RedirectStatus) const override;
  base::Optional<ResourceRequestBlockedReason> CheckCSPForRequest(
      WebURLRequest::RequestContext,
      const KURL&,
      const ResourceLoaderOptions&,
      SecurityViolationReportingPolicy,
      ResourceRequest::RedirectStatus) const override;
  base::Optional<ResourceRequestBlockedReason> CheckResponseNosniff(
      WebURLRequest::RequestContext,
      const ResourceResponse&) const override;

  void Trace(blink::Visitor*) override;

  virtual KURL GetSiteForCookies() const = 0;
  virtual SubresourceFilter* GetSubresourceFilter() const = 0;
  virtual void CountUsage(WebFeature) const = 0;
  virtual void CountDeprecation(WebFeature) const = 0;
  virtual bool ShouldBlockWebSocketByMixedContentCheck(const KURL&) const = 0;
  virtual std::unique_ptr<WebSocketHandshakeThrottle>
  CreateWebSocketHandshakeThrottle() = 0;

  void AddWarningConsoleMessage(const String&, LogSource) const override;
  void AddErrorConsoleMessage(const String&, LogSource) const override;
  bool IsAdResource(const KURL&,
                    Resource::Type,
                    WebURLRequest::RequestContext) const override;

 protected:
  // Used for security checks.
  virtual bool AllowScriptFromSource(const KURL&) const = 0;

  // Note: subclasses are expected to override following methods.
  // Used in the default implementation for CanRequest, CanFollowRedirect
  // and AllowResponse.
  virtual bool ShouldBlockRequestByInspector(const KURL&) const = 0;
  virtual void DispatchDidBlockRequest(const ResourceRequest&,
                                       const FetchInitiatorInfo&,
                                       ResourceRequestBlockedReason,
                                       Resource::Type) const = 0;
  virtual bool ShouldBypassMainWorldCSP() const = 0;
  virtual bool IsSVGImageChromeClient() const = 0;
  virtual bool ShouldBlockFetchByMixedContentCheck(
      WebURLRequest::RequestContext,
      network::mojom::RequestContextFrameType,
      ResourceRequest::RedirectStatus,
      const KURL&,
      SecurityViolationReportingPolicy) const = 0;
  virtual bool ShouldBlockFetchAsCredentialedSubresource(const ResourceRequest&,
                                                         const KURL&) const = 0;
  virtual ReferrerPolicy GetReferrerPolicy() const = 0;
  virtual String GetOutgoingReferrer() const = 0;
  virtual const KURL& Url() const = 0;
  virtual const SecurityOrigin* GetParentSecurityOrigin() const = 0;
  virtual base::Optional<mojom::IPAddressSpace> GetAddressSpace() const = 0;
  virtual const ContentSecurityPolicy* GetContentSecurityPolicy() const = 0;

  virtual void AddConsoleMessage(ConsoleMessage*) const = 0;

  // Utility method that can be used to implement other methods.
  void PrintAccessDeniedMessage(const KURL&) const;
  void AddCSPHeaderIfNecessary(Resource::Type, ResourceRequest&);

 private:
  // Utility methods that are used in default implement for CanRequest,
  // CanFollowRedirect and AllowResponse.
  base::Optional<ResourceRequestBlockedReason> CanRequestInternal(
      Resource::Type,
      const ResourceRequest&,
      const KURL&,
      const ResourceLoaderOptions&,
      SecurityViolationReportingPolicy,
      FetchParameters::OriginRestriction,
      ResourceRequest::RedirectStatus) const;

  base::Optional<ResourceRequestBlockedReason> CheckCSPForRequestInternal(
      WebURLRequest::RequestContext,
      const KURL&,
      const ResourceLoaderOptions&,
      SecurityViolationReportingPolicy,
      ResourceRequest::RedirectStatus,
      ContentSecurityPolicy::CheckHeaderType) const;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_LOADER_BASE_FETCH_CONTEXT_H_
