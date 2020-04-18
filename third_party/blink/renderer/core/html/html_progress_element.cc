/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "third_party/blink/renderer/core/html/html_progress_element.h"

#include "third_party/blink/renderer/bindings/core/v8/exception_messages.h"
#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/core/dom/shadow_root.h"
#include "third_party/blink/renderer/core/frame/use_counter.h"
#include "third_party/blink/renderer/core/html/parser/html_parser_idioms.h"
#include "third_party/blink/renderer/core/html/shadow/progress_shadow_element.h"
#include "third_party/blink/renderer/core/html_names.h"
#include "third_party/blink/renderer/core/layout/layout_progress.h"

namespace blink {

using namespace HTMLNames;

const double HTMLProgressElement::kIndeterminatePosition = -1;
const double HTMLProgressElement::kInvalidPosition = -2;

HTMLProgressElement::HTMLProgressElement(Document& document)
    : LabelableElement(progressTag, document), value_(nullptr) {
  UseCounter::Count(document, WebFeature::kProgressElement);
}

HTMLProgressElement::~HTMLProgressElement() = default;

HTMLProgressElement* HTMLProgressElement::Create(Document& document) {
  HTMLProgressElement* progress = new HTMLProgressElement(document);
  progress->EnsureUserAgentShadowRoot();
  return progress;
}

LayoutObject* HTMLProgressElement::CreateLayoutObject(
    const ComputedStyle& style) {
  if (!style.HasAppearance()) {
    UseCounter::Count(GetDocument(),
                      WebFeature::kProgressElementWithNoneAppearance);
    return LayoutObject::CreateObject(this, style);
  }
  UseCounter::Count(GetDocument(),
                    WebFeature::kProgressElementWithProgressBarAppearance);
  return new LayoutProgress(this);
}

LayoutProgress* HTMLProgressElement::GetLayoutProgress() const {
  if (GetLayoutObject() && GetLayoutObject()->IsProgress())
    return ToLayoutProgress(GetLayoutObject());
  return nullptr;
}

void HTMLProgressElement::ParseAttribute(
    const AttributeModificationParams& params) {
  if (params.name == valueAttr) {
    if (params.old_value.IsNull() != params.new_value.IsNull())
      PseudoStateChanged(CSSSelector::kPseudoIndeterminate);
    DidElementStateChange();
  } else if (params.name == maxAttr) {
    DidElementStateChange();
  } else {
    LabelableElement::ParseAttribute(params);
  }
}

void HTMLProgressElement::AttachLayoutTree(AttachContext& context) {
  LabelableElement::AttachLayoutTree(context);
  if (LayoutProgress* layout_progress = GetLayoutProgress())
    layout_progress->UpdateFromElement();
}

double HTMLProgressElement::value() const {
  double value = GetFloatingPointAttribute(valueAttr);
  // Otherwise, if the parsed value was greater than or equal to the maximum
  // value, then the current value of the progress bar is the maximum value
  // of the progress bar. Otherwise, if parsing the value attribute's value
  // resulted in an error, or a number less than or equal to zero, then the
  // current value of the progress bar is zero.
  return !std::isfinite(value) || value < 0 ? 0 : std::min(value, max());
}

void HTMLProgressElement::setValue(double value) {
  SetFloatingPointAttribute(valueAttr, std::max(value, 0.));
}

double HTMLProgressElement::max() const {
  double max = GetFloatingPointAttribute(maxAttr);
  // Otherwise, if the element has no max attribute, or if it has one but
  // parsing it resulted in an error, or if the parsed value was less than or
  // equal to zero, then the maximum value of the progress bar is 1.0.
  return !std::isfinite(max) || max <= 0 ? 1 : max;
}

void HTMLProgressElement::setMax(double max) {
  // FIXME: The specification says we should ignore the input value if it is
  // inferior or equal to 0.
  SetFloatingPointAttribute(maxAttr, max > 0 ? max : 1);
}

double HTMLProgressElement::position() const {
  if (!IsDeterminate())
    return HTMLProgressElement::kIndeterminatePosition;
  return value() / max();
}

bool HTMLProgressElement::IsDeterminate() const {
  return FastHasAttribute(valueAttr);
}

void HTMLProgressElement::DidElementStateChange() {
  SetValueWidthPercentage(position() * 100);
  if (LayoutProgress* layout_progress = GetLayoutProgress())
    layout_progress->UpdateFromElement();
}

void HTMLProgressElement::DidAddUserAgentShadowRoot(ShadowRoot& root) {
  DCHECK(!value_);

  ProgressShadowElement* inner = ProgressShadowElement::Create(GetDocument());
  inner->SetShadowPseudoId(AtomicString("-webkit-progress-inner-element"));
  root.AppendChild(inner);

  ProgressShadowElement* bar = ProgressShadowElement::Create(GetDocument());
  bar->SetShadowPseudoId(AtomicString("-webkit-progress-bar"));
  value_ = ProgressShadowElement::Create(GetDocument());
  value_->SetShadowPseudoId(AtomicString("-webkit-progress-value"));
  SetValueWidthPercentage(HTMLProgressElement::kIndeterminatePosition * 100);
  bar->AppendChild(value_);

  inner->AppendChild(bar);
}

bool HTMLProgressElement::ShouldAppearIndeterminate() const {
  return !IsDeterminate();
}

void HTMLProgressElement::Trace(blink::Visitor* visitor) {
  visitor->Trace(value_);
  LabelableElement::Trace(visitor);
}

void HTMLProgressElement::SetValueWidthPercentage(double width) const {
  value_->SetInlineStyleProperty(CSSPropertyWidth, width,
                                 CSSPrimitiveValue::UnitType::kPercentage);
}

}  // namespace blink
