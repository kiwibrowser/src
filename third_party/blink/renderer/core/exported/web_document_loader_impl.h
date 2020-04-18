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

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_EXPORTED_WEB_DOCUMENT_LOADER_IMPL_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_EXPORTED_WEB_DOCUMENT_LOADER_IMPL_H_

#include <memory>
#include "third_party/blink/public/web/web_document_loader.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/frame/frame_types.h"
#include "third_party/blink/renderer/core/loader/document_loader.h"
#include "third_party/blink/renderer/platform/exported/wrapped_resource_request.h"
#include "third_party/blink/renderer/platform/exported/wrapped_resource_response.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

// Extends blink::DocumentLoader to attach |extra_data_| to store data that can
// be set/get via the WebDocumentLoader interface.
class CORE_EXPORT WebDocumentLoaderImpl final : public DocumentLoader,
                                                public WebDocumentLoader {
 public:
  static WebDocumentLoaderImpl* Create(
      LocalFrame*,
      const ResourceRequest&,
      const SubstituteData&,
      ClientRedirectPolicy,
      const base::UnguessableToken& devtools_navigation_token);

  static WebDocumentLoaderImpl* FromDocumentLoader(DocumentLoader* loader) {
    return static_cast<WebDocumentLoaderImpl*>(loader);
  }

  // WebDocumentLoader methods:
  const WebURLRequest& OriginalRequest() const override;
  const WebURLRequest& GetRequest() const override;
  const WebURLResponse& GetResponse() const override;
  bool HasUnreachableURL() const override;
  WebURL UnreachableURL() const override;
  void AppendRedirect(const WebURL&) override;
  void RedirectChain(WebVector<WebURL>&) const override;
  bool IsClientRedirect() const override;
  bool ReplacesCurrentHistoryItem() const override;
  WebNavigationType GetNavigationType() const override;
  ExtraData* GetExtraData() const override;
  void SetExtraData(ExtraData*) override;
  void SetNavigationStartTime(base::TimeTicks) override;
  void UpdateNavigation(base::TimeTicks redirect_start_time,
                        base::TimeTicks redirect_end_time,
                        base::TimeTicks fetch_start_time,
                        bool has_redirect) override;
  void SetSubresourceFilter(WebDocumentSubresourceFilter*) override;
  void SetServiceWorkerNetworkProvider(
      std::unique_ptr<WebServiceWorkerNetworkProvider>) override;
  WebServiceWorkerNetworkProvider* GetServiceWorkerNetworkProvider() override;
  void SetSourceLocation(const WebSourceLocation&) override;
  void ResetSourceLocation() override;
  void SetUserActivated() override;
  void BlockParser() override;
  void ResumeParser() override;
  bool IsArchive() const override;
  WebArchiveInfo GetArchiveInfo() const override;

  static WebNavigationType ToWebNavigationType(NavigationType);

  void Trace(blink::Visitor*) override;

 private:
  WebDocumentLoaderImpl(
      LocalFrame*,
      const ResourceRequest&,
      const SubstituteData&,
      ClientRedirectPolicy,
      const base::UnguessableToken& devtools_navigation_token);
  ~WebDocumentLoaderImpl() override;
  void DetachFromFrame() override;
  String DebugName() const override { return "WebDocumentLoaderImpl"; }

  // Mutable because the const getters will magically sync these to the
  // latest version from WebKit.
  mutable WrappedResourceRequest original_request_wrapper_;
  mutable WrappedResourceRequest request_wrapper_;
  mutable WrappedResourceResponse response_wrapper_;

  std::unique_ptr<ExtraData> extra_data_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_EXPORTED_WEB_DOCUMENT_LOADER_IMPL_H_
