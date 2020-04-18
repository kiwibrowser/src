/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 *           (C) 2006 Alexey Proskuryakov (ap@webkit.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2012 Apple Inc. All
 * rights reserved.
 * Copyright (C) 2008, 2009 Torch Mobile Inc. All rights reserved.
 * (http://www.torchmobile.com/)
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "third_party/blink/renderer/core/dom/document_init.h"

#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/html/custom/v0_custom_element_registration_context.h"
#include "third_party/blink/renderer/core/html/html_frame_owner_element.h"
#include "third_party/blink/renderer/core/html/imports/html_imports_controller.h"
#include "third_party/blink/renderer/core/loader/document_loader.h"
#include "third_party/blink/renderer/platform/network/network_utils.h"

namespace blink {

// FIXME: Broken with OOPI.
static Document* ParentDocument(LocalFrame* frame) {
  DCHECK(frame);

  Element* owner_element = frame->DeprecatedLocalOwner();
  if (!owner_element)
    return nullptr;
  return &owner_element->GetDocument();
}

DocumentInit DocumentInit::Create() {
  return DocumentInit(nullptr);
}

DocumentInit DocumentInit::CreateWithImportsController(
    HTMLImportsController* controller) {
  DCHECK(controller);
  Document* master = controller->Master();
  return DocumentInit(controller)
      .WithContextDocument(master->ContextDocument())
      .WithRegistrationContext(master->RegistrationContext());
}

DocumentInit::DocumentInit(HTMLImportsController* imports_controller)
    : imports_controller_(imports_controller),
      create_new_registration_context_(false) {}

DocumentInit::DocumentInit(const DocumentInit&) = default;

DocumentInit::~DocumentInit() = default;

bool DocumentInit::ShouldSetURL() const {
  LocalFrame* frame = FrameForSecurityContext();
  return (frame && frame->Tree().Parent()) || !url_.IsEmpty();
}

bool DocumentInit::ShouldTreatURLAsSrcdocDocument() const {
  return parent_document_ &&
         frame_->Loader().ShouldTreatURLAsSrcdocDocument(url_);
}

LocalFrame* DocumentInit::FrameForSecurityContext() const {
  if (frame_)
    return frame_;
  if (imports_controller_)
    return imports_controller_->Master()->GetFrame();
  return nullptr;
}

SandboxFlags DocumentInit::GetSandboxFlags() const {
  DCHECK(FrameForSecurityContext());
  FrameLoader* loader = &FrameForSecurityContext()->Loader();
  SandboxFlags flags = loader->EffectiveSandboxFlags();

  // If the load was blocked by CSP, force the Document's origin to be unique,
  // so that the blocked document appears to be a normal cross-origin document's
  // load per CSP spec: https://www.w3.org/TR/CSP3/#directive-frame-ancestors.
  if (loader->GetDocumentLoader() &&
      loader->GetDocumentLoader()->WasBlockedAfterCSP()) {
    flags |= kSandboxOrigin;
  }

  return flags;
}

WebInsecureRequestPolicy DocumentInit::GetInsecureRequestPolicy() const {
  DCHECK(FrameForSecurityContext());
  return FrameForSecurityContext()->Loader().GetInsecureRequestPolicy();
}

SecurityContext::InsecureNavigationsSet*
DocumentInit::InsecureNavigationsToUpgrade() const {
  DCHECK(FrameForSecurityContext());
  return FrameForSecurityContext()->Loader().InsecureNavigationsToUpgrade();
}

bool DocumentInit::IsHostedInReservedIPRange() const {
  if (LocalFrame* frame = FrameForSecurityContext()) {
    if (DocumentLoader* loader =
            frame->Loader().GetProvisionalDocumentLoader()
                ? frame->Loader().GetProvisionalDocumentLoader()
                : frame->Loader().GetDocumentLoader()) {
      if (!loader->GetResponse().RemoteIPAddress().IsEmpty())
        return NetworkUtils::IsReservedIPAddress(
            loader->GetResponse().RemoteIPAddress());
    }
  }
  return false;
}

Settings* DocumentInit::GetSettings() const {
  DCHECK(FrameForSecurityContext());
  return FrameForSecurityContext()->GetSettings();
}

KURL DocumentInit::ParentBaseURL() const {
  return parent_document_->BaseURL();
}

DocumentInit& DocumentInit::WithFrame(LocalFrame* frame) {
  DCHECK(!frame_);
  DCHECK(!imports_controller_);
  frame_ = frame;
  if (frame_)
    parent_document_ = ParentDocument(frame_);
  return *this;
}

DocumentInit& DocumentInit::WithContextDocument(Document* context_document) {
  DCHECK(!context_document_);
  context_document_ = context_document;
  return *this;
}

DocumentInit& DocumentInit::WithURL(const KURL& url) {
  DCHECK(url_.IsNull());
  url_ = url;
  return *this;
}

DocumentInit& DocumentInit::WithOwnerDocument(Document* owner_document) {
  DCHECK(!owner_document_);
  owner_document_ = owner_document;
  return *this;
}

DocumentInit& DocumentInit::WithRegistrationContext(
    V0CustomElementRegistrationContext* registration_context) {
  DCHECK(!create_new_registration_context_);
  DCHECK(!registration_context_);
  registration_context_ = registration_context;
  return *this;
}

DocumentInit& DocumentInit::WithNewRegistrationContext() {
  DCHECK(!create_new_registration_context_);
  DCHECK(!registration_context_);
  create_new_registration_context_ = true;
  return *this;
}

V0CustomElementRegistrationContext* DocumentInit::RegistrationContext(
    Document* document) const {
  if (!document->IsHTMLDocument() && !document->IsXHTMLDocument())
    return nullptr;

  if (create_new_registration_context_)
    return V0CustomElementRegistrationContext::Create();

  return registration_context_.Get();
}

Document* DocumentInit::ContextDocument() const {
  return context_document_;
}

}  // namespace blink
