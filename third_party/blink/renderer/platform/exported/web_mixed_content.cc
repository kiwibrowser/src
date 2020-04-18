/*
 * Copyright (C) 2016 Google Inc. All rights reserved.
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

#include "third_party/blink/public/platform/web_mixed_content.h"

namespace blink {

// static
WebMixedContentContextType WebMixedContent::ContextTypeFromRequestContext(
    WebURLRequest::RequestContext context,
    bool strict_mixed_content_checking_for_plugin) {
  switch (context) {
    // "Optionally-blockable" mixed content
    case WebURLRequest::kRequestContextAudio:
    case WebURLRequest::kRequestContextImage:
    case WebURLRequest::kRequestContextVideo:
      return WebMixedContentContextType::kOptionallyBlockable;

    // Plugins! Oh how dearly we love plugin-loaded content!
    case WebURLRequest::kRequestContextPlugin: {
      return strict_mixed_content_checking_for_plugin
                 ? WebMixedContentContextType::kBlockable
                 : WebMixedContentContextType::kOptionallyBlockable;
    }

    // "Blockable" mixed content
    case WebURLRequest::kRequestContextBeacon:
    case WebURLRequest::kRequestContextCSPReport:
    case WebURLRequest::kRequestContextEmbed:
    case WebURLRequest::kRequestContextEventSource:
    case WebURLRequest::kRequestContextFavicon:
    case WebURLRequest::kRequestContextFetch:
    case WebURLRequest::kRequestContextFont:
    case WebURLRequest::kRequestContextForm:
    case WebURLRequest::kRequestContextFrame:
    case WebURLRequest::kRequestContextHyperlink:
    case WebURLRequest::kRequestContextIframe:
    case WebURLRequest::kRequestContextImageSet:
    case WebURLRequest::kRequestContextImport:
    case WebURLRequest::kRequestContextInternal:
    case WebURLRequest::kRequestContextLocation:
    case WebURLRequest::kRequestContextManifest:
    case WebURLRequest::kRequestContextObject:
    case WebURLRequest::kRequestContextPing:
    case WebURLRequest::kRequestContextPrefetch:
    case WebURLRequest::kRequestContextScript:
    case WebURLRequest::kRequestContextServiceWorker:
    case WebURLRequest::kRequestContextSharedWorker:
    case WebURLRequest::kRequestContextStyle:
    case WebURLRequest::kRequestContextSubresource:
    case WebURLRequest::kRequestContextTrack:
    case WebURLRequest::kRequestContextWorker:
    case WebURLRequest::kRequestContextXMLHttpRequest:
    case WebURLRequest::kRequestContextXSLT:
      return WebMixedContentContextType::kBlockable;

    // FIXME: Contexts that we should block, but don't currently.
    // https://crbug.com/388650
    case WebURLRequest::kRequestContextDownload:
      return WebMixedContentContextType::kShouldBeBlockable;

    case WebURLRequest::kRequestContextUnspecified:
      NOTREACHED();
  }
  NOTREACHED();
  return WebMixedContentContextType::kBlockable;
}

}  // namespace blink
