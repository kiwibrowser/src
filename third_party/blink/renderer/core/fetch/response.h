// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_FETCH_RESPONSE_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_FETCH_RESPONSE_H_

#include "third_party/blink/public/platform/modules/fetch/fetch_api_response.mojom-blink.h"
#include "third_party/blink/renderer/bindings/core/v8/dictionary.h"
#include "third_party/blink/renderer/bindings/core/v8/script_value.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/fetch/body.h"
#include "third_party/blink/renderer/core/fetch/body_stream_buffer.h"
#include "third_party/blink/renderer/core/fetch/fetch_response_data.h"
#include "third_party/blink/renderer/core/fetch/headers.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/blob/blob_data.h"
#include "third_party/blink/renderer/platform/heap/handle.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

class ExceptionState;
class ResponseInit;
class ScriptState;
class WebServiceWorkerResponse;

class CORE_EXPORT Response final : public Body {
  DEFINE_WRAPPERTYPEINFO();

 public:
  // These "create" function which takes a ScriptState* must be called with
  // entering an appropriate V8 context.
  // From Response.idl:
  static Response* Create(ScriptState*, ExceptionState&);
  static Response* Create(ScriptState*,
                          ScriptValue body,
                          const ResponseInit&,
                          ExceptionState&);

  static Response* Create(ScriptState*,
                          BodyStreamBuffer*,
                          const String& content_type,
                          const ResponseInit&,
                          ExceptionState&);
  static Response* Create(ExecutionContext*, FetchResponseData*);
  static Response* Create(ScriptState*, const WebServiceWorkerResponse&);
  static Response* Create(ScriptState*, mojom::blink::FetchAPIResponse&);

  static Response* CreateClone(const Response&);

  static Response* error(ScriptState*);
  static Response* redirect(ScriptState*,
                            const String& url,
                            unsigned short status,
                            ExceptionState&);

  const FetchResponseData* GetResponse() const { return response_; }

  // From Response.idl:
  String type() const;
  String url() const;
  bool redirected() const;
  unsigned short status() const;
  bool ok() const;
  String statusText() const;
  Headers* headers() const;

  // From Response.idl:
  // This function must be called with entering an appropriate V8 context.
  Response* clone(ScriptState*, ExceptionState&);

  // ScriptWrappable
  bool HasPendingActivity() const final;

  // Does not call response.setBlobDataHandle().
  void PopulateWebServiceWorkerResponse(
      WebServiceWorkerResponse& /* response */);
  mojom::blink::FetchAPIResponsePtr PopulateFetchAPIResponse();

  bool HasBody() const;
  BodyStreamBuffer* BodyBuffer() override { return response_->Buffer(); }
  // Returns the BodyStreamBuffer of |m_response|. This method doesn't check
  // the internal response of |m_response| even if |m_response| has it.
  const BodyStreamBuffer* BodyBuffer() const override {
    return response_->Buffer();
  }
  // Returns the BodyStreamBuffer of the internal response of |m_response| if
  // any. Otherwise, returns one of |m_response|.
  BodyStreamBuffer* InternalBodyBuffer() { return response_->InternalBuffer(); }
  const BodyStreamBuffer* InternalBodyBuffer() const {
    return response_->InternalBuffer();
  }
  bool bodyUsed() override;

  String ContentType() const override;
  String MimeType() const override;
  String InternalMIMEType() const;

  const Vector<KURL>& InternalURLList() const;

  void Trace(blink::Visitor*) override;

 private:
  explicit Response(ExecutionContext*);
  Response(ExecutionContext*, FetchResponseData*);
  Response(ExecutionContext*, FetchResponseData*, Headers*);

  void InstallBody();
  void RefreshBody(ScriptState*);

  const Member<FetchResponseData> response_;
  const Member<Headers> headers_;
  DISALLOW_COPY_AND_ASSIGN(Response);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_FETCH_RESPONSE_H_
