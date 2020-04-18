// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_FETCH_FETCH_REQUEST_DATA_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_FETCH_FETCH_REQUEST_DATA_H_

#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "services/network/public/mojom/fetch_api.mojom-blink.h"
#include "services/network/public/mojom/url_loader_factory.mojom-blink.h"
#include "third_party/blink/public/platform/modules/fetch/fetch_api_request.mojom-shared.h"
#include "third_party/blink/public/platform/modules/serviceworker/web_service_worker_request.h"
#include "third_party/blink/public/platform/web_url_request.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/weborigin/referrer.h"
#include "third_party/blink/renderer/platform/weborigin/referrer_policy.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

class BodyStreamBuffer;
class FetchHeaderList;
class SecurityOrigin;
class ScriptState;
class WebServiceWorkerRequest;

class FetchRequestData final
    : public GarbageCollectedFinalized<FetchRequestData> {
 public:
  enum Tainting { kBasicTainting, kCORSTainting, kOpaqueTainting };

  static FetchRequestData* Create();
  static FetchRequestData* Create(ScriptState*, const WebServiceWorkerRequest&);
  // Call Request::refreshBody() after calling clone() or pass().
  FetchRequestData* Clone(ScriptState*);
  FetchRequestData* Pass(ScriptState*);
  ~FetchRequestData();

  void SetMethod(AtomicString method) { method_ = method; }
  const AtomicString& Method() const { return method_; }
  void SetURL(const KURL& url) { url_ = url; }
  const KURL& Url() const { return url_; }
  WebURLRequest::RequestContext Context() const { return context_; }
  void SetContext(WebURLRequest::RequestContext context) { context_ = context; }
  scoped_refptr<const SecurityOrigin> Origin() { return origin_; }
  void SetOrigin(scoped_refptr<const SecurityOrigin> origin) {
    origin_ = std::move(origin);
  }
  bool SameOriginDataURLFlag() { return same_origin_data_url_flag_; }
  void SetSameOriginDataURLFlag(bool flag) {
    same_origin_data_url_flag_ = flag;
  }
  const Referrer& GetReferrer() const { return referrer_; }
  void SetReferrer(const Referrer& r) { referrer_ = r; }
  const AtomicString& ReferrerString() const { return referrer_.referrer; }
  void SetReferrerString(const AtomicString& s) { referrer_.referrer = s; }
  ReferrerPolicy GetReferrerPolicy() const { return referrer_.referrer_policy; }
  void SetReferrerPolicy(ReferrerPolicy p) { referrer_.referrer_policy = p; }
  void SetMode(network::mojom::FetchRequestMode mode) { mode_ = mode; }
  network::mojom::FetchRequestMode Mode() const { return mode_; }
  void SetCredentials(network::mojom::FetchCredentialsMode credentials) {
    credentials_ = credentials;
  }
  network::mojom::FetchCredentialsMode Credentials() const {
    return credentials_;
  }
  void SetCacheMode(mojom::FetchCacheMode cache_mode) {
    cache_mode_ = cache_mode;
  }
  mojom::FetchCacheMode CacheMode() const { return cache_mode_; }
  void SetRedirect(network::mojom::FetchRedirectMode redirect) {
    redirect_ = redirect;
  }
  network::mojom::FetchRedirectMode Redirect() const { return redirect_; }
  void SetResponseTainting(Tainting tainting) { response_tainting_ = tainting; }
  Tainting ResponseTainting() const { return response_tainting_; }
  FetchHeaderList* HeaderList() const { return header_list_.Get(); }
  void SetHeaderList(FetchHeaderList* header_list) {
    header_list_ = header_list;
  }
  BodyStreamBuffer* Buffer() const { return buffer_; }
  // Call Request::refreshBody() after calling setBuffer().
  void SetBuffer(BodyStreamBuffer* buffer) { buffer_ = buffer; }
  String MimeType() const { return mime_type_; }
  void SetMIMEType(const String& type) { mime_type_ = type; }
  String Integrity() const { return integrity_; }
  void SetIntegrity(const String& integrity) { integrity_ = integrity; }
  bool Keepalive() const { return keepalive_; }
  void SetKeepalive(bool b) { keepalive_ = b; }

  network::mojom::blink::URLLoaderFactory* URLLoaderFactory() const {
    return url_loader_factory_.get();
  }
  void SetURLLoaderFactory(network::mojom::blink::URLLoaderFactoryPtr factory) {
    url_loader_factory_ = std::move(factory);
  }

  // We use these strings instead of "no-referrer" and "client" in the spec.
  static AtomicString NoReferrerString() { return AtomicString(); }
  static AtomicString ClientReferrerString() {
    return AtomicString("about:client");
  }

  void Trace(blink::Visitor*);

 private:
  FetchRequestData();

  FetchRequestData* CloneExceptBody();

  AtomicString method_;
  KURL url_;
  Member<FetchHeaderList> header_list_;
  // FIXME: Support m_skipServiceWorkerFlag;
  WebURLRequest::RequestContext context_;
  scoped_refptr<const SecurityOrigin> origin_;
  // FIXME: Support m_forceOriginHeaderFlag;
  bool same_origin_data_url_flag_;
  // |m_referrer| consists of referrer string and referrer policy.
  // We use |noReferrerString()| and |clientReferrerString()| as
  // "no-referrer" and "client" strings in the spec.
  Referrer referrer_;
  // FIXME: Support m_authenticationFlag;
  // FIXME: Support m_synchronousFlag;
  network::mojom::FetchRequestMode mode_;
  network::mojom::FetchCredentialsMode credentials_;
  // TODO(yiyix): |cache_mode_| is exposed but does not yet affect fetch
  // behavior. We must transfer the mode to the network layer and service
  // worker.
  mojom::FetchCacheMode cache_mode_;
  network::mojom::FetchRedirectMode redirect_;
  // FIXME: Support m_useURLCredentialsFlag;
  // FIXME: Support m_redirectCount;
  Tainting response_tainting_;
  Member<BodyStreamBuffer> buffer_;
  String mime_type_;
  String integrity_;
  bool keepalive_;
  // A specific factory that should be used for this request instead of whatever
  // the system would otherwise decide to use to load this request.
  // Currently used for blob: URLs, to ensure they can still be loaded even if
  // the URL got revoked after creating the request.
  network::mojom::blink::URLLoaderFactoryPtr url_loader_factory_;

  DISALLOW_COPY_AND_ASSIGN(FetchRequestData);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_FETCH_FETCH_REQUEST_DATA_H_
