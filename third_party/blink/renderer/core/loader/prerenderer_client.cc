/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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
 *
 */

#include "third_party/blink/renderer/core/loader/prerenderer_client.h"

#include "third_party/blink/public/platform/web_prerender.h"
#include "third_party/blink/public/web/web_prerenderer_client.h"
#include "third_party/blink/renderer/core/exported/web_view_impl.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/platform/prerender.h"

namespace blink {

// static
const char PrerendererClient::kSupplementName[] = "PrerendererClient";

// static
PrerendererClient* PrerendererClient::From(Page* page) {
  return Supplement<Page>::From<PrerendererClient>(page);
}

PrerendererClient::PrerendererClient(Page& page, WebPrerendererClient* client)
    : Supplement<Page>(page), client_(client) {}

void PrerendererClient::WillAddPrerender(Prerender* prerender) {
  if (!client_)
    return;
  WebPrerender web_prerender(prerender);
  client_->WillAddPrerender(&web_prerender);
}

bool PrerendererClient::IsPrefetchOnly() {
  return client_ && client_->IsPrefetchOnly();
}

void ProvidePrerendererClientTo(Page& page, PrerendererClient* client) {
  PrerendererClient::ProvideTo(page, client);
}

}  // namespace blink
