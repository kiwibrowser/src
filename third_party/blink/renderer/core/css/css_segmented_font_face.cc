/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/css/css_segmented_font_face.h"

#include "third_party/blink/renderer/core/css/css_font_face.h"
#include "third_party/blink/renderer/core/css/css_font_selector.h"
#include "third_party/blink/renderer/platform/fonts/font_cache.h"
#include "third_party/blink/renderer/platform/fonts/font_description.h"
#include "third_party/blink/renderer/platform/fonts/font_face_creation_params.h"
#include "third_party/blink/renderer/platform/fonts/segmented_font_data.h"
#include "third_party/blink/renderer/platform/fonts/simple_font_data.h"

namespace blink {

CSSSegmentedFontFace::CSSSegmentedFontFace(
    FontSelectionCapabilities font_selection_capabilities)
    : font_selection_capabilities_(font_selection_capabilities),
      first_non_css_connected_face_(font_faces_.end()),
      approximate_character_count_(0) {}

CSSSegmentedFontFace::~CSSSegmentedFontFace() = default;

void CSSSegmentedFontFace::PruneTable() {
  // Make sure the glyph page tree prunes out all uses of this custom font.
  if (font_data_table_.IsEmpty())
    return;

  font_data_table_.clear();
}

bool CSSSegmentedFontFace::IsValid() const {
  // Valid if at least one font face is valid.
  for (const auto& font_face : font_faces_) {
    if (font_face->CssFontFace()->IsValid())
      return true;
  }
  return false;
}

void CSSSegmentedFontFace::FontFaceInvalidated() {
  PruneTable();
}

void CSSSegmentedFontFace::AddFontFace(FontFace* font_face,
                                       bool css_connected) {
  PruneTable();
  font_face->CssFontFace()->SetSegmentedFontFace(this);
  if (css_connected) {
    font_faces_.InsertBefore(first_non_css_connected_face_, font_face);
  } else {
    // This is the only place in Blink that is using addReturnIterator.
    FontFaceList::iterator iterator = font_faces_.AddReturnIterator(font_face);
    if (first_non_css_connected_face_ == font_faces_.end())
      first_non_css_connected_face_ = iterator;
  }
}

void CSSSegmentedFontFace::RemoveFontFace(FontFace* font_face) {
  FontFaceList::iterator it = font_faces_.find(font_face);
  if (it == font_faces_.end())
    return;

  if (it == first_non_css_connected_face_)
    ++first_non_css_connected_face_;
  font_faces_.erase(it);

  PruneTable();
  font_face->CssFontFace()->ClearSegmentedFontFace();
}

scoped_refptr<FontData> CSSSegmentedFontFace::GetFontData(
    const FontDescription& font_description) {
  if (!IsValid())
    return nullptr;

  const FontSelectionRequest& font_selection_request =
      font_description.GetFontSelectionRequest();
  FontCacheKey key = font_description.CacheKey(FontFaceCreationParams(),
                                               font_selection_request);

  scoped_refptr<SegmentedFontData>& font_data =
      font_data_table_.insert(key, nullptr).stored_value->value;
  if (font_data && font_data->NumFaces()) {
    // No release, we have a reference to an object in the cache which should
    // retain the ref count it has.
    return font_data;
  }

  if (!font_data)
    font_data = SegmentedFontData::Create();

  FontDescription requested_font_description(font_description);
  if (!font_selection_capabilities_.HasRange()) {
    requested_font_description.SetSyntheticBold(
        font_selection_capabilities_.weight.maximum < BoldThreshold() &&
        font_selection_request.weight >= BoldThreshold());
    requested_font_description.SetSyntheticItalic(
        font_selection_capabilities_.slope.maximum == NormalSlopeValue() &&
        font_selection_request.slope == ItalicSlopeValue());
  }

  for (FontFaceList::reverse_iterator it = font_faces_.rbegin();
       it != font_faces_.rend(); ++it) {
    if (!(*it)->CssFontFace()->IsValid())
      continue;
    if (scoped_refptr<SimpleFontData> face_font_data =
            (*it)->CssFontFace()->GetFontData(requested_font_description)) {
      DCHECK(!face_font_data->IsSegmented());
      if (face_font_data->IsCustomFont()) {
        font_data->AppendFace(base::AdoptRef(new FontDataForRangeSet(
            std::move(face_font_data), (*it)->CssFontFace()->Ranges())));
      } else {
        font_data->AppendFace(base::AdoptRef(new FontDataForRangeSetFromCache(
            std::move(face_font_data), (*it)->CssFontFace()->Ranges())));
      }
    }
  }
  if (font_data->NumFaces()) {
    // No release, we have a reference to an object in the cache which should
    // retain the ref count it has.
    return font_data;
  }

  return nullptr;
}

void CSSSegmentedFontFace::WillUseFontData(
    const FontDescription& font_description,
    const String& text) {
  approximate_character_count_ += text.length();
  for (FontFaceList::reverse_iterator it = font_faces_.rbegin();
       it != font_faces_.rend(); ++it) {
    if ((*it)->LoadStatus() != FontFace::kUnloaded)
      break;
    if ((*it)->CssFontFace()->MaybeLoadFont(font_description, text))
      break;
  }
}

void CSSSegmentedFontFace::WillUseRange(
    const blink::FontDescription& font_description,
    const blink::FontDataForRangeSet& range_set) {
  // Iterating backwards since later defined unicode-range faces override
  // previously defined ones, according to the CSS3 fonts module.
  // https://drafts.csswg.org/css-fonts/#composite-fonts
  for (FontFaceList::reverse_iterator it = font_faces_.rbegin();
       it != font_faces_.rend(); ++it) {
    CSSFontFace* css_font_face = (*it)->CssFontFace();
    if (css_font_face->MaybeLoadFont(font_description, range_set))
      break;
  }
}

bool CSSSegmentedFontFace::CheckFont(const String& text) const {
  for (const auto& font_face : font_faces_) {
    if (font_face->LoadStatus() != FontFace::kLoaded &&
        font_face->CssFontFace()->Ranges()->IntersectsWith(text))
      return false;
  }
  return true;
}

void CSSSegmentedFontFace::Match(const String& text,
                                 HeapVector<Member<FontFace>>& faces) const {
  for (const auto& font_face : font_faces_) {
    if (font_face->CssFontFace()->Ranges()->IntersectsWith(text))
      faces.push_back(font_face);
  }
}

void CSSSegmentedFontFace::Trace(blink::Visitor* visitor) {
  visitor->Trace(first_non_css_connected_face_);
  visitor->Trace(font_faces_);
}

}  // namespace blink
