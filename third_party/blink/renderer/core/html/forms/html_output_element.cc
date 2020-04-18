/*
 * Copyright (c) 2010 Google Inc. All rights reserved.
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

#include "third_party/blink/renderer/core/html/forms/html_output_element.h"

#include "third_party/blink/renderer/bindings/core/v8/exception_state.h"
#include "third_party/blink/renderer/core/html_names.h"

namespace blink {

inline HTMLOutputElement::HTMLOutputElement(Document& document)
    : HTMLFormControlElement(HTMLNames::outputTag, document),
      is_default_value_mode_(true),
      default_value_(""),
      tokens_(DOMTokenList::Create(*this, HTMLNames::forAttr)) {}

HTMLOutputElement::~HTMLOutputElement() = default;

HTMLOutputElement* HTMLOutputElement::Create(Document& document) {
  return new HTMLOutputElement(document);
}

const AtomicString& HTMLOutputElement::FormControlType() const {
  DEFINE_STATIC_LOCAL(const AtomicString, output, ("output"));
  return output;
}

bool HTMLOutputElement::IsDisabledFormControl() const {
  return false;
}

bool HTMLOutputElement::MatchesEnabledPseudoClass() const {
  return false;
}

bool HTMLOutputElement::SupportsFocus() const {
  return HTMLElement::SupportsFocus();
}

void HTMLOutputElement::ParseAttribute(
    const AttributeModificationParams& params) {
  if (params.name == HTMLNames::forAttr)
    tokens_->DidUpdateAttributeValue(params.old_value, params.new_value);
  else
    HTMLFormControlElement::ParseAttribute(params);
}

DOMTokenList* HTMLOutputElement::htmlFor() const {
  return tokens_.Get();
}

void HTMLOutputElement::ChildrenChanged(const ChildrenChange& change) {
  HTMLFormControlElement::ChildrenChanged(change);

  if (is_default_value_mode_)
    default_value_ = textContent();
}

void HTMLOutputElement::ResetImpl() {
  // The reset algorithm for output elements is to set the element's
  // value mode flag to "default" and then to set the element's textContent
  // attribute to the default value.
  if (default_value_ == value())
    return;
  setTextContent(default_value_);
  is_default_value_mode_ = true;
}

String HTMLOutputElement::value() const {
  return textContent();
}

void HTMLOutputElement::setValue(const String& value) {
  // The value mode flag set to "value" when the value attribute is set.
  is_default_value_mode_ = false;
  if (value == this->value())
    return;
  setTextContent(value);
}

String HTMLOutputElement::defaultValue() const {
  return default_value_;
}

void HTMLOutputElement::setDefaultValue(const String& value) {
  if (default_value_ == value)
    return;
  default_value_ = value;
  // The spec requires the value attribute set to the default value
  // when the element's value mode flag to "default".
  if (is_default_value_mode_)
    setTextContent(value);
}

int HTMLOutputElement::tabIndex() const {
  return HTMLElement::tabIndex();
}

void HTMLOutputElement::Trace(blink::Visitor* visitor) {
  visitor->Trace(tokens_);
  HTMLFormControlElement::Trace(visitor);
}

}  // namespace blink
