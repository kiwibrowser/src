/*
 *  Copyright (C) 2003, 2006, 2008 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301 USA
 */

#include "third_party/blink/renderer/core/xml/dom_parser.h"

#include "third_party/blink/renderer/core/dom/dom_implementation.h"
#include "third_party/blink/renderer/platform/weborigin/security_origin.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

Document* DOMParser::parseFromString(const String& str, const String& type) {
  Document* doc = DOMImplementation::createDocument(
      type, DocumentInit::Create().WithContextDocument(context_document_),
      false);
  doc->SetContent(str);
  doc->SetMimeType(AtomicString(type));
  if (context_document_) {
    doc->SetURL(context_document_->Url());
    doc->SetSecurityOrigin(context_document_->GetMutableSecurityOrigin());
  }
  return doc;
}

DOMParser::DOMParser(Document& document)
    : context_document_(document.ContextDocument()) {}

void DOMParser::Trace(blink::Visitor* visitor) {
  visitor->Trace(context_document_);
  ScriptWrappable::Trace(visitor);
}

}  // namespace blink
