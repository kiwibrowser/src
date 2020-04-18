// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_MAC_ATTRIBUTED_STRING_CODER_H_
#define CONTENT_COMMON_MAC_ATTRIBUTED_STRING_CODER_H_

#include <set>

#include "base/strings/string16.h"
#include "content/common/content_export.h"
#include "ipc/ipc_message_utils.h"
#include "ui/gfx/range/range.h"

#if __OBJC__
@class NSAttributedString;
@class NSDictionary;
#else
class NSAttributedString;
class NSDictionary;
#endif

namespace base {
class Pickle;
class PickleIterator;
}

namespace mac {

// This class will serialize the font information of an NSAttributedString so
// that it can be sent over IPC. This class only stores the information of the
// NSFontAttributeName. The motive is that of security: using NSArchiver and
// friends to send objects from the renderer to the browser could lead to
// deserialization of arbitrary objects. This class restricts serialization to
// a specific object class and specific attributes of that object.
class CONTENT_EXPORT AttributedStringCoder {
 public:
  // A C++ IPC-friendly representation of the NSFontAttributeName attribute
  // set.
  class FontAttribute {
   public:
    FontAttribute(NSDictionary* ns_attributes, gfx::Range effective_range);
    FontAttribute(const base::string16& font_name,
                  float font_point_size,
                  const gfx::Range& range);
    FontAttribute();
    ~FontAttribute();

    // Creates an autoreleased NSDictionary that can be attached to an
    // NSAttributedString.
    NSDictionary* ToAttributesDictionary() const;

    // Whether or not the attribute should be placed in the EncodedString. This
    // can return false, e.g. if the Cocoa-based constructor can't find any
    // information to encode.
    bool ShouldEncode() const;

    // Accessors:
    const base::string16& font_name() const { return font_name_; }
    float font_point_size() const { return font_point_size_; }
    const gfx::Range& effective_range() const { return effective_range_; }

   private:
    base::string16 font_name_;
    float font_point_size_;
    gfx::Range effective_range_;
  };

  // A class that contains the pertinent information from an NSAttributedString,
  // which can be serialized over IPC.
  class EncodedString {
   public:
    explicit EncodedString(base::string16 string);
    EncodedString();
    EncodedString(const EncodedString& other);
    EncodedString& operator=(const EncodedString& other);
    ~EncodedString();

    // Accessors:
    base::string16 string() const { return string_; }
    const std::vector<FontAttribute>& attributes() const {
      return attributes_;
    }
    std::vector<FontAttribute>* attributes() { return &attributes_; }

   private:
    // The plain-text string.
    base::string16 string_;
    // The set of attributes that style |string_|.
    std::vector<FontAttribute> attributes_;
  };

  // Takes an NSAttributedString, extracts the pertinent attributes, and returns
  // an object that represents it. Caller owns the result.
  static const EncodedString* Encode(NSAttributedString* str);

  // Returns an autoreleased NSAttributedString from an encoded representation.
  static NSAttributedString* Decode(const EncodedString* str);

 private:
  AttributedStringCoder();
};

}  // namespace mac

// IPC ParamTraits specialization //////////////////////////////////////////////

namespace IPC {

template <>
struct ParamTraits<mac::AttributedStringCoder::EncodedString> {
  typedef mac::AttributedStringCoder::EncodedString param_type;
  static void Write(base::Pickle* m, const param_type& p);
  static bool Read(const base::Pickle* m,
                   base::PickleIterator* iter,
                   param_type* r);
  static void Log(const param_type& p, std::string* l);
};

template <>
struct ParamTraits<mac::AttributedStringCoder::FontAttribute> {
  typedef mac::AttributedStringCoder::FontAttribute param_type;
  static void Write(base::Pickle* m, const param_type& p);
  static bool Read(const base::Pickle* m,
                   base::PickleIterator* iter,
                   param_type* r);
  static void Log(const param_type& p, std::string* l);
};

}  // namespace IPC

#endif  // CONTENT_COMMON_MAC_ATTRIBUTED_STRING_CODER_H_
