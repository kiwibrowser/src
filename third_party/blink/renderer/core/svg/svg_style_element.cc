/*
 * Copyright (C) 2004, 2005 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2007 Rob Buis <buis@kde.org>
 * Copyright (C) 2006 Apple Inc. All rights reserved.
 * Copyright (C) 2009 Cameron McCormack <cam@mcc.id.au>
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
 */

#include "third_party/blink/renderer/core/svg/svg_style_element.h"

#include "third_party/blink/public/platform/task_type.h"
#include "third_party/blink/renderer/core/css/css_style_sheet.h"
#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/core/media_type_names.h"
#include "third_party/blink/renderer/platform/wtf/std_lib_extras.h"

namespace blink {

inline SVGStyleElement::SVGStyleElement(Document& document,
                                        const CreateElementFlags flags)
    : SVGElement(SVGNames::styleTag, document),
      StyleElement(&document, flags.IsCreatedByParser()) {}

SVGStyleElement::~SVGStyleElement() = default;

SVGStyleElement* SVGStyleElement::Create(Document& document,
                                         const CreateElementFlags flags) {
  return new SVGStyleElement(document, flags);
}

bool SVGStyleElement::disabled() const {
  if (!sheet_)
    return false;

  return sheet_->disabled();
}

void SVGStyleElement::setDisabled(bool set_disabled) {
  if (CSSStyleSheet* style_sheet = sheet())
    style_sheet->setDisabled(set_disabled);
}

const AtomicString& SVGStyleElement::type() const {
  DEFINE_STATIC_LOCAL(const AtomicString, default_value, ("text/css"));
  const AtomicString& n = getAttribute(SVGNames::typeAttr);
  return n.IsNull() ? default_value : n;
}

void SVGStyleElement::setType(const AtomicString& type) {
  setAttribute(SVGNames::typeAttr, type);
}

const AtomicString& SVGStyleElement::media() const {
  const AtomicString& n = FastGetAttribute(SVGNames::mediaAttr);
  return n.IsNull() ? MediaTypeNames::all : n;
}

void SVGStyleElement::setMedia(const AtomicString& media) {
  setAttribute(SVGNames::mediaAttr, media);
}

String SVGStyleElement::title() const {
  return FastGetAttribute(SVGNames::titleAttr);
}

void SVGStyleElement::setTitle(const AtomicString& title) {
  setAttribute(SVGNames::titleAttr, title);
}

void SVGStyleElement::ParseAttribute(
    const AttributeModificationParams& params) {
  if (params.name == SVGNames::titleAttr) {
    if (sheet_ && IsInDocumentTree())
      sheet_->SetTitle(params.new_value);

    return;
  }

  SVGElement::ParseAttribute(params);
}

void SVGStyleElement::FinishParsingChildren() {
  StyleElement::ProcessingResult result =
      StyleElement::FinishParsingChildren(*this);
  SVGElement::FinishParsingChildren();
  if (result == StyleElement::kProcessingFatalError)
    NotifyLoadedSheetAndAllCriticalSubresources(
        kErrorOccurredLoadingSubresource);
}

Node::InsertionNotificationRequest SVGStyleElement::InsertedInto(
    ContainerNode* insertion_point) {
  SVGElement::InsertedInto(insertion_point);
  return kInsertionShouldCallDidNotifySubtreeInsertions;
}

void SVGStyleElement::DidNotifySubtreeInsertionsToDocument() {
  if (StyleElement::ProcessStyleSheet(GetDocument(), *this) ==
      StyleElement::kProcessingFatalError)
    NotifyLoadedSheetAndAllCriticalSubresources(
        kErrorOccurredLoadingSubresource);
}

void SVGStyleElement::RemovedFrom(ContainerNode* insertion_point) {
  SVGElement::RemovedFrom(insertion_point);
  StyleElement::RemovedFrom(*this, insertion_point);
}

void SVGStyleElement::ChildrenChanged(const ChildrenChange& change) {
  SVGElement::ChildrenChanged(change);
  if (StyleElement::ChildrenChanged(*this) ==
      StyleElement::kProcessingFatalError)
    NotifyLoadedSheetAndAllCriticalSubresources(
        kErrorOccurredLoadingSubresource);
}

void SVGStyleElement::NotifyLoadedSheetAndAllCriticalSubresources(
    LoadedSheetErrorStatus error_status) {
  if (error_status != kNoErrorLoadingSubresource) {
    GetDocument()
        .GetTaskRunner(TaskType::kDOMManipulation)
        ->PostTask(FROM_HERE, WTF::Bind(&SVGStyleElement::DispatchPendingEvent,
                                        WrapPersistent(this)));
  }
}

void SVGStyleElement::DispatchPendingEvent() {
  DispatchEvent(Event::Create(EventTypeNames::error));
}

void SVGStyleElement::Trace(blink::Visitor* visitor) {
  StyleElement::Trace(visitor);
  SVGElement::Trace(visitor);
}

}  // namespace blink
