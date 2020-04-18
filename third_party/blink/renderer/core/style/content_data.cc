/*
 * Copyright (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights
 * reserved.
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

#include "third_party/blink/renderer/core/style/content_data.h"

#include <memory>
#include "third_party/blink/renderer/core/dom/pseudo_element.h"
#include "third_party/blink/renderer/core/layout/layout_counter.h"
#include "third_party/blink/renderer/core/layout/layout_image.h"
#include "third_party/blink/renderer/core/layout/layout_image_resource.h"
#include "third_party/blink/renderer/core/layout/layout_image_resource_style_image.h"
#include "third_party/blink/renderer/core/layout/layout_quote.h"
#include "third_party/blink/renderer/core/layout/layout_text_fragment.h"
#include "third_party/blink/renderer/core/style/computed_style.h"

namespace blink {

ContentData* ContentData::Create(StyleImage* image) {
  return new ImageContentData(image);
}

ContentData* ContentData::Create(const String& text) {
  return new TextContentData(text);
}

ContentData* ContentData::Create(std::unique_ptr<CounterContent> counter) {
  return new CounterContentData(std::move(counter));
}

ContentData* ContentData::Create(QuoteType quote) {
  return new QuoteContentData(quote);
}

ContentData* ContentData::Clone() const {
  ContentData* result = CloneInternal();

  ContentData* last_new_data = result;
  for (const ContentData* content_data = Next(); content_data;
       content_data = content_data->Next()) {
    ContentData* new_data = content_data->CloneInternal();
    last_new_data->SetNext(new_data);
    last_new_data = last_new_data->Next();
  }

  return result;
}

void ContentData::Trace(blink::Visitor* visitor) {
  visitor->Trace(next_);
}

LayoutObject* ImageContentData::CreateLayoutObject(
    PseudoElement& pseudo,
    ComputedStyle& pseudo_style) const {
  LayoutImage* image = LayoutImage::CreateAnonymous(pseudo);
  image->SetPseudoStyle(&pseudo_style);
  if (image_)
    image->SetImageResource(
        LayoutImageResourceStyleImage::Create(image_.Get()));
  else
    image->SetImageResource(LayoutImageResource::Create());
  return image;
}

void ImageContentData::Trace(blink::Visitor* visitor) {
  visitor->Trace(image_);
  ContentData::Trace(visitor);
}

LayoutObject* TextContentData::CreateLayoutObject(
    PseudoElement& pseudo,
    ComputedStyle& pseudo_style) const {
  LayoutObject* layout_object =
      LayoutTextFragment::CreateAnonymous(pseudo, text_.Impl());
  layout_object->SetPseudoStyle(&pseudo_style);
  return layout_object;
}

LayoutObject* CounterContentData::CreateLayoutObject(
    PseudoElement& pseudo,
    ComputedStyle& pseudo_style) const {
  LayoutObject* layout_object = new LayoutCounter(pseudo, *counter_);
  layout_object->SetPseudoStyle(&pseudo_style);
  return layout_object;
}

LayoutObject* QuoteContentData::CreateLayoutObject(
    PseudoElement& pseudo,
    ComputedStyle& pseudo_style) const {
  LayoutObject* layout_object = new LayoutQuote(pseudo, quote_);
  layout_object->SetPseudoStyle(&pseudo_style);
  return layout_object;
}

}  // namespace blink
