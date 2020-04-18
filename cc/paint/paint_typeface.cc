// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/paint/paint_typeface.h"
#include "build/build_config.h"
#include "third_party/skia/include/ports/SkFontConfigInterface.h"
#include "third_party/skia/include/ports/SkFontMgr.h"

namespace cc {

// static
PaintTypeface PaintTypeface::TestTypeface() {
  PaintTypeface typeface;
  typeface.type_ = Type::kTestTypeface;
  typeface.sk_typeface_ = SkTypeface::MakeDefault();
  typeface.CreateSkTypeface();
  return typeface;
}

// static
PaintTypeface PaintTypeface::FromSkTypeface(const sk_sp<SkTypeface>& tf) {
  PaintTypeface typeface;
  typeface.type_ = Type::kSkTypeface;
  typeface.sk_typeface_ = tf;
  typeface.CreateSkTypeface();
  return typeface;
}

// static
PaintTypeface PaintTypeface::FromFontConfigInterfaceIdAndTtcIndex(
    int config_id,
    int ttc_index) {
  PaintTypeface typeface;
  typeface.type_ = Type::kFontConfigInterfaceIdAndTtcIndex;
  typeface.font_config_interface_id_ = config_id;
  typeface.ttc_index_ = ttc_index;
  typeface.CreateSkTypeface();
  return typeface;
}

// static
PaintTypeface PaintTypeface::FromFilenameAndTtcIndex(
    const std::string& filename,
    int ttc_index) {
  PaintTypeface typeface;
  typeface.type_ = Type::kFilenameAndTtcIndex;
  typeface.filename_ = filename;
  typeface.ttc_index_ = ttc_index;
  typeface.CreateSkTypeface();
  return typeface;
}

// static
PaintTypeface PaintTypeface::FromFamilyNameAndFontStyle(
    const std::string& family_name,
    const SkFontStyle& font_style) {
  PaintTypeface typeface;
  typeface.type_ = Type::kFamilyNameAndFontStyle;
  typeface.family_name_ = family_name;
  typeface.font_style_ = font_style;
  typeface.CreateSkTypeface();
  return typeface;
}

PaintTypeface::PaintTypeface() = default;
PaintTypeface::PaintTypeface(const PaintTypeface& other) {
  *this = other;
}
PaintTypeface::PaintTypeface(PaintTypeface&& other) = default;
PaintTypeface::~PaintTypeface() {
  // TODO(crbug.com/785682): Debugging possible ref counting issue.
  if (sk_typeface_) {
    // We should be a strong owner of this reference. However, to debug a
    // possible ref counting issue, we also ensured that we're a weak owner of
    // this as well. This means that we should be able to always acquire a
    // strong reference (ie the class isn't deleted because we're weak owners,
    // and it should always succeed because we're strong owners).
    if (sk_typeface_->try_ref()) {
      sk_typeface_->unref();
    } else {
      CHECK(false) << "SkTypeface ref-counting problem detected.";
    }
    sk_typeface_->weak_unref();
  }
  // Explicitly delete the typeface so that any crashes resulting from this
  // would point to this line.
  sk_typeface_ = nullptr;
}

PaintTypeface& PaintTypeface::operator=(const PaintTypeface& other) {
  sk_id_ = other.sk_id_;
  sk_typeface_ = other.sk_typeface_;

  // TODO(crbug.com/785682): Debugging possible ref counting issue.
  if (sk_typeface_) {
    // Since we're copying this object which will do a weak unref in the dtor,
    // ensure to bump the weak ref one more time.
    sk_typeface_->weak_ref();
  }

  type_ = other.type_;
  font_config_interface_id_ = other.font_config_interface_id_;
  ttc_index_ = other.ttc_index_;
  filename_ = other.filename_;
  family_name_ = other.family_name_;
  font_style_ = other.font_style_;
  return *this;
}

PaintTypeface& PaintTypeface::operator=(PaintTypeface&& other) = default;

void PaintTypeface::CreateSkTypeface() {
// MacOS doesn't support this type of creation and relies on NSFonts instead.
#if !defined(OS_MACOSX)
  switch (type_) {
    case Type::kTestTypeface:
      // Nothing to do here.
      break;
    case Type::kSkTypeface:
      // Nothing to do here.
      break;
    case Type::kFontConfigInterfaceIdAndTtcIndex: {
// Mimic FontCache::CreateTypeface defines to ensure same behavior.
#if !defined(OS_WIN) && !defined(OS_ANDROID) && !defined(OS_FUCHSIA)
      sk_sp<SkFontConfigInterface> fci(SkFontConfigInterface::RefGlobal());
      SkFontConfigInterface::FontIdentity font_identity;
      font_identity.fID = font_config_interface_id_;
      font_identity.fTTCIndex = ttc_index_;
      sk_typeface_ = fci->makeTypeface(font_identity);
#endif
      break;
    }
    case Type::kFilenameAndTtcIndex:
// Mimic FontCache::CreateTypeface defines to ensure same behavior.
#if !defined(OS_WIN) && !defined(OS_ANDROID) && !defined(OS_FUCHSIA)
      sk_typeface_ = SkTypeface::MakeFromFile(filename_.c_str(), ttc_index_);
#endif
      break;
    case Type::kFamilyNameAndFontStyle: {
      // This is a fallthrough in all cases in FontCache::CreateTypeface, so
      // this is done unconditionally. Since we create the typeface upon
      // PaintTypeface creation, this should be safe in all cases.
      auto fm(SkFontMgr::RefDefault());
      sk_typeface_ = fm->legacyMakeTypeface(family_name_.c_str(), font_style_);
      break;
    }
  }
#endif  // !defined(OS_MACOSX)
  sk_id_ = sk_typeface_ ? sk_typeface_->uniqueID() : 0;

  // TODO(crbug.com/785682): Debugging possible ref counting issue.
  if (sk_typeface_)
    sk_typeface_->weak_ref();
}

}  // namespace cc
