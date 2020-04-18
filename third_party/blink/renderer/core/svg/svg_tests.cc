/*
 * Copyright (C) 2004, 2005, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2007 Rob Buis <buis@kde.org>
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

#include "third_party/blink/renderer/core/svg/svg_tests.h"

#include "third_party/blink/renderer/core/svg/svg_element.h"
#include "third_party/blink/renderer/core/svg/svg_static_string_list.h"
#include "third_party/blink/renderer/core/svg_names.h"
#include "third_party/blink/renderer/platform/language.h"

namespace blink {

SVGTests::SVGTests(SVGElement* context_element)
    : required_extensions_(
          SVGStaticStringList::Create(context_element,
                                      SVGNames::requiredExtensionsAttr)),
      system_language_(
          SVGStaticStringList::Create(context_element,
                                      SVGNames::systemLanguageAttr)) {
  DCHECK(context_element);

  context_element->AddToPropertyMap(required_extensions_);
  context_element->AddToPropertyMap(system_language_);
}

void SVGTests::Trace(blink::Visitor* visitor) {
  visitor->Trace(required_extensions_);
  visitor->Trace(system_language_);
}

SVGStringListTearOff* SVGTests::requiredExtensions() {
  return required_extensions_->TearOff();
}

SVGStringListTearOff* SVGTests::systemLanguage() {
  return system_language_->TearOff();
}

bool SVGTests::IsValid() const {
  if (system_language_->IsSpecified()) {
    bool match_found = false;
    for (const auto& value : system_language_->Value()->Values()) {
      if (value.length() == 2 && DefaultLanguage().StartsWith(value)) {
        match_found = true;
        break;
      }
    }
    if (!match_found)
      return false;
  }

  if (!required_extensions_->Value()->Values().IsEmpty())
    return false;

  return true;
}

bool SVGTests::IsKnownAttribute(const QualifiedName& attr_name) {
  return attr_name == SVGNames::requiredExtensionsAttr ||
         attr_name == SVGNames::systemLanguageAttr;
}

}  // namespace blink
