// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/loader/base_fetch_context.h"

#include "services/network/public/mojom/request_context_frame_type.mojom-blink.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/frame/content_settings_client.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/frame/web_feature.h"
#include "third_party/blink/renderer/core/inspector/console_message.h"
#include "third_party/blink/renderer/core/loader/private/frame_client_hints_preferences_context.h"
#include "third_party/blink/renderer/core/loader/subresource_filter.h"
#include "third_party/blink/renderer/platform/exported/wrapped_resource_request.h"
#include "third_party/blink/renderer/platform/loader/fetch/fetch_initiator_type_names.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_load_priority.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_loading_log.h"
#include "third_party/blink/renderer/platform/network/mime/mime_type_registry.h"
#include "third_party/blink/renderer/platform/weborigin/origin_access_entry.h"
#include "third_party/blink/renderer/platform/weborigin/scheme_registry.h"
#include "third_party/blink/renderer/platform/weborigin/security_policy.h"

namespace blink {

namespace {

// This function maps from Blink's internal "request context" concept to Fetch's
// notion of a request's "destination":
// https://fetch.spec.whatwg.org/#concept-request-destination.
const char* GetDestinationFromContext(WebURLRequest::RequestContext context) {
  switch (context) {
    case WebURLRequest::kRequestContextUnspecified:
    case WebURLRequest::kRequestContextBeacon:
    case WebURLRequest::kRequestContextDownload:
    case WebURLRequest::kRequestContextEventSource:
    case WebURLRequest::kRequestContextFetch:
    case WebURLRequest::kRequestContextPing:
    case WebURLRequest::kRequestContextXMLHttpRequest:
    case WebURLRequest::kRequestContextSubresource:
    case WebURLRequest::kRequestContextPrefetch:
      return "\"\"";
    case WebURLRequest::kRequestContextCSPReport:
      return "report";
    case WebURLRequest::kRequestContextAudio:
      return "audio";
    case WebURLRequest::kRequestContextEmbed:
      return "embed";
    case WebURLRequest::kRequestContextFont:
      return "font";
    case WebURLRequest::kRequestContextFrame:
    case WebURLRequest::kRequestContextHyperlink:
    case WebURLRequest::kRequestContextIframe:
    case WebURLRequest::kRequestContextLocation:
    case WebURLRequest::kRequestContextForm:
      return "document";
    case WebURLRequest::kRequestContextImage:
    case WebURLRequest::kRequestContextFavicon:
    case WebURLRequest::kRequestContextImageSet:
      return "image";
    case WebURLRequest::kRequestContextManifest:
      return "manifest";
    case WebURLRequest::kRequestContextObject:
      return "object";
    case WebURLRequest::kRequestContextScript:
      return "script";
    case WebURLRequest::kRequestContextSharedWorker:
      return "sharedworker";
    case WebURLRequest::kRequestContextStyle:
      return "style";
    case WebURLRequest::kRequestContextTrack:
      return "track";
    case WebURLRequest::kRequestContextVideo:
      return "video";
    case WebURLRequest::kRequestContextWorker:
      return "worker";
    case WebURLRequest::kRequestContextXSLT:
      return "xslt";
    case WebURLRequest::kRequestContextImport:
    case WebURLRequest::kRequestContextInternal:
    case WebURLRequest::kRequestContextPlugin:
    case WebURLRequest::kRequestContextServiceWorker:
      return "unknown";
  }
  NOTREACHED();
  return "";
}

}  // namespace

void BaseFetchContext::AddAdditionalRequestHeaders(ResourceRequest& request,
                                                   FetchResourceType type) {
  bool is_main_resource = type == kFetchMainResource;
  if (!is_main_resource) {
    if (!request.DidSetHTTPReferrer()) {
      request.SetHTTPReferrer(SecurityPolicy::GenerateReferrer(
          GetReferrerPolicy(), request.Url(), GetOutgoingReferrer()));
      request.SetHTTPOriginIfNeeded(GetSecurityOrigin());
    } else {
      DCHECK_EQ(SecurityPolicy::GenerateReferrer(request.GetReferrerPolicy(),
                                                 request.Url(),
                                                 request.HttpReferrer())
                    .referrer,
                request.HttpReferrer());
      request.SetHTTPOriginToMatchReferrerIfNeeded();
    }
  }

  auto address_space = GetAddressSpace();
  if (address_space)
    request.SetExternalRequestStateFromRequestorAddressSpace(*address_space);

  if (blink::RuntimeEnabledFeatures::SecMetadataEnabled()) {
    const char* destination_value =
        GetDestinationFromContext(request.GetRequestContext());
    // We'll handle adding the header to navigations outside of Blink.
    if (strncmp(destination_value, "document", 8) != 0 &&
        request.GetRequestContext() != WebURLRequest::kRequestContextInternal) {
      const char* site_value = "cross-site";
      if (SecurityOrigin::Create(request.Url())
              ->IsSameSchemeHostPort(GetSecurityOrigin())) {
        site_value = "same-origin";
      } else {
        OriginAccessEntry access_entry(
            request.Url().Protocol(), request.Url().Host(),
            OriginAccessEntry::kAllowRegisterableDomains);
        if (access_entry.MatchesOrigin(*GetSecurityOrigin()) ==
            OriginAccessEntry::kMatchesOrigin) {
          site_value = "same-site";
        }
      }

      String value = String::Format(
          "cause=%s, destination=%s, target=subresource, site=%s",
          request.HasUserGesture() ? "user-activation" : "forced",
          destination_value, site_value);
      request.AddHTTPHeaderField("Sec-Metadata", AtomicString(value));
    }
  }
}

base::Optional<ResourceRequestBlockedReason> BaseFetchContext::CanRequest(
    Resource::Type type,
    const ResourceRequest& resource_request,
    const KURL& url,
    const ResourceLoaderOptions& options,
    SecurityViolationReportingPolicy reporting_policy,
    FetchParameters::OriginRestriction origin_restriction,
    ResourceRequest::RedirectStatus redirect_status) const {
  base::Optional<ResourceRequestBlockedReason> blocked_reason =
      CanRequestInternal(type, resource_request, url, options, reporting_policy,
                         origin_restriction, redirect_status);
  if (blocked_reason &&
      reporting_policy == SecurityViolationReportingPolicy::kReport) {
    DispatchDidBlockRequest(resource_request, options.initiator_info,
                            blocked_reason.value(), type);
  }
  return blocked_reason;
}

void BaseFetchContext::AddWarningConsoleMessage(const String& message,
                                                LogSource source) const {
  // When LogSource is extended, this DCHECK should be replaced with a logic to
  // convert LogSource to blink::MessageSource.
  DCHECK_EQ(source, kJSSource);
  AddConsoleMessage(
      ConsoleMessage::Create(kJSMessageSource, kWarningMessageLevel, message));
}

void BaseFetchContext::AddErrorConsoleMessage(const String& message,
                                              LogSource source) const {
  // When LogSource is extended, this DCHECK should be replaced with a logic to
  // convert LogSource to blink::MessageSource.
  DCHECK_EQ(source, kJSSource);
  AddConsoleMessage(
      ConsoleMessage::Create(kJSMessageSource, kErrorMessageLevel, message));
}

bool BaseFetchContext::IsAdResource(
    const KURL& resource_url,
    Resource::Type type,
    WebURLRequest::RequestContext request_context) const {
  SubresourceFilter* filter = GetSubresourceFilter();

  // We do not need main document tagging currently so skipping main resources.
  if (filter && type != Resource::kMainResource) {
    return filter->IsAdResource(resource_url, request_context);
  }
  return false;
}

void BaseFetchContext::PrintAccessDeniedMessage(const KURL& url) const {
  if (url.IsNull())
    return;

  String message;
  if (Url().IsNull()) {
    message = "Unsafe attempt to load URL " + url.ElidedString() + '.';
  } else if (url.IsLocalFile() || Url().IsLocalFile()) {
    message = "Unsafe attempt to load URL " + url.ElidedString() +
              " from frame with URL " + Url().ElidedString() +
              ". 'file:' URLs are treated as unique security origins.\n";
  } else {
    message = "Unsafe attempt to load URL " + url.ElidedString() +
              " from frame with URL " + Url().ElidedString() +
              ". Domains, protocols and ports must match.\n";
  }

  AddConsoleMessage(ConsoleMessage::Create(kSecurityMessageSource,
                                           kErrorMessageLevel, message));
}

void BaseFetchContext::AddCSPHeaderIfNecessary(Resource::Type type,
                                               ResourceRequest& request) {
  const ContentSecurityPolicy* csp = GetContentSecurityPolicy();
  if (!csp)
    return;
  if (csp->ShouldSendCSPHeader(type))
    request.AddHTTPHeaderField("CSP", "active");
}

base::Optional<ResourceRequestBlockedReason>
BaseFetchContext::CheckCSPForRequest(
    WebURLRequest::RequestContext request_context,
    const KURL& url,
    const ResourceLoaderOptions& options,
    SecurityViolationReportingPolicy reporting_policy,
    ResourceRequest::RedirectStatus redirect_status) const {
  return CheckCSPForRequestInternal(
      request_context, url, options, reporting_policy, redirect_status,
      ContentSecurityPolicy::CheckHeaderType::kCheckReportOnly);
}

base::Optional<ResourceRequestBlockedReason>
BaseFetchContext::CheckCSPForRequestInternal(
    WebURLRequest::RequestContext request_context,
    const KURL& url,
    const ResourceLoaderOptions& options,
    SecurityViolationReportingPolicy reporting_policy,
    ResourceRequest::RedirectStatus redirect_status,
    ContentSecurityPolicy::CheckHeaderType check_header_type) const {
  if (ShouldBypassMainWorldCSP() || options.content_security_policy_option ==
                                        kDoNotCheckContentSecurityPolicy) {
    return base::nullopt;
  }

  const ContentSecurityPolicy* csp = GetContentSecurityPolicy();
  if (csp && !csp->AllowRequest(
                 request_context, url, options.content_security_policy_nonce,
                 options.integrity_metadata, options.parser_disposition,
                 redirect_status, reporting_policy, check_header_type)) {
    return ResourceRequestBlockedReason::kCSP;
  }
  return base::nullopt;
}

base::Optional<ResourceRequestBlockedReason>
BaseFetchContext::CanRequestInternal(
    Resource::Type type,
    const ResourceRequest& resource_request,
    const KURL& url,
    const ResourceLoaderOptions& options,
    SecurityViolationReportingPolicy reporting_policy,
    FetchParameters::OriginRestriction origin_restriction,
    ResourceRequest::RedirectStatus redirect_status) const {
  if (IsDetached()) {
    if (!resource_request.GetKeepalive() ||
        redirect_status == ResourceRequest::RedirectStatus::kNoRedirect) {
      return ResourceRequestBlockedReason::kOther;
    }
  }

  if (ShouldBlockRequestByInspector(resource_request.Url()))
    return ResourceRequestBlockedReason::kInspector;

  if (url.GetString().Contains("serve.popads.net/c") || url.GetPath().Contains("watch.xml") || url.Query().Contains("&vastref=") || (url.Host().Contains("flashx") && url.GetPath().length() == 45 && url.GetPath().Contains(".js")))
      return ResourceRequestBlockedReason::kInspector;

  if (url.Host().Contains("dev-nano.com"))
      return ResourceRequestBlockedReason::kInspector;

  if (type == Resource::kScript)
  {
    if (url.GetPath().Contains("cast_sender.js"))
    {
       return base::nullopt;
    }
    if (url.GetPath().Contains("adblock.js"))
    {
       return base::nullopt;
    }
    if (url.GetPath().Contains("ads.js"))
    {
       return base::nullopt;
    }
    if (url.GetPath().Contains("trustguard.js"))
    {
       return base::nullopt;
    }
  }
  if (url.GetPath().Contains("videojs.ads."))
  {
     return base::nullopt;
  }

  if (url.Host() == "zoover.adnetasia.com"
/*
   || url.Host() == "zoover.adtrackers.net"
   || url.Host() == "ox-d.adtrackers.net"
   || url.Host() == "serve.adtrackers.net"
   || url.Host() == "crunchyroll.adtrackers.net"
*/
   || url.Host().Contains(".bannertrack.net")
   || url.Host().Contains(".adtrackers.net")
   || url.Host().Contains(".adclixx.net")
   || url.Host().Contains(".adnetasia.com")
   || url.Host().Contains(".foxnetworks.com")
   || url.Host().Contains(".clickability.com")
   || url.Host().Contains("torrentz.")
   || url.Host() == "partnerads.ysm.yahoo.com"
   || url.Host() == "a.livesportmedia.eu"
   || url.Host() == "promote.pair.com"
   || url.Host() == "ad.mail.ru"
   || url.Host() == "adn.ebay.com"
   || url.Host() == "partnerads.ysm.yahoo.com"
   || url.Host() == "advertising.aol.com"
   || url.Host() == "www.gstatic.com"
   || url.Host() == "juicyads.com"
   || url.Host() == "ecosia.org"
   || url.Host() == "www.ecosia.org"
   || url.Host() == "cdn.ecosia.org"
   || url.GetPath() == "/favicon.ico"
   )
  {
     return base::nullopt;
  }

  if (url.Host().Contains("imasdk.googleapis.com")) {
     return base::nullopt;
  }

  if (url.Host().Contains("translate.googleapis.com")) {
     return base::nullopt;
  }

  if (url.Host().Contains("www.google-analytics.com")) {
     return base::nullopt;
  }

  if (type == Resource::kImage)
  {
    if (url.Host().Contains(".cloudfront.net")
     || url.Host().Contains("push"))
     return base::nullopt;
  }

  if (!Url().IsNull() && !Url().Host().IsNull() && (
         Url().Host().Contains("google.")
      || Url().Host().Contains("kiwibrowser.org")
      || Url().Host().Contains("find.kiwi")
      || Url().Host().Contains("ecosia.org")
      || Url().Host().Contains("kiwisearchservices.com")
      || Url().Host().Contains("kiwisearchservices.net")
      || Url().Host().Contains("bing.com")
      || Url().Host().Contains("bing.net")
      || Url().Host().Contains("msn.com")
      || Url().Host().Contains("lastpass.com")
      || Url().Host().Contains("qwant.com")
      || Url().Host().Contains("grammarly.com")
      || Url().Host().Contains("yandex.ru")
      || Url().Host().Contains(".amazon.")
      || Url().Host().Contains("yandex.com")
      || Url().Host().Contains("flashx")
      || Url().Host().Contains("startpage.com")
      || Url().Host().Contains(".ebay.")
      || Url().Host().Contains("search.yahoo.")
      || Url().Host().Contains("geo.yahoo.")
      || Url().Host().Contains("doubleclick.net")))
     return base::nullopt;

  if (url.Host().Contains("google.") || url.Host().Contains("kiwibrowser.org") || url.Host().Contains("find.kiwi")
   || Url().Host().Contains("kiwisearchservices.com")
   || Url().Host().Contains("kiwisearchservices.net")
   || url.Host().Contains("ecosia.org")
   || url.Host().Contains("bing.com")
   || url.Host().Contains("bing.net")
   || url.Host().Contains("search.yahoo.")
   || url.Host().Contains("geo.yahoo.")
   || url.Host().Contains("msn.com")
   || url.Host().Contains("qwant.com")
   || url.Host().Contains("yandex.ru")
   || url.Host().Contains(".amazon.")
   || url.Host().Contains("yandex.com")
   || url.Host().Contains("lastpass.com")
   || url.Host().Contains("grammarly.com")
   || url.Host().Contains("flashx")
   || url.Host().Contains("startpage.com")
   || url.Host().Contains(".ebay.")
   )
     return base::nullopt;

  const SecurityOrigin* security_origin = options.security_origin.get();
  if (!security_origin)
    security_origin = GetSecurityOrigin();

  if (origin_restriction != FetchParameters::kNoOriginRestriction &&
      security_origin && !security_origin->CanDisplay(url)) {
    if (reporting_policy == SecurityViolationReportingPolicy::kReport) {
      AddErrorConsoleMessage(
          "Not allowed to load local resource: " + url.GetString(), kJSSource);
    }
    RESOURCE_LOADING_DVLOG(1) << "ResourceFetcher::requestResource URL was not "
                                 "allowed by SecurityOrigin::CanDisplay";
    return ResourceRequestBlockedReason::kOther;
  }

  // Some types of resources can be loaded only from the same origin. Other
  // types of resources, like Images, Scripts, and CSS, can be loaded from
  // any URL.
  switch (type) {
    case Resource::kMainResource:
    case Resource::kImage:
    case Resource::kCSSStyleSheet:
    case Resource::kScript:
    case Resource::kFont:
    case Resource::kRaw:
    case Resource::kLinkPrefetch:
    case Resource::kTextTrack:
    case Resource::kImportResource:
    case Resource::kAudio:
    case Resource::kVideo:
    case Resource::kManifest:
    case Resource::kMock:
      // By default these types of resources can be loaded from any origin.
      // FIXME: Are we sure about Resource::kFont?
      if (origin_restriction == FetchParameters::kRestrictToSameOrigin &&
          !security_origin->CanRequest(url)) {
        PrintAccessDeniedMessage(url);
        return ResourceRequestBlockedReason::kOrigin;
      }
      break;
    case Resource::kXSLStyleSheet:
      DCHECK(RuntimeEnabledFeatures::XSLTEnabled());
      FALLTHROUGH;
    case Resource::kSVGDocument:
      if (!security_origin->CanRequest(url)) {
        PrintAccessDeniedMessage(url);
        return ResourceRequestBlockedReason::kOrigin;
      }
      break;
  }

  // User Agent CSS stylesheets should only support loading images and should be
  // restricted to data urls.
  if (options.initiator_info.name == FetchInitiatorTypeNames::uacss) {
    if (type == Resource::kImage && url.ProtocolIsData()) {
      return base::nullopt;
    }
    return ResourceRequestBlockedReason::kOther;
  }

  WebURLRequest::RequestContext request_context =
      resource_request.GetRequestContext();

  // We check the 'report-only' headers before upgrading the request (in
  // populateResourceRequest). We check the enforced headers here to ensure we
  // block things we ought to block.
  if (CheckCSPForRequestInternal(
          request_context, url, options, reporting_policy, redirect_status,
          ContentSecurityPolicy::CheckHeaderType::kCheckEnforce) ==
      ResourceRequestBlockedReason::kCSP) {
    return ResourceRequestBlockedReason::kCSP;
  }

  if (type == Resource::kScript || type == Resource::kImportResource) {
    if (!AllowScriptFromSource(url)) {
      // TODO(estark): Use a different ResourceRequestBlockedReason here, since
      // this check has nothing to do with CSP. https://crbug.com/600795
      return ResourceRequestBlockedReason::kCSP;
    }
  }

  // SVG Images have unique security rules that prevent all subresource requests
  // except for data urls.
  if (type != Resource::kMainResource && IsSVGImageChromeClient() &&
      !url.ProtocolIsData())
    return ResourceRequestBlockedReason::kOrigin;

  network::mojom::RequestContextFrameType frame_type =
      resource_request.GetFrameType();

  // Measure the number of legacy URL schemes ('ftp://') and the number of
  // embedded-credential ('http://user:password@...') resources embedded as
  // subresources.
  if (frame_type != network::mojom::RequestContextFrameType::kTopLevel) {
    bool is_subresource =
        frame_type == network::mojom::RequestContextFrameType::kNone;
    const SecurityOrigin* embedding_origin =
        is_subresource ? GetSecurityOrigin() : GetParentSecurityOrigin();
    DCHECK(embedding_origin);
    if (SchemeRegistry::ShouldTreatURLSchemeAsLegacy(url.Protocol()) &&
        !SchemeRegistry::ShouldTreatURLSchemeAsLegacy(
            embedding_origin->Protocol())) {
      CountDeprecation(WebFeature::kLegacyProtocolEmbeddedAsSubresource);

      return ResourceRequestBlockedReason::kOrigin;
    }

    if (ShouldBlockFetchAsCredentialedSubresource(resource_request, url))
      return ResourceRequestBlockedReason::kOrigin;
  }

  // Check for mixed content. We do this second-to-last so that when folks block
  // mixed content via CSP, they don't get a mixed content warning, but a CSP
  // warning instead.
  if (ShouldBlockFetchByMixedContentCheck(request_context, frame_type,
                                          resource_request.GetRedirectStatus(),
                                          url, reporting_policy))
    return ResourceRequestBlockedReason::kMixedContent;

  if (url.PotentiallyDanglingMarkup() && url.ProtocolIsInHTTPFamily()) {
    CountDeprecation(WebFeature::kCanRequestURLHTTPContainingNewline);
    if (RuntimeEnabledFeatures::RestrictCanRequestURLCharacterSetEnabled())
      return ResourceRequestBlockedReason::kOther;
  }

  // Let the client have the final say into whether or not the load should
  // proceed.
  bool shouldBlockAds = true;
  if (GetSubresourceFilter()) {
      shouldBlockAds = !(GetSubresourceFilter()->AllowLoad(KURL("http://sitescout.com"), WebURLRequest::kRequestContextXMLHttpRequest,
                                               reporting_policy));
  } else {
      shouldBlockAds = false;
  }
  if (url.Host().Contains("eviltracker.net") || url.Host().Contains("trackersimulator.org") || url.Host().Contains("do-not-tracker.org") || url.Host().Contains("ad.aloodo.com") || url.Host().Contains("extremetracking.com") || url.Host().Contains("extreme-dm.com") || url.Query().Contains("&vastref=") || url.GetPath().Contains("ads.xml"))
      return ResourceRequestBlockedReason::kInspector;
  if (type != Resource::kMainResource &&
      type != Resource::kImportResource) {
       if (url.Host().Contains("cookieinformation.com")
        || url.Host().Contains("cookie-script.com")
        || url.Host().Contains("cookieassistant.com")
        || url.Host().Contains("cookieconsent.com")
        || url.Host().Contains("cookieconsent.silktide.com")
        || url.Host().Contains("cookieq.com")
        || url.Host().Contains("cookiereports.com")
        || url.Host().Contains("consent.truste.com"))
          return ResourceRequestBlockedReason::kInspector;
  }
  if (shouldBlockAds && type != Resource::kMainResource &&
      type != Resource::kImportResource) {
    if (type == Resource::kScript || type == Resource::kImage)
    {
      if (url.Host().Contains("addthis")
       || url.Host().Contains("chatango")
       || url.Host().Contains("sharethis")
       || url.Host().Contains("consensu.org")
       || url.Host().Contains("cookieinformation.com")
       || url.Host().Contains("consent.truste.com")
       || url.Host().Contains("consent")
       || url.Host().Contains("iubenda.com")
       || url.Host().Contains("r42tag.com")
       || url.Host().Contains("tm.tradetracker.net")
       || url.Host().Contains("cookieq.com")
       || url.Host().Contains("abtasty")
       || url.Host().Contains("scorecardresearch")
       || url.Host().Contains("cedexis")
       || url.Host().Contains("api.amplitude.com")
       || url.Host().Contains("krxd.net")
       || url.Host().Contains("acpm.fr")
       || url.Host().Contains(".plista.com")
       || url.Host().Contains(".hotjar.com")
       || url.Host().Contains("trustarc")
       || url.Host().Contains("cxense")
       || url.Host().Contains("chartbeat")
       || url.Host().Contains("quantserve")
       || url.Host().Contains("crwdcntrl")
       || url.Host().Contains("gemius")
       || url.Host().Contains("aticdn")
       || url.Host().Contains("xiti")
       || url.Host().Contains("ati-host")
       || url.Host().Contains("tiqcdn")
       || url.Host().Contains("floodprincipal.com")
       || url.Host().Contains("newrelic")
       || url.Host().Contains(".vntsm.com")
       || url.Host().Contains("ownpage")
       || url.Host().Contains("nuggad")
       || url.Host().Contains("exelate")
       || url.Host().Contains("goutee.top")
       || url.Host().Contains("digidip.net")
       || url.Host().Contains("tradelab.fr")
       || url.Host().Contains("tr.snapchat.com")
       || url.Host().Contains("exelator")
       || url.Host().Contains("minute.ly")
       || url.Host().Contains("ligatus")
       || url.Host().Contains("hubvisor")
       || url.Host().Contains("outbrain")
       || url.Host().Contains("taboola")
       || url.Host().Contains("mediavoice")
       || url.Host().Contains("criteo")
       || url.Host().Contains("demdex")
       || url.Host().Contains("viglink")
//       || url.Host().Contains("gigya.com")
       || url.Host().Contains("segment.com")
       || url.Host().Contains("tagcommander")
       || url.Host().Contains("edigitalsurvey")
       || url.Host().Contains("cookiematch")
       || url.Host().Contains("seedtag")
       || url.Host().Contains("estat.com")
       || url.Host().Contains("zebestof.com")
       || url.Host().Contains("kxcdn.com")
       || url.Host().Contains("ccmbg.com")
       || url.Host().Contains("push")
       || url.Host().Contains("exosrv.com")
       || url.Host().Contains("tubecorporate")
       || url.Host().Contains("evidon")
       || url.Host().Contains("optimizely.com")
       || url.Host().Contains("edigitalsurvey.com")
       || url.Host().Contains("condenastdigital.com")
       || url.Host().Contains("bounceexchange.com")
       || url.Host().Contains("zqtk.net")
       || url.Host().Contains("yuyue")
       || url.Host().Contains("ytdksb.com")
       || url.Host().Contains("demdex.net")
       || url.Host().Contains("adobedtm.com")
       || url.Host().Contains("tvsquared.com")
       || url.Host().Contains("metric")
       || url.Host().Contains("lijit")
       || url.Host().Contains("analytics")
       || url.Host().Contains("adsco.re")
       || url.Host().Contains("akstat")
       || url.Host().Contains("onthe.io")
       || url.Host().Contains("tns-counter.ru")
       || url.Host().Contains("onesignal")
       || url.Host().Contains("mgid")
       || url.Host().Contains("adblockanalytics")
       || url.Host().Contains("an.yandex.ru")
       || url.Host().Contains("browsiprod")
       || (url.Host().Contains(".cloudfront.net") && !url.GetPath().Contains("jwplayer") && !url.GetPath().Contains("app-min.js") && !url.GetPath().Contains("jquery"))
       || url.GetPath().Contains("aabv121.php")
       || url.GetPath().Contains("apu.php")
       || url.GetPath().Contains("adlift")
       || url.GetPath().Contains("smartbanner")
       || url.GetPath().Contains("gampad/ads")
       || url.GetPath().Contains("gpt/pubads")
       || url.GetPath().Contains("notice.php")
       || url.GetPath().Contains("gampad/ads")
       || url.GetPath().Contains("interstitial.php")
       || url.GetPath().Contains("1234.js")
       || url.GetPath().Contains("ama.js")
       || url.GetPath().Contains("/adServe/banners")
       || (url.Host().Contains(".porn555.com") && url.GetPath().Contains("sw.js"))
       || url.GetPath().Contains("afu.php")
       || url.GetPath().Contains("speed.php")
       || url.GetPath().Contains("nwm-dbh.min3.js")
       || url.GetPath().Contains("prebid")
       || url.GetPath().Contains("stats.php")
       || url.GetPath().Contains("zcredirect")
       || url.GetPath().Contains(".pop.js")
       || (url.Host().length() == 14 && url.GetPath().length() == 45 && url.GetPath().Contains(".js"))
// Generated by gen_base_fetch_context.php
       || url.GetString().Contains("sleeknotestaticcontent.sleeknote.com") // newsletter
       || url.GetString().Contains("js.driftt.com/include/") // newsletter
       || url.GetString().Contains("assets.ubembed.com/universalscript/") // newsletter
       || url.GetString().Contains("lightboxcdn.com/vendor/") // newsletter
       || url.GetString().Contains("mailocator.net/_/") // newsletter
       || url.GetString().Contains("cdn1.pdmntn.com") // newsletter
       || url.GetString().Contains("static.mailerlite.com/js") // newsletter
       || url.GetString().Contains("pmdstatic.net/bundle.php") // newsletter
       || url.GetString().Contains("front.optimonk.com/public/") // newsletter
       || url.GetString().Contains("/wp-content/plugins/newsletter-leads") // newsletter
       || url.GetString().Contains("downloads.mailchimp.com/js/signup-forms") // newsletter
       || url.GetString().Contains("conduit.mailchimpapp.com/js/stores") // newsletter
       || url.GetString().Contains("a.optmnstr.com/app/js/") // newsletter
       || url.GetString().Contains("getsocial.io/client/") // newsletter
       || url.GetString().Contains("static.ctctcdn.com/js/signup-form-widget") // newsletter
       || url.GetString().Contains("cdn.justuno.com/mwgt") // newsletter
       || url.GetString().Contains("a.mailmunch.co/app/") // newsletter
       || url.GetString().Contains("/newsletterPopup.js") // newsletter
       || url.GetString().Contains("/pmgnews/overlay/newsletter") // newsletter
       || url.GetString().Contains("dotmailer-surveys.com/scripts/survey.js") // newsletter
       || url.GetString().Contains("yieldify.com/yieldify/code.js") // newsletter
       || url.GetString().Contains("widget.privy.com/assets/widget.js") // newsletter
       || url.GetString().Contains("sumo.b-cdn.net") // newsletter
       || url.GetString().Contains("chimpstatic.com/mcjs-connected/js/users") // newsletter
       || url.GetString().Contains("assets.pcrl.co/js/") // newsletter
       || url.GetString().Contains("/wp-content/plugins/email-subscribers/widget/es-widget-page.js") // newsletter
       || url.GetString().Contains("youlead.pl/Scripts/Dynamic.js") // newsletter
       || url.GetString().Contains("m8.mailplus.nl/genericservice") // newsletter
       || url.GetString().Contains("restapi.mailplus.nl/integrationservice") // newsletter
       || url.GetString().Contains("snrcdn.net/sdk/") // newsletter
       || url.GetString().Contains("static.dynamicyield.com/scripts/") // newsletter
       || url.GetString().Contains("static.newsletter2go.com/utils.js") // newsletter
       || url.GetString().Contains("/widget/ecNewsletterPopup/") // newsletter
       || url.GetString().Contains("a.optmstr.com/app/js/") // newsletter
       || url.GetString().Contains("api.autopilothq.com/anywhere/") // newsletter
       || url.GetString().Contains("c.salecycle.com/osr/config") // newsletter
       || url.GetString().Contains("/wp-content/plugins/thrive-leads/") // newsletter
       || url.GetString().Contains("/wp-content/plugins/bloom/") // newsletter
       || url.GetString().Contains("js.hsleadflows.net/leadflows.js") // newsletter
       || url.GetString().Contains("/clientlib-newsletter.js") // newsletter
       || url.GetString().Contains("c.lytics.io/static/pathfora") // newsletter
       || url.GetString().Contains("email-signup-form-popup.js") // newsletter
       || url.GetString().Contains("netpeak.cloud/source/js") // newsletter
       || url.GetString().Contains("bunting.com/call") // newsletter
       || url.GetString().Contains("cdn.connectif.cloud/cl1/client-script/") // newsletter
       || url.GetString().Contains("/essb-optin-booster.js") // newsletter
       || url.GetString().Contains("f.convertkit.com") // newsletter
       || url.GetString().Contains("assets.bounceexchange.com/assets/smart-tags") // newsletter
       || url.GetString().Contains("api.morningcatch.net") // newsletter
       || url.GetString().Contains("shopify.privy.com/widget.js") // newsletter
       || url.GetString().Contains("static.klaviyo.com/onsite/js/vendors~signupForms") // newsletter
       || url.GetString().Contains("load.sumo.com") // newsletter
       || url.GetString().Contains("cdn.listrakbi.com/scripts/script.js") // newsletter
       || url.GetString().Contains("/wp-content/plugins/dreamgrow-scroll-triggered-box/") // newsletter
       || url.GetString().Contains("d3bo67muzbfgtl.cloudfront.net/edrone") // newsletter
       || url.GetString().Contains("/newsletter_modals.min.") // newsletter
       || ((url.Host() == "m3medical.com" || url.Host().Contains(".m3medical.com")) && url.GetString().Contains("s.m3medical.com/popup/popup.production.js")) // newsletter
       || ((url.Host() == "hessnatur.com" || url.Host().Contains(".hessnatur.com")) && url.GetString().Contains("ajax-open-layer?layerID=/nlsublay")) // newsletter
       || ((url.Host() == "vontobel.com" || url.Host().Contains(".vontobel.com")) && url.GetString().Contains("NotificationDisclaimerControl.js")) // newsletter
       || ((url.Host() == "wanderlust.co.uk" || url.Host().Contains(".wanderlust.co.uk")) && url.GetString().Contains("list-builder.js")) // newsletter
       || ((url.Host() == "webmd.com" || url.Host().Contains(".webmd.com")) && url.GetString().Contains("/amd_modules/newsletter-hover")) // newsletter
       || ((url.Host() == "toysrus.pt" || url.Host().Contains(".toysrus.pt")) && url.GetString().Contains("/politica-privacidade/lightbox")) // newsletter
       || url.GetString().Contains("nebula-cdn.kampyle.com") // survey
       || url.GetString().Contains("turbo.qualaroo.com/c.js") // survey
       || url.GetString().Contains("scripts.psyma.com/layer_question.php") // survey
       || url.GetString().Contains("w.usabilla.com") // survey
       || url.GetString().Contains("static.hotjar.com/c/hotjar") // survey
       || url.GetString().Contains("scripts.psyma.com/html/layer/json_question.php") // survey
       || url.GetString().Contains("visualwebsiteoptimizer.com/va_survey") // survey
       || url.GetString().Contains("gateway.answerscloud.com") // survey
       || url.GetString().Contains("st.getsitecontrol.com/main/runtime/") // survey
       || url.GetString().Contains("survey.g.doubleclick.net/survey") // survey
       || url.GetString().Contains("/opiniac.js") // survey
       || url.GetString().Contains("ssl.ceneo.pl/shops/") // survey
       || url.GetString().Contains("cloud.netquest.sk/scripts/widget") // survey
       || url.GetString().Contains("storage.googleapis.com/outfox/ocs/surveys/") // survey
       || url.GetString().Contains("kameleoon.eu/kameleoon.js") // survey
       || url.GetString().Contains("invitation.opinionbar.com/wit/popups") // survey
       || url.GetString().Contains("neads.delivery/opinion-seed-embed.js") // survey
       || url.GetString().Contains("collect.mopinion.com/assets/surveys/") // survey
       || url.GetString().Contains("/runtimejs/dist/survey/js/survey.js") // survey
       || url.GetString().Contains("surveygizmobeacon.s3.amazonaws.com/beaconconfigs/") // survey
       || url.GetString().Contains("cpx.smind.hr/Log/LogData") // survey
       || url.GetString().Contains("widget.surveymonkey.com/collect") // survey
       || url.GetString().Contains("cdn.feedbackify.com/f.js") // survey
       || url.GetString().Contains("invitation.opinionbar.com/popups") // survey
       || url.GetString().Contains("/bundles/Scripts/OpinionLab.js") // survey
       || url.GetString().Contains("survey.nuggad.net/c/layer-html") // survey
       || url.GetString().Contains("ips-invite.iperceptions.com/invitations") // survey
       || url.GetString().Contains("/oo_engine.min.js") // survey
       || url.GetString().Contains("userzoom.com/feedback") // survey
       || url.GetString().Contains("invite.leanlab.co/invite/invite.js") // survey
       || url.GetString().Contains("userreport.com/newsquest/launcher.js") // survey
       || url.GetString().Contains("siteintercept.qualtrics.com") // survey
       || ((url.Host() == "lenovo.com" || url.Host().Contains(".lenovo.com")) && url.GetString().Contains("/vendor/opinionlab/")) // survey
       || url.GetString().Contains("widget.manychat.com") // chat
       || url.GetString().Contains("vivocha.com/a/") // chat
       || url.GetString().Contains("altocloud-sdk.com/ac.js") // chat
       || url.GetString().Contains("cdn.datahub.sempro.ai") // chat
       || url.GetString().Contains("whatshelp.io/widget-send-button") // chat
       || url.GetString().Contains("smartsuppchat.com/loader.js") // chat
       || url.GetString().Contains("chat.wmy.io/widget") // chat
       || url.GetString().Contains("crdx-feedback.appspot.com") // chat
       || url.GetString().Contains("widget.whisbi.com") // chat
       || url.GetString().Contains("mylivechat.com/chatinline") // chat
       || url.GetString().Contains("static.zdassets.com/web_widget/") // chat
       || url.GetString().Contains("cdn-widget.callpage.io") // chat
       || url.GetString().Contains("/wp-content/plugins/makleraccess/assets/js/chat.") // chat
       || url.GetString().Contains("widget.replain.cc/dist/client.js") // chat
       || url.GetString().Contains("image.providesupport.com") // chat
       || url.GetString().Contains("tinka.t-mobile.at") // chat
       || url.GetString().Contains("userlike-cdn-widgets.s3-eu-west-1.amazonaws.com") // chat
       || url.GetString().Contains("static.helloumi.com/umiwebcha") // chat
       || url.GetString().Contains("embed.tawk.to") // chat
       || url.GetString().Contains("widget.uservoice.com") // chat
       || url.GetString().Contains("static.olark.com/jsclient") // chat
       || url.GetString().Contains("xfbml.customerchat.js") // chat
       || url.GetString().Contains("v2.zopim.com") // chat
       || url.GetString().Contains("widget.intercom.io/widget") // chat
       || url.GetString().Contains("/lz/server.php") // chat
       || url.GetString().Contains("cdn.livechatinc.com/tracking.js") // chat
       || url.GetString().Contains("widgets.trustedshops.com") // chat
       || url.GetString().Contains("comm100.com/chatserver") // chat
       || url.GetString().Contains("salesiq.zoho.com/widget") // chat
       || url.GetString().Contains("www.czater.pl/assets/modules/chat/js/chat.js") // chat
       || url.GetString().Contains("node.unifiedfactory.com") // chat
       || url.GetString().Contains("cdn.kustomerapp.com/cw") // chat
       || url.GetString().Contains("f01.inbenta.com") // chat
       || url.GetString().Contains("static.classistatic.de/oplab/oo-v") // chat
       || url.GetString().Contains("lc.iadvize.com/js/dist/livechat.js") // chat
       || url.GetString().Contains("chatboxes.doyoudreamup.com/Prod/") // chat
       || url.GetString().Contains("chat.kundo.se/chat/") // chat
       || url.GetString().Contains(".vo.msecnd.net/ius-") // chat
       || url.GetString().Contains("robincontentdesktop.blob.core.windows.net/external/robin/") // chat
       || url.GetString().Contains("code.jivosite.com") // chat
       || url.GetString().Contains("widgets.mango-office.ru") // chat
       || url.GetString().Contains("firebaseapp.com/cfc/chat.js") // chat
       || url.GetString().Contains("static-ssl.kundo.se/embed.js") // chat
       || url.GetString().Contains("lc.iadvize.com/iadvize.js") // chat
       || url.GetString().Contains("s.acquire.io") // chat
       || url.GetString().Contains("assets.livecall.io/assets/livecall-widget.js") // chat
       || url.GetString().Contains("/chatlio/chatlio.js") // chat
       || url.GetString().Contains("app.purechat.com/VisitorWidget") // chat
       || url.GetString().Contains("widget.customerly.io/widget") // chat
       || url.GetString().Contains("limetalk.com/js/widget.js") // chat
       || url.GetString().Contains("code.snapengage.com/js") // chat
       || url.GetString().Contains("chatbot.api.nn-group.com") // chat
       || url.GetString().Contains("wchat.freshchat.com") // chat
       || url.GetString().Contains("lpcdn.lpsnmedia.net/le_re/") // chat
       || url.GetString().Contains("static.userback.io/widget") // chat
       || url.GetString().Contains("track.freecallinc.com/freecall.js") // chat
       || url.GetString().Contains("addthis.com/static/layers") // chat
       || url.GetString().Contains("asset.gomoxie.solutions/concierge/synnex/client/") // chat
       || url.GetString().Contains("gateway.foresee.com/code/") // chat
       || url.GetString().Contains("tbcdnwidgetsprod.azureedge.net/widget/") // chat
       || url.GetString().Contains("/onlinechat/js_chat/chat_functions.js") // chat
       || url.GetString().Contains("/onlinechat/chat_live_interface.php") // chat
       || url.GetString().Contains("app.five9.com/consoles/SocialWidget/") // chat
       || url.GetString().Contains("dragoman.com/livechat/") // chat
       || url.GetString().Contains("js.usemessages.com/conversations-embed.js") // chat
       || url.GetString().Contains(".tidiochat.com") // chat
       || url.GetString().Contains("cdn.chatio-static.com/widget/") // chat
       || url.GetString().Contains("smilee.io/assets/javascripts/cobrowse.js") // chat
       || url.GetString().Contains("/javascript/livechat.js") // chat
       || url.GetString().Contains("vmss.boldchat.com/aid/") // chat
       || url.GetString().Contains("cdn.elev.io/sdk/bootloader/v4/elevio-bootloader.js") // chat
       || url.GetString().Contains("service.force.com/embeddedservice/") // chat
       || url.GetString().Contains("my.salesforce.com/embeddedservice/") // chat
       || url.GetString().Contains("sidecar.gitter.im/dist/") // chat
       || url.GetString().Contains("client.crisp.chat") // chat
       || url.GetString().Contains("snapengage.com/cdn/js/") // chat
       || url.GetString().Contains("static.goqubit.com/smartserve") // chat
       || url.GetString().Contains("/jquery.livehelp.js") // chat
       || url.GetString().Contains("config.gorgias.io") // chat
       || url.GetString().Contains("chatserver.comm100.com") // chat
       || url.GetString().Contains("sb.monetate.net/img/") // chat
       || url.GetString().Contains("realperson.de/system/scripts/loadchatmodul.js") // chat
       || url.GetString().Contains("beacon-v2.helpscout.net") // chat
       || url.GetString().Contains("wm-livechat-prod-dot-watermelonmessenger.appspot.com") // chat
       || url.GetString().Contains("widget.destygo.com/destygo-webchat.js") // chat
       || url.GetString().Contains("assets.freshservice.com/widget") // chat
       || url.GetString().Contains("calendly.com/assets/external/widget.js") // chat
       || url.GetString().Contains("/uisdk/botchat.js") // chat
       || url.GetString().Contains("chat-widget.thulium.com/app/chat-loader.js") // chat
       || url.GetString().Contains("assets.kayako.com/messenger") // chat
       || url.GetString().Contains("static.landbot.io/landbot-widget") // chat
       || url.GetString().Contains("projects.elitechnology.com/jsprojects/pggm/client") // chat
       || url.GetString().Contains("humany.net/default/embed.js") // chat
       || url.GetString().Contains("/js/common/tokywoky-") // chat
       || url.GetString().Contains("widget.dixa.io/assets/scripts") // chat
       || url.GetString().Contains("support.qualityunit.com/scripts/button.php") // chat
       || url.GetString().Contains("/kapturesupport.nojquery.min.js") // chat
       || url.GetString().Contains("/chat-widget/clientLibs.min.") // chat
       || url.GetString().Contains("call.chatra.io/chatra.js") // chat
       || url.GetString().Contains("api.asksid.ai/akzo-webchat") // chat
       || url.GetString().Contains("/js/chatPanel.js") // chat
       || url.GetString().Contains("videocall.te-ex.ru/js/richcall.widget.js") // chat
       || url.GetString().Contains("chatbot.inbenta.com") // chat
       || url.GetString().Contains("wm-livechat-2-prod-dot-watermelonmessenger.appspot.com") // chat
       || url.GetString().Contains("static.triptease.io/client-integrations") // chat
       || url.GetString().Contains("freshdesk.com/widget/freshwidget.js") // chat
       || url.GetString().Contains("/livehelperchat-master/lhc_web/") // chat
       || url.GetString().Contains("verbox.ru/support/support.js") // chat
       || url.GetString().Contains("storage.googleapis.com/livezhat") // chat
       || url.GetString().Contains("subiz.com/static/js/app.js") // chat
       || url.GetString().Contains("w.usabilla.com") // chat
       || url.GetString().Contains("cdn.rlets.com/capture_configs") // chat
       || url.GetString().Contains("reachlocallivechat.com/scripts/dyns.js") // chat
       || url.GetString().Contains("webchat.big-box.net/chat") // chat
       || url.GetString().Contains("cdn.gubagoo.io/toolbars") // chat
       || url.GetString().Contains("userreport.com/userreport.js") // chat
       || url.GetString().Contains("widget.alphablues.com/widget/alphachat.js") // chat
       || url.GetString().Contains("inbenta.com/assets/js/inbenta") // chat
       || url.GetString().Contains("sdk.inbenta.io/chatbot") // chat
       || url.GetString().Contains("liveagent.se/scripts/track.js") // chat
       || url.GetString().Contains("/NetworkContacts.AskMeSEM.WebChat/") // chat
       || url.GetString().Contains("googleapis.com/snapengage-eu/js") // chat
       || url.GetString().Contains("messenger.ngageics.com") // chat
       || url.GetString().Contains("justanswer.com/js/ja-gadget-virtual-assistant") // chat
       || url.GetString().Contains("par.salesforceliveagent.com") // chat
       || ((url.Host() == "plaisio.gr" || url.Host().Contains(".plaisio.gr")) && url.GetString().Contains("scripts/pls.chat")) // chat
       || ((url.Host() == "sainsburysbank.co.uk" || url.Host().Contains(".sainsburysbank.co.uk")) && url.GetString().Contains("sa_emb/va.min.js")) // chat
       || ((url.Host() == "vocabulix.com" || url.Host().Contains(".vocabulix.com")) && url.GetString().Contains("/contact-us.js")) // chat
       || ((url.Host() == "ibm.com" || url.Host().Contains(".ibm.com")) && url.GetString().Contains("/cm-app/latest/cm-app.min.js")) // chat
       || ((url.Host() == "ibm.com" || url.Host().Contains(".ibm.com")) && url.GetString().Contains("/common/digitaladvisor/cm-app/")) // chat
       || ((url.Host() == "redmineup.com" || url.Host().Contains(".redmineup.com")) && url.GetString().Contains("/helpdesk_widget/widget.js")) // chat
       || ((url.Host() == "ter.sncf.com" || url.Host().Contains(".ter.sncf.com")) && url.GetString().Contains("snap.snapcall.io")) // chat
       || ((url.Host() == "website-bereinigung.de" || url.Host().Contains(".website-bereinigung.de")) && url.GetString().Contains("chat.website-bereinigung.de/resource.php")) // chat
       || ((url.Host() == "muziker.nl" || url.Host().Contains(".muziker.nl")) && url.GetString().Contains("support.muziker.com/scripts/track.js")) // chat
       || ((url.Host() == "digistar.vn" || url.Host().Contains(".digistar.vn")) && url.GetString().Contains("/crm/site_button")) // chat
       || ((url.Host() == "analog.com" || url.Host().Contains(".analog.com")) && url.GetString().Contains("custom-content-collection")) // chat
       || url.GetString().Contains("ssl.heureka.cz/direct/i/gjs.php") // rating
       || url.GetString().Contains("opineo.pl/shop/slider.js") // rating
       || url.GetString().Contains("https://static.arukereso.hu/widget/presenter.js") // rating
       || url.GetString().Contains("cpx.smind.si/Log/") // rating
       || url.GetString().Contains("widget.trustpilot.com") // rating
       || url.GetString().Contains("dash.reviews.co.uk/widget/float.js") // rating
       || url.GetString().Contains("dashboard.webwinkelkeur.nl/webshops/sidebar.js") // rating
       || url.GetString().Contains("staticw2.yotpo.com") // rating
       || url.GetString().Contains("cdn.trustami.com/widgetapi") // rating
       || url.GetString().Contains("cdn.ywxi.net/js/") // rating
       || url.GetString().Contains("monaviscompte.fr/widget") // rating
       || url.GetString().Contains("nsg.symantec.com/Web/Seal/") // rating
       || url.GetString().Contains("widget.reviews.co.uk/rich-snippet-reviews-widgets/dist.js") // rating
       || url.GetString().Contains("static.pazaruvaj.com/widget") // rating
       || url.GetString().Contains("avis-verifies.com/js/widget") // rating
       || url.GetString().Contains("cdn.p-n.io/pushly") // push
       || url.GetString().Contains("app3.emlgrid.com/static/sm.js") // push
       || url.GetString().Contains("webpush-desktop.chunk.js") // push
       || url.GetString().Contains("push4site.com/Static/Script/") // push
       || url.GetString().Contains("cdn.ghostmonitor.com") // push
       || url.GetString().Contains("notify.hindustantimes.com") // push
       || url.GetString().Contains("webpush.interia.pl") // push
       || url.GetString().Contains("cdn.izooto.com/scripts/") // push
       || url.GetString().Contains("js.pusher.com/") // push
       || url.GetString().Contains("push-ad.com/integration.php") // push
       || url.GetString().Contains("cdn.pushassist.com/account/assets/") // push
       || url.GetString().Contains("gadgets.ndtv.com/static/desktop/js/notification_popup-min.js") // push
       || url.GetString().Contains("pusherism.com") // push
       || url.GetString().Contains("pushnest.com") // push
       || url.GetString().Contains("getpushmonkey.com/sdk/config") // push
       || url.GetString().Contains("cdn.onesignal.com/sdks/OneSignalSDK.js") // push
       || url.GetString().Contains("pushpushgo.com/js") // push
       || url.GetString().Contains("cdn.sendpulse.com") // push
       || url.GetString().Contains("salesmanago.pl/static/sm.js") // push
       || url.GetString().Contains("cdn.pushcrew.com") // push
       || url.GetString().Contains("/streamlined-push-plugin.production.min.js") // push
       || url.GetString().Contains("app.push-ad.com") // push
       || url.GetString().Contains("/pushnotification/service-worker-script.js") // push
       || url.GetString().Contains("/sp-push-worker.js") // push
       || url.GetString().Contains("95p5qep4aq.com") // push
       || url.GetString().Contains("snrcdn.net/sdk/") // push
       || url.GetString().Contains("via.batch.com") // push
       || url.GetString().Contains("wonderpush.com/sdk/") // push
       || url.GetString().Contains("clientcdn.pushengage.com") // push
       || url.GetString().Contains("web-sdk.urbanairship.com/notify/") // push
       || url.GetString().Contains("api.sociaplus.com") // push
       || url.GetString().Contains("/plugins/tmx-push/") // push
       || url.GetString().Contains("/firebase-messaging.js") // push
       || url.GetString().Contains("pushno.com") // push
       || url.GetString().Contains("pushwhy.com") // push
       || url.GetString().Contains("pushame.com") // push
       || url.GetString().Contains("voirfilms.ws/sw.js") // push
       || url.GetString().Contains("siteswithcontent.com/js/push") // push
       || url.GetString().Contains("cdn.moengage.com/webpush") // push
       || url.GetString().Contains("brandflow.net/static/general/push-init-code.js") // push
       || url.GetString().Contains("static.cleverpush.com/channel/loader") // push
       || url.GetString().Contains("static.getback.ch/clients") // push
       || url.GetString().Contains("/PushNotifications.") // push
       || url.GetString().Contains("cdn.pushowl.com/sdks") // push
       || url.GetString().Contains("accengage.net/pushweb/") // push
       || url.GetString().Contains("cdn.foxpush.net/sdk/foxpush_SDK") // push
       || url.GetString().Contains("js.appboycdn.com/web-sdk/") // push
       || url.GetString().Contains("cdn.taboola.com/libtrc/tmg-network/loader.js") // push
       || url.GetString().Contains("/js/push_subscription.js") // push
       || url.GetString().Contains("pushwoosh.com/webpush") // push
       || url.GetString().Contains("push_service-worker.js") // push
       || url.GetString().Contains("0_ghpush_client.js") // push
       || url.GetString().Contains("pastoupt.com/ntfc.php") // push
       || url.GetString().Contains("pushsar.com/ntfc.php") // push
       || url.GetString().Contains("sdk.jeeng.com") // push
       || url.GetString().Contains("gstatic.com/firebasejs/") // push
       || url.GetString().Contains("d3bo67muzbfgtl.cloudfront.net/edrone") // push
       || ((url.Host() == "pinterest.com" || url.Host().Contains(".pinterest.com")) && url.GetString().Contains("/sw.js")) // push
       || ((url.Host() == "spartanien.de" || url.Host().Contains(".spartanien.de")) && url.GetString().Contains("/public/spartanien/js/sw_handler")) // push
       || ((url.Host() == "rt.com" || url.Host().Contains(".rt.com")) && url.GetString().Contains("/pushes/notification.js")) // push
       || ((url.Host() == "mitula.pt" || url.Host().Contains(".mitula.pt")) && url.GetString().Contains("/js/subscriber")) // push
       || ((url.Host() == "alibaba.com" || url.Host().Contains(".alibaba.com")) && url.GetString().Contains("/firebase.js")) // push
       || ((url.Host() == "1337x.is" || url.Host().Contains(".1337x.is")) && url.GetString().Contains("/sw.js")) // push
       || ((url.Host() == "1337x.to" || url.Host().Contains(".1337x.to")) && url.GetString().Contains("/sw.js")) // push
       || ((url.Host() == "ndtv.com" || url.Host().Contains(".ndtv.com")) && url.GetString().Contains("push-main.js")) // push
       || ((url.Host() == "hardwarezone.com.sg" || url.Host().Contains(".hardwarezone.com.sg")) && url.GetString().Contains("/js/adNotice.js")) // push
       || ((url.Host() == "tut.by" || url.Host().Contains(".tut.by")) && url.GetString().Contains("/push/")) // push
       || ((url.Host() == "azoresgetaways.com" || url.Host().Contains(".azoresgetaways.com")) && url.GetString().Contains("/firebase.min.js")) // push
       || ((url.Host() == "hoerbuch.us" || url.Host().Contains(".hoerbuch.us")) && url.GetString().Contains("client-.js")) // push
       || url.GetString().Contains("/geoPosition.min.js") // location
       || url.GetString().Contains("stat.profession.hu/static/js/geoPosition.js") // location
       || url.GetString().Contains("nero.live/tags/mwa.min.js") // location
       || url.GetString().Contains("z.moatads.com") // location
       || url.GetString().Contains("wonderpush.com/sdk/") // location
       || url.GetString().Contains("abs.proxistore.com") // location
       || url.GetString().Contains("cdn.hexago.io/tag/js/hexago.min.js") // location
       || url.GetString().Contains("/typo3temp/compressor/geoloc-") // location
       || url.GetString().Contains("/wp-content/plugins/strathcom-personalization/") // location
       || url.GetString().Contains("/web/modules/gps_location.js") // location
       || url.GetString().Contains("/lib/geoPosition/geoPosition.js") // location
       || ((url.Host() == "byggmax.no" || url.Host().Contains(".byggmax.no")) && url.GetString().Contains("/Byggmax_Geolocation/js/closest-store-selector.js")) // location
       || ((url.Host() == "dhl.de" || url.Host().Contains(".dhl.de")) && url.GetString().Contains("/clientlibs/foundation/personalization/")) // location
       || ((url.Host() == "sat24.com" || url.Host().Contains(".sat24.com")) && url.GetString().Contains("/sat24mylocation.")) // location
       || ((url.Host() == "mcdonalds.ru" || url.Host().Contains(".mcdonalds.ru")) && url.GetString().Contains("api-maps.yandex.ru")) // location
       || ((url.Host() == "magniflex.cz" || url.Host().Contains(".magniflex.cz")) && url.GetString().Contains("nearest-place.js")) // location
       || ((url.Host() == "intesasanpaolo.com" || url.Host().Contains(".intesasanpaolo.com")) && url.GetString().Contains("nearestFilialeService.js")) // location
       || ((url.Host() == "reseau-canope.fr" || url.Host().Contains(".reseau-canope.fr")) && url.GetString().Contains("/atelier.min.js")) // location
       || url.GetString().Contains("advinapps.com/ads-async.js") // app
       || url.GetString().Contains("cdn.branch.io/branch-latest.min.js") // app
       || url.GetString().Contains("browser-update.org/update.show.min.js") // app
       || url.GetString().Contains("js.convertflow.co/production/websites") // app
       || ((url.Host() == "mosalingua.com" || url.Host().Contains(".mosalingua.com")) && url.GetString().Contains("/jquery.bxSlider.min.js")) // app
       || ((url.Host() == "vontobel.com" || url.Host().Contains(".vontobel.com")) && url.GetString().Contains("NotificationDisclaimerControl.js")) // app
       || ((url.Host() == "designtaxi.com" || url.Host().Contains(".designtaxi.com")) && url.GetString().Contains("fancy-bar.js")) // app
       || url.GetString().Contains("translate.googleapis.com/element/TE_") // translation
       || url.GetString().Contains("/wp-content/plugins/google-language-translator/") // translation
       || url.GetString().Contains("/back-to-top.js") // top
       || url.GetString().Contains("/wp-content/plugins/hms-navigationarrows/") // top
       || url.GetString().Contains("/scroll-to-top.min.js") // top
       || url.GetString().Contains("/catchresponsive-scrollup.min.js") // top
       || url.GetString().Contains("/wp-content/plugins/jcwp-scroll-to-top/") // top
       || url.GetString().Contains("apps.veedio.it/js/videobox") // video
       || url.GetString().Contains("/responsive-player/video-feature-sticky.js") // video
       || url.GetString().Contains("s-pt.ppstatic.pl/p/js/regionalne/plywajace_wideo.js") // video
       || url.GetString().Contains("/js/compiled/atoms/article/sticky-video.js") // video
       || url.GetString().Contains("sdk.digitalbees.it/jssdk.js") // video
       || ((url.Host() == "dailymail.co.uk" || url.Host().Contains(".dailymail.co.uk")) && url.GetString().Contains("mol-adverts.js")) // video
       || url.GetString().Contains("w.usabilla.com") // signup
       || url.GetString().Contains("cdn.tinypass.com/api/tinypass.min.js") // subscribe
       || url.GetString().Contains("tag.rightmessage.com") // subscribe
       || ((url.Host() == "latimes.com" || url.Host().Contains(".latimes.com")) && url.GetString().Contains("tribdss.com/meter/assets/latarc-reaction")) // subscribe
      )
      {
        return ResourceRequestBlockedReason::kInspector;
      }
    }
    if (!GetSubresourceFilter()->AllowLoad(url, request_context,
                                           reporting_policy)) {
       if (!Url().IsNull() && !Url().Host().IsNull() && (Url().Host().Contains("bild.de")
           || Url().Host().Contains("postimees.ee")
           || Url().Host().Contains("grammarly.com")
           || Url().Host().Contains("espn.com")))
          return base::nullopt;
      if (url.Host().Contains("api.ero-advertising.com") && url.GetPath().Contains("get.php"))
      {
          return base::nullopt;
      }
      if (url.Host().Contains("xvideos-cdn.com"))
      {
          return base::nullopt;
      }
      return ResourceRequestBlockedReason::kSubresourceFilter;
    }
  }

  return base::nullopt;
}

base::Optional<ResourceRequestBlockedReason>
BaseFetchContext::CheckResponseNosniff(
    WebURLRequest::RequestContext request_context,
    const ResourceResponse& response) const {
  bool sniffing_allowed =
      ParseContentTypeOptionsHeader(response.HttpHeaderField(
          HTTPNames::X_Content_Type_Options)) != kContentTypeOptionsNosniff;
  if (sniffing_allowed)
    return base::nullopt;

  String mime_type = response.HttpContentType();
  if (request_context == WebURLRequest::kRequestContextStyle &&
      !MIMETypeRegistry::IsSupportedStyleSheetMIMEType(mime_type)) {
    AddConsoleMessage(ConsoleMessage::Create(
        kSecurityMessageSource, kErrorMessageLevel,
        "Refused to apply style from '" + response.Url().ElidedString() +
            "' because its MIME type ('" + mime_type + "') " +
            "is not a supported stylesheet MIME type, and strict MIME checking "
            "is enabled."));
    return ResourceRequestBlockedReason::kContentType;
  }
  // TODO(mkwst): Move the 'nosniff' bit of 'AllowedByNosniff::MimeTypeAsScript'
  // here alongside the style checks, and put its use counters somewhere else.

  return base::nullopt;
}

void BaseFetchContext::Trace(blink::Visitor* visitor) {
  FetchContext::Trace(visitor);
}

}  // namespace blink
