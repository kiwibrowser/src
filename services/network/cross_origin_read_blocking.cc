// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/network/cross_origin_read_blocking.h"

#include <stddef.h>

#include <algorithm>
#include <string>
#include <unordered_set>

#include "base/command_line.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_piece.h"
#include "base/strings/string_util.h"
#include "net/base/mime_sniffer.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "net/http/http_response_headers.h"
#include "net/url_request/url_request.h"
#include "services/network/public/cpp/resource_response.h"
#include "services/network/public/cpp/resource_response_info.h"

using base::StringPiece;
using MimeType = network::CrossOriginReadBlocking::MimeType;
using SniffingResult = network::CrossOriginReadBlocking::SniffingResult;

namespace network {

namespace {

// MIME types
const char kTextHtml[] = "text/html";
const char kTextXml[] = "text/xml";
const char kAppXml[] = "application/xml";
const char kAppJson[] = "application/json";
const char kImageSvg[] = "image/svg+xml";
const char kTextJson[] = "text/json";
const char kTextPlain[] = "text/plain";
// TODO(lukasza): Remove kJsonProtobuf once this MIME type is not used in
// practice.  See also https://crbug.com/826756#c3
const char kJsonProtobuf[] = "application/json+protobuf";

// MIME type suffixes
const char kJsonSuffix[] = "+json";
const char kXmlSuffix[] = "+xml";

void AdvancePastWhitespace(StringPiece* data) {
  size_t offset = data->find_first_not_of(" \t\r\n");
  if (offset == base::StringPiece::npos) {
    // |data| was entirely whitespace.
    data->clear();
  } else {
    data->remove_prefix(offset);
  }
}

// Returns kYes if |data| starts with one of the string patterns in
// |signatures|, kMaybe if |data| is a prefix of one of the patterns in
// |signatures|, and kNo otherwise.
//
// When kYes is returned, the matching prefix is erased from |data|.
SniffingResult MatchesSignature(StringPiece* data,
                                const StringPiece signatures[],
                                size_t arr_size,
                                base::CompareCase compare_case) {
  for (size_t i = 0; i < arr_size; ++i) {
    if (signatures[i].length() <= data->length()) {
      if (base::StartsWith(*data, signatures[i], compare_case)) {
        // When |signatures[i]| is a prefix of |data|, it constitutes a match.
        // Strip the matching characters, and return.
        data->remove_prefix(signatures[i].length());
        return CrossOriginReadBlocking::kYes;
      }
    } else {
      if (base::StartsWith(signatures[i], *data, compare_case)) {
        // When |data| is a prefix of |signatures[i]|, that means that
        // subsequent bytes in the stream could cause a match to occur.
        return CrossOriginReadBlocking::kMaybe;
      }
    }
  }
  return CrossOriginReadBlocking::kNo;
}

size_t FindFirstJavascriptLineTerminator(const base::StringPiece& hay,
                                         size_t pos) {
  // https://www.ecma-international.org/ecma-262/8.0/index.html#prod-LineTerminator
  // defines LineTerminator ::= <LF> | <CR> | <LS> | <PS>.
  //
  // https://www.ecma-international.org/ecma-262/8.0/index.html#sec-line-terminators
  // defines <LF>, <CR>, <LS> ::= "\u2028", <PS> ::= "\u2029".
  //
  // In UTF8 encoding <LS> is 0xE2 0x80 0xA8 and <PS> is 0xE2 0x80 0xA9.
  while (true) {
    pos = hay.find_first_of("\n\r\xe2", pos);
    if (pos == base::StringPiece::npos)
      break;

    if (hay[pos] != '\xe2') {
      DCHECK(hay[pos] == '\r' || hay[pos] == '\n');
      break;
    }

    // TODO(lukasza): Prevent matching 3 bytes that span/straddle 2 UTF8
    // characters.
    base::StringPiece substr = hay.substr(pos);
    if (substr.starts_with("\u2028") || substr.starts_with("\u2029"))
      break;

    pos++;  // Skip the \xe2 character.
  }
  return pos;
}

// Checks if |data| starts with an HTML comment (i.e. with "<!-- ... -->").
// - If there is a valid, terminated comment then returns kYes.
// - If there is a start of a comment, but the comment is not completed (e.g.
//   |data| == "<!-" or |data| == "<!-- not terminated yet") then returns
//   kMaybe.
// - Returns kNo otherwise.
//
// Mutates |data| to advance past the comment when returning kYes.  Note that
// SingleLineHTMLCloseComment ECMAscript rule is taken into account which means
// that characters following an HTML comment are consumed up to the nearest line
// terminating character.
SniffingResult MaybeSkipHtmlComment(StringPiece* data) {
  constexpr StringPiece kStartString = "<!--";
  if (!data->starts_with(kStartString)) {
    if (kStartString.starts_with(*data))
      return CrossOriginReadBlocking::kMaybe;
    return CrossOriginReadBlocking::kNo;
  }

  constexpr StringPiece kEndString = "-->";
  size_t end_of_html_comment = data->find(kEndString, kStartString.length());
  if (end_of_html_comment == StringPiece::npos)
    return CrossOriginReadBlocking::kMaybe;
  end_of_html_comment += kEndString.length();

  // Skipping until the first line terminating character.  See
  // https://crbug.com/839945 for the motivation behind this.
  size_t end_of_line =
      FindFirstJavascriptLineTerminator(*data, end_of_html_comment);
  if (end_of_line == base::StringPiece::npos)
    return CrossOriginReadBlocking::kMaybe;

  // Found real end of the combined HTML/JS comment.
  data->remove_prefix(end_of_line);
  return CrossOriginReadBlocking::kYes;
}

// Headers from
// https://fetch.spec.whatwg.org/#cors-safelisted-response-header-name.
//
// Note that XSDB doesn't block responses allowed through CORS - this means
// that the list of allowed headers below doesn't have to consider header
// names listed in the Access-Control-Expose-Headers header.
const char* const kCorsSafelistedHeaders[] = {
    "cache-control", "content-language", "content-type",
    "expires",       "last-modified",    "pragma",
};

// Removes headers that should be blocked in cross-origin case.
// See https://fetch.spec.whatwg.org/#cors-safelisted-response-header-name.
void BlockResponseHeaders(
    const scoped_refptr<net::HttpResponseHeaders>& headers) {
  DCHECK(headers);
  std::unordered_set<std::string> names_of_headers_to_remove;

  size_t it = 0;
  std::string name;
  std::string value;
  while (headers->EnumerateHeaderLines(&it, &name, &value)) {
    // Don't remove CORS headers - doing so would lead to incorrect error
    // messages for CORS-blocked responses (e.g. Blink would say "[...] No
    // 'Access-Control-Allow-Origin' header is present [...]" instead of saying
    // something like "[...] Access-Control-Allow-Origin' header has a value
    // 'http://www2.localhost:8000' that is not equal to the supplied origin
    // [...]").
    if (base::StartsWith(name, "Access-Control-",
                         base::CompareCase::INSENSITIVE_ASCII)) {
      continue;
    }

    // Remove all other headers (but note the final exclusion below).
    names_of_headers_to_remove.insert(base::ToLowerASCII(name));
  }

  // Exclude from removals headers from
  // https://fetch.spec.whatwg.org/#cors-safelisted-response-header-name.
  for (const char* header : kCorsSafelistedHeaders)
    names_of_headers_to_remove.erase(header);

  headers->RemoveHeaders(names_of_headers_to_remove);
}

}  // namespace

MimeType CrossOriginReadBlocking::GetCanonicalMimeType(
    base::StringPiece mime_type) {
  // Checking for image/svg+xml early ensures that it won't get classified as
  // MimeType::kXml by the presence of the "+xml"
  // suffix.
  if (base::LowerCaseEqualsASCII(mime_type, kImageSvg))
    return MimeType::kOthers;

  // See also https://mimesniff.spec.whatwg.org/#html-mime-type
  if (base::LowerCaseEqualsASCII(mime_type, kTextHtml))
    return MimeType::kHtml;

  // See also https://mimesniff.spec.whatwg.org/#json-mime-type
  constexpr auto kCaseInsensitive = base::CompareCase::INSENSITIVE_ASCII;
  if (base::LowerCaseEqualsASCII(mime_type, kAppJson) ||
      base::LowerCaseEqualsASCII(mime_type, kTextJson) ||
      base::LowerCaseEqualsASCII(mime_type, kJsonProtobuf) ||
      base::EndsWith(mime_type, kJsonSuffix, kCaseInsensitive)) {
    return MimeType::kJson;
  }

  // See also https://mimesniff.spec.whatwg.org/#xml-mime-type
  if (base::LowerCaseEqualsASCII(mime_type, kAppXml) ||
      base::LowerCaseEqualsASCII(mime_type, kTextXml) ||
      base::EndsWith(mime_type, kXmlSuffix, kCaseInsensitive)) {
    return MimeType::kXml;
  }

  if (base::LowerCaseEqualsASCII(mime_type, kTextPlain))
    return MimeType::kPlain;

  return MimeType::kOthers;
}

bool CrossOriginReadBlocking::IsBlockableScheme(const GURL& url) {
  // We exclude ftp:// from here. FTP doesn't provide a Content-Type
  // header which our policy depends on, so we cannot protect any
  // response from FTP servers.
  return url.SchemeIs(url::kHttpScheme) || url.SchemeIs(url::kHttpsScheme);
}

bool CrossOriginReadBlocking::IsValidCorsHeaderSet(
    const url::Origin& frame_origin,
    const std::string& access_control_origin) {
  // Many websites are sending back "\"*\"" instead of "*". This is
  // non-standard practice, and not supported by Chrome. Refer to
  // CrossOriginAccessControl::passesAccessControlCheck().

  // Note that "null" offers no more protection than "*" because it matches any
  // unique origin, such as data URLs. Any origin can thus access it, so don't
  // bother trying to block this case.

  // TODO(dsjang): * is not allowed for the response from a request
  // with cookies. This allows for more than what the renderer will
  // eventually be able to receive, so we won't see illegal cross-site
  // documents allowed by this. We have to find a way to see if this
  // response is from a cookie-tagged request or not in the future.
  if (access_control_origin == "*" || access_control_origin == "null")
    return true;

  return frame_origin.IsSameOriginWith(
      url::Origin::Create(GURL(access_control_origin)));
}

// This function is a slight modification of |net::SniffForHTML|.
SniffingResult CrossOriginReadBlocking::SniffForHTML(StringPiece data) {
  // The content sniffers used by Chrome and Firefox are using "<!--" as one of
  // the HTML signatures, but it also appears in valid JavaScript, considered as
  // well-formed JS by the browser.  Since we do not want to block any JS, we
  // exclude it from our HTML signatures. This can weaken our CORB policy,
  // but we can break less websites.
  //
  // Note that <body> and <br> are not included below, since <b is a prefix of
  // them.
  //
  // TODO(dsjang): parameterize |net::SniffForHTML| with an option that decides
  // whether to include <!-- or not, so that we can remove this function.
  // TODO(dsjang): Once CrossOriginReadBlocking is moved into the browser
  // process, we should do single-thread checking here for the static
  // initializer.
  static const StringPiece kHtmlSignatures[] = {
      StringPiece("<!doctype html"),  // HTML5 spec
      StringPiece("<script"),         // HTML5 spec, Mozilla
      StringPiece("<html"),           // HTML5 spec, Mozilla
      StringPiece("<head"),           // HTML5 spec, Mozilla
      StringPiece("<iframe"),         // Mozilla
      StringPiece("<h1"),             // Mozilla
      StringPiece("<div"),            // Mozilla
      StringPiece("<font"),           // Mozilla
      StringPiece("<table"),          // Mozilla
      StringPiece("<a"),              // Mozilla
      StringPiece("<style"),          // Mozilla
      StringPiece("<title"),          // Mozilla
      StringPiece("<b"),              // Mozilla (note: subsumes <body>, <br>)
      StringPiece("<p")               // Mozilla
  };

  while (data.length() > 0) {
    AdvancePastWhitespace(&data);

    SniffingResult signature_match =
        MatchesSignature(&data, kHtmlSignatures, arraysize(kHtmlSignatures),
                         base::CompareCase::INSENSITIVE_ASCII);
    if (signature_match != kNo)
      return signature_match;

    SniffingResult comment_match = MaybeSkipHtmlComment(&data);
    if (comment_match != kYes)
      return comment_match;
  }

  // All of |data| was consumed, without a clear determination.
  return kMaybe;
}

SniffingResult CrossOriginReadBlocking::SniffForXML(base::StringPiece data) {
  // TODO(dsjang): Once CrossOriginReadBlocking is moved into the browser
  // process, we should do single-thread checking here for the static
  // initializer.
  AdvancePastWhitespace(&data);
  static const StringPiece kXmlSignatures[] = {StringPiece("<?xml")};
  return MatchesSignature(&data, kXmlSignatures, arraysize(kXmlSignatures),
                          base::CompareCase::SENSITIVE);
}

SniffingResult CrossOriginReadBlocking::SniffForJSON(base::StringPiece data) {
  // Currently this function looks for an opening brace ('{'), followed by a
  // double-quoted string literal, followed by a colon. Importantly, such a
  // sequence is a Javascript syntax error: although the JSON object syntax is
  // exactly Javascript's object-initializer syntax, a Javascript object-
  // initializer expression is not valid as a standalone Javascript statement.
  //
  // TODO(nick): We have to come up with a better way to sniff JSON. The
  // following are known limitations of this function:
  // https://crbug.com/795470/ Support non-dictionary values (e.g. lists)
  enum {
    kStartState,
    kLeftBraceState,
    kLeftQuoteState,
    kEscapeState,
    kRightQuoteState,
  } state = kStartState;

  for (size_t i = 0; i < data.length(); ++i) {
    const char c = data[i];
    if (state != kLeftQuoteState && state != kEscapeState) {
      // Whitespace is ignored (outside of string literals)
      if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
        continue;
    } else {
      // Inside string literals, control characters should result in rejection.
      if ((c >= 0 && c < 32) || c == 127)
        return kNo;
    }

    switch (state) {
      case kStartState:
        if (c == '{')
          state = kLeftBraceState;
        else
          return kNo;
        break;
      case kLeftBraceState:
        if (c == '"')
          state = kLeftQuoteState;
        else
          return kNo;
        break;
      case kLeftQuoteState:
        if (c == '"')
          state = kRightQuoteState;
        else if (c == '\\')
          state = kEscapeState;
        break;
      case kEscapeState:
        // Simplification: don't bother rejecting hex escapes.
        state = kLeftQuoteState;
        break;
      case kRightQuoteState:
        if (c == ':')
          return kYes;
        else
          return kNo;
        break;
    }
  }
  return kMaybe;
}

SniffingResult CrossOriginReadBlocking::SniffForFetchOnlyResource(
    base::StringPiece data) {
  // kScriptBreakingPrefixes contains prefixes that are conventionally used to
  // prevent a JSON response from becoming a valid Javascript program (an attack
  // vector known as XSSI). The presence of such a prefix is a strong signal
  // that the resource is meant to be consumed only by the fetch API or
  // XMLHttpRequest, and is meant to be protected from use in non-CORS, cross-
  // origin contexts like <script>, <img>, etc.
  //
  // These prefixes work either by inducing a syntax error, or inducing an
  // infinite loop. In either case, the prefix must create a guarantee that no
  // matter what bytes follow it, the entire response would be worthless to
  // execute as a <script>.
  static const StringPiece kScriptBreakingPrefixes[] = {
      // Parser breaker prefix.
      //
      // Built into angular.js (followed by a comma and a newline):
      //   https://docs.angularjs.org/api/ng/service/$http
      //
      // Built into the Java Spring framework (followed by a comma and a space):
      //   https://goo.gl/xP7FWn
      //
      // Observed on google.com (without a comma, followed by a newline).
      StringPiece(")]}'"),

      // Apache struts: https://struts.apache.org/plugins/json/#prefix
      StringPiece("{}&&"),

      // Spring framework (historically): https://goo.gl/JYPFAv
      StringPiece("{} &&"),

      // Infinite loops.
      StringPiece("for(;;);"),  // observed on facebook.com
      StringPiece("while(1);"), StringPiece("for (;;);"),
      StringPiece("while (1);"),
  };
  SniffingResult has_parser_breaker = MatchesSignature(
      &data, kScriptBreakingPrefixes, arraysize(kScriptBreakingPrefixes),
      base::CompareCase::SENSITIVE);
  if (has_parser_breaker != kNo)
    return has_parser_breaker;

  // A non-empty JSON object also effectively introduces a JS syntax error.
  return SniffForJSON(data);
}

// static
void CrossOriginReadBlocking::SanitizeBlockedResponse(
    const scoped_refptr<network::ResourceResponse>& response) {
  DCHECK(response);
  response->head.content_length = 0;
  if (response->head.headers)
    BlockResponseHeaders(response->head.headers);
}

// static
std::vector<std::string>
CrossOriginReadBlocking::GetCorsSafelistedHeadersForTesting() {
  return std::vector<std::string>(
      kCorsSafelistedHeaders,
      kCorsSafelistedHeaders + arraysize(kCorsSafelistedHeaders));
}

// static
void CrossOriginReadBlocking::LogAction(Action action) {
  UMA_HISTOGRAM_ENUMERATION("SiteIsolation.XSD.Browser.Action", action);
}

// An interface to enable incremental content sniffing. These are instantiated
// for each each request; thus they can be stateful.
class CrossOriginReadBlocking::ResponseAnalyzer::ConfirmationSniffer {
 public:
  virtual ~ConfirmationSniffer() = default;

  // Called after data is read from the network. |sniffing_buffer| contains the
  // entire response body delivered thus far. To support streaming,
  // |new_data_offset| gives the offset into |sniffing_buffer| at which new data
  // was appended since the last read.
  virtual void OnDataAvailable(base::StringPiece sniffing_buffer,
                               size_t new_data_offset) = 0;

  // Returns true if the return value of IsConfirmedContentType() might change
  // with the addition of more data. Returns false if a final decision is
  // available.
  virtual bool WantsMoreData() const = 0;

  // Returns true if the data has been confirmed to be of the CORB-protected
  // content type that this sniffer is intended to detect.
  virtual bool IsConfirmedContentType() const = 0;

  // Helper for reporting the right UMA.
  virtual bool IsParserBreakerSniffer() const = 0;
};

// A ConfirmationSniffer that wraps one of the sniffing functions from
// network::CrossOriginReadBlocking.
class CrossOriginReadBlocking::ResponseAnalyzer::SimpleConfirmationSniffer
    : public CrossOriginReadBlocking::ResponseAnalyzer::ConfirmationSniffer {
 public:
  // The function pointer type corresponding to one of the available sniffing
  // functions from network::CrossOriginReadBlocking.
  using SnifferFunction =
      decltype(&network::CrossOriginReadBlocking::SniffForHTML);

  explicit SimpleConfirmationSniffer(SnifferFunction sniffer_function)
      : sniffer_function_(sniffer_function) {}
  ~SimpleConfirmationSniffer() override = default;

  void OnDataAvailable(base::StringPiece sniffing_buffer,
                       size_t new_data_offset) final {
    DCHECK_LE(new_data_offset, sniffing_buffer.length());
    if (new_data_offset == sniffing_buffer.length()) {
      // No new data -- do nothing. This happens at end-of-stream.
      return;
    }
    // The sniffing functions don't support streaming, so with each new chunk of
    // data, call the sniffer on the whole buffer.
    last_sniff_result_ = (*sniffer_function_)(sniffing_buffer);
  }

  bool WantsMoreData() const final {
    // kNo and kYes results are final, meaning that sniffing can stop once they
    // occur. A kMaybe result corresponds to an indeterminate state, that could
    // change to kYes or kNo with more data.
    return last_sniff_result_ == SniffingResult::kMaybe;
  }

  bool IsConfirmedContentType() const final {
    // Only confirm the mime type if an affirmative pattern (e.g. an HTML tag,
    // if using the HTML sniffer) was detected.
    //
    // Note that if the stream ends (or net::kMaxBytesToSniff has been reached)
    // and |last_sniff_result_| is kMaybe, the response is allowed to go
    // through.
    return last_sniff_result_ == SniffingResult::kYes;
  }

  bool IsParserBreakerSniffer() const override { return false; }

 private:
  // The function that actually knows how to sniff for a content type.
  SnifferFunction sniffer_function_;

  // Result of sniffing the data available thus far.
  SniffingResult last_sniff_result_ = SniffingResult::kMaybe;

  DISALLOW_COPY_AND_ASSIGN(SimpleConfirmationSniffer);
};

// A ConfirmationSniffer for parser breakers (fetch-only resources). This logs
// to an UMA histogram whenever it is the reason for a response being blocked.
class CrossOriginReadBlocking::ResponseAnalyzer::FetchOnlyResourceSniffer
    : public CrossOriginReadBlocking::ResponseAnalyzer::
          SimpleConfirmationSniffer {
 public:
  FetchOnlyResourceSniffer()
      : SimpleConfirmationSniffer(
            &network::CrossOriginReadBlocking::SniffForFetchOnlyResource) {}

  bool IsParserBreakerSniffer() const override { return true; }

 private:
  DISALLOW_COPY_AND_ASSIGN(FetchOnlyResourceSniffer);
};

CrossOriginReadBlocking::ResponseAnalyzer::ResponseAnalyzer(
    const net::URLRequest& request,
    const ResourceResponse& response) {
  content_length_ = response.head.content_length;
  should_block_based_on_headers_ = ShouldBlockBasedOnHeaders(request, response);
  if (should_block_based_on_headers_ == kNeedToSniffMore)
    CreateSniffers();
}

CrossOriginReadBlocking::ResponseAnalyzer::~ResponseAnalyzer() = default;

CrossOriginReadBlocking::ResponseAnalyzer::BlockingDecision
CrossOriginReadBlocking::ResponseAnalyzer::ShouldBlockBasedOnHeaders(
    const net::URLRequest& request,
    const ResourceResponse& response) {
  // The checks in this method are ordered to rule out blocking in most cases as
  // quickly as possible.  Checks that are likely to lead to returning false or
  // that are inexpensive should be near the top.
  url::Origin target_origin = url::Origin::Create(request.url());

  // Treat a missing initiator as an empty origin to be safe, though we don't
  // expect this to happen.  Unfortunately, this requires a copy.
  url::Origin initiator;
  if (request.initiator().has_value())
    initiator = request.initiator().value();

  // Don't block same-origin documents.
  if (initiator.IsSameOriginWith(target_origin))
    return kAllow;

  // Only block documents from HTTP(S) schemes.  Checking the scheme of
  // |target_origin| ensures that we also protect content of blob: and
  // filesystem: URLs if their nested origins have a HTTP(S) scheme.
  if (!IsBlockableScheme(target_origin.GetURL()))
    return kAllow;

  // Allow requests from file:// URLs for now.
  // TODO(creis): Limit this to when the allow_universal_access_from_file_urls
  // preference is set.  See https://crbug.com/789781.
  if (initiator.scheme() == url::kFileScheme)
    return kAllow;

  // Allow the response through if it has valid CORS headers.
  std::string cors_header;
  response.head.headers->GetNormalizedHeader("access-control-allow-origin",
                                             &cors_header);
  if (IsValidCorsHeaderSet(initiator, cors_header)) {
    return kAllow;
  }

  // Requests from foo.example.com will consult foo.example.com's service worker
  // first (if one has been registered).  The service worker can handle requests
  // initiated by foo.example.com even if they are cross-origin (e.g. requests
  // for bar.example.com).  This is okay and should not be blocked by CORB,
  // unless the initiator opted out of CORS / opted into receiving an opaque
  // response.  See also https://crbug.com/803672.
  if (response.head.was_fetched_via_service_worker) {
    switch (response.head.response_type_via_service_worker) {
      case network::mojom::FetchResponseType::kBasic:
      case network::mojom::FetchResponseType::kCORS:
      case network::mojom::FetchResponseType::kDefault:
      case network::mojom::FetchResponseType::kError:
        // Non-opaque responses shouldn't be blocked.
        return kAllow;
      case network::mojom::FetchResponseType::kOpaque:
      case network::mojom::FetchResponseType::kOpaqueRedirect:
        // Opaque responses are eligible for blocking. Continue on...
        break;
    }
  }

  // We intend to block the response at this point.  However, we will usually
  // sniff the contents to confirm the MIME type, to avoid blocking incorrectly
  // labeled JavaScript, JSONP, etc files.
  //
  // Note: if there is a nosniff header, it means we should honor the response
  // mime type without trying to confirm it.
  std::string nosniff_header;
  response.head.headers->GetNormalizedHeader("x-content-type-options",
                                             &nosniff_header);
  bool has_nosniff_header =
      base::LowerCaseEqualsASCII(nosniff_header, "nosniff");

  // CORB should look directly at the Content-Type header if one has been
  // received from the network.  Ignoring |response.head.mime_type| helps avoid
  // breaking legitimate websites (which might happen more often when blocking
  // would be based on the mime type sniffed by MimeSniffingResourceHandler).
  //
  // TODO(nick): What if the mime type is omitted? Should that be treated the
  // same as text/plain? https://crbug.com/795971
  std::string mime_type;
  if (response.head.headers)
    response.head.headers->GetMimeType(&mime_type);
  // Canonicalize the MIME type.  Note that even if it doesn't claim to be a
  // blockable type (i.e., HTML, XML, JSON, or plain text), it may still fail
  // the checks during the SniffForFetchOnlyResource() phase.
  canonical_mime_type_ =
      network::CrossOriginReadBlocking::GetCanonicalMimeType(mime_type);

  // If this is a partial response, sniffing is not possible, so allow the
  // response if it's not a protected mime type.
  std::string range_header;
  response.head.headers->GetNormalizedHeader("content-range", &range_header);
  bool has_range_header = !range_header.empty();
  if (has_range_header) {
    switch (canonical_mime_type_) {
      case MimeType::kOthers:
      case MimeType::kPlain:  // See also https://crbug.com/801709
        return kAllow;
      case MimeType::kHtml:
      case MimeType::kJson:
      case MimeType::kXml:
        return kBlock;
      case MimeType::kMax:
        NOTREACHED();
        return kBlock;
    }
  }

  // Decide whether to block based on the MIME type.
  switch (canonical_mime_type_) {
    case MimeType::kHtml:
    case MimeType::kXml:
    case MimeType::kJson:
    case MimeType::kPlain:
      if (has_nosniff_header)
        return kBlock;
      else
        return kNeedToSniffMore;
      break;

    case MimeType::kOthers:
      // Stylesheets shouldn't be sniffed for JSON parser breakers - see
      // https://crbug.com/809259.
      if (base::LowerCaseEqualsASCII(response.head.mime_type, "text/css"))
        return kAllow;
      else
        return kNeedToSniffMore;
      break;

    case MimeType::kInvalidMimeType:
      NOTREACHED();
      return kBlock;
  }
  NOTREACHED();
  return kBlock;
}

void CrossOriginReadBlocking::ResponseAnalyzer::CreateSniffers() {
  // Create one or more |sniffers_| to confirm that the body is actually the
  // MIME type advertised in the Content-Type header.
  DCHECK_EQ(kNeedToSniffMore, should_block_based_on_headers_);
  DCHECK(sniffers_.empty());

  // When the MIME type is "text/plain", create sniffers for HTML, XML and
  // JSON. If any of these sniffers match, the response will be blocked.
  const bool use_all = canonical_mime_type() == MimeType::kPlain;

  // HTML sniffer.
  if (use_all || canonical_mime_type() == MimeType::kHtml) {
    sniffers_.push_back(std::make_unique<SimpleConfirmationSniffer>(
        &network::CrossOriginReadBlocking::SniffForHTML));
  }

  // XML sniffer.
  if (use_all || canonical_mime_type() == MimeType::kXml) {
    sniffers_.push_back(std::make_unique<SimpleConfirmationSniffer>(
        &network::CrossOriginReadBlocking::SniffForXML));
  }

  // JSON sniffer.
  if (use_all || canonical_mime_type() == MimeType::kJson) {
    sniffers_.push_back(std::make_unique<SimpleConfirmationSniffer>(
        &network::CrossOriginReadBlocking::SniffForJSON));
  }

  // Parser-breaker sniffer.
  //
  // Because these prefixes are an XSSI-defeating mechanism, CORB considers
  // them distinctive enough to be worth blocking no matter the Content-Type
  // header. So this sniffer is created unconditionally.
  //
  // For MimeType::kOthers, this will be the only sniffer that's active.
  sniffers_.push_back(std::make_unique<FetchOnlyResourceSniffer>());
}

void CrossOriginReadBlocking::ResponseAnalyzer::SniffResponseBody(
    base::StringPiece data,
    size_t new_data_offset) {
  DCHECK_EQ(kNeedToSniffMore, should_block_based_on_headers_);
  DCHECK(!sniffers_.empty());
  DCHECK(!found_blockable_content_);

  DCHECK_LE(bytes_read_for_sniffing_, static_cast<int>(data.size()));
  bytes_read_for_sniffing_ = static_cast<int>(data.size());

  DCHECK_LE(data.size(), static_cast<size_t>(net::kMaxBytesToSniff));
  DCHECK_LE(new_data_offset, data.size());
  bool has_new_data = (new_data_offset < data.size());

  for (size_t i = 0; i < sniffers_.size();) {
    if (has_new_data)
      sniffers_[i]->OnDataAvailable(data, new_data_offset);

    if (sniffers_[i]->WantsMoreData()) {
      i++;
      continue;
    }

    if (sniffers_[i]->IsConfirmedContentType()) {
      if (sniffers_[i]->IsParserBreakerSniffer())
        found_parser_breaker_ = true;

      found_blockable_content_ = true;
      sniffers_.clear();
      break;
    } else {
      // This response is CORB-exempt as far as this sniffer is concerned;
      // remove it from the list.
      sniffers_.erase(sniffers_.begin() + i);
    }
  }
}

bool CrossOriginReadBlocking::ResponseAnalyzer::should_allow() const {
  switch (should_block_based_on_headers_) {
    case kAllow:
      return true;
    case kNeedToSniffMore:
      return sniffers_.empty() && !found_blockable_content_;
    case kBlock:
      return false;
  }
}

bool CrossOriginReadBlocking::ResponseAnalyzer::should_block() const {
  switch (should_block_based_on_headers_) {
    case kAllow:
      return false;
    case kNeedToSniffMore:
      return sniffers_.empty() && found_blockable_content_;
    case kBlock:
      return true;
  }
}

void CrossOriginReadBlocking::ResponseAnalyzer::LogBytesReadForSniffing() {
  if (bytes_read_for_sniffing_ >= 0) {
    UMA_HISTOGRAM_COUNTS("SiteIsolation.XSD.Browser.BytesReadForSniffing",
                         bytes_read_for_sniffing_);
  }
}

void CrossOriginReadBlocking::ResponseAnalyzer::LogAllowedResponse() {
  // Note that if a response is allowed because of hitting EOF or
  // kMaxBytesToSniff, then |sniffers_| are not emptied and consequently
  // should_allow doesn't start returning true.  This means that we can't
  // DCHECK(should_allow()) or DCHECK(sniffers_.empty()) here - the decision to
  // allow the response could have been made in the
  // CrossSiteDocumentResourceHandler layer without CrossOriginReadBlocking
  // realizing that it has hit EOF or kMaxBytesToSniff.

  // Note that the response might be allowed even if should_block() returns true
  // - for example to allow responses to requests initiated by content scripts.
  // This means that we cannot DCHECK(!should_block()) here.

  CrossOriginReadBlocking::LogAction(
      needs_sniffing()
          ? network::CrossOriginReadBlocking::Action::kAllowedAfterSniffing
          : network::CrossOriginReadBlocking::Action::kAllowedWithoutSniffing);

  LogBytesReadForSniffing();
}

void CrossOriginReadBlocking::ResponseAnalyzer::LogBlockedResponse() {
  DCHECK(!should_allow());
  DCHECK(should_block());
  DCHECK(sniffers_.empty());

  CrossOriginReadBlocking::LogAction(
      needs_sniffing()
          ? network::CrossOriginReadBlocking::Action::kBlockedAfterSniffing
          : network::CrossOriginReadBlocking::Action::kBlockedWithoutSniffing);

  UMA_HISTOGRAM_BOOLEAN(
      "SiteIsolation.XSD.Browser.Blocked.ContentLength.WasAvailable",
      content_length() >= 0);
  if (content_length() >= 0) {
    UMA_HISTOGRAM_COUNTS_10000(
        "SiteIsolation.XSD.Browser.Blocked.ContentLength.ValueIfAvailable",
        content_length());
  }

  LogBytesReadForSniffing();
}

}  // namespace network
