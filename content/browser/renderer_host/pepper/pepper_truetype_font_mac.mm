// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/pepper/pepper_truetype_font.h"

#import <ApplicationServices/ApplicationServices.h>
#include <stddef.h>
#include <stdint.h>

#include <stdio.h>

#include "base/compiler_specific.h"
#include "base/mac/foundation_util.h"
#include "base/mac/scoped_cftyperef.h"
#include "base/mac/scoped_nsautorelease_pool.h"
#include "base/macros.h"
#include "base/numerics/safe_conversions.h"
#include "base/strings/sys_string_conversions.h"
#include "base/sys_byteorder.h"
#include "ppapi/c/dev/ppb_truetype_font_dev.h"
#include "ppapi/c/pp_errors.h"

namespace content {

namespace {

static bool FindFloat(CFDictionaryRef dict, CFStringRef name, float* value) {
  CFNumberRef num;
  return CFDictionaryGetValueIfPresent(
             dict, name, reinterpret_cast<const void**>(&num)) &&
         CFNumberIsFloatType(num) &&
         CFNumberGetValue(num, kCFNumberFloatType, value);
}

float GetMacWeight(PP_TrueTypeFontWeight_Dev weight) {
  // Map values from NORMAL (400) to HEAVY (900) to the range [0 .. 1], and
  // values below NORMAL to the range [-0.6 .. 0]. NORMAL should map to 0.
  float normal = PP_TRUETYPEFONTWEIGHT_NORMAL;
  float heavy = PP_TRUETYPEFONTWEIGHT_HEAVY;
  return (weight - normal) / (heavy - normal);
}

PP_TrueTypeFontWeight_Dev GetPepperWeight(float weight) {
  // Perform the inverse mapping of GetMacWeight.
  return static_cast<PP_TrueTypeFontWeight_Dev>(
      weight * (PP_TRUETYPEFONTWEIGHT_HEAVY - PP_TRUETYPEFONTWEIGHT_NORMAL) +
      PP_TRUETYPEFONTWEIGHT_NORMAL);
}

float GetMacWidth(PP_TrueTypeFontWidth_Dev width) {
  // Map values from NORMAL (4) to ULTRA_EXPANDED (8) to the range [0 .. 1],
  // and values below NORMAL to the range [-1 .. 0]. Normal should map to 0.
  float normal = PP_TRUETYPEFONTWIDTH_NORMAL;
  float ultra_expanded = PP_TRUETYPEFONTWIDTH_ULTRAEXPANDED;
  return (width - normal) / (ultra_expanded - normal);
}

PP_TrueTypeFontWidth_Dev GetPepperWidth(float width) {
  // Perform the inverse mapping of GetMacWeight.
  return static_cast<PP_TrueTypeFontWidth_Dev>(
      width *
          (PP_TRUETYPEFONTWIDTH_ULTRAEXPANDED - PP_TRUETYPEFONTWIDTH_NORMAL) +
      PP_TRUETYPEFONTWIDTH_NORMAL);
}

#define MAKE_TABLE_TAG(a, b, c, d) ((a) << 24) + ((b) << 16) + ((c) << 8) + (d)

// TrueType font header and table entry structs. See
// https://developer.apple.com/fonts/TTRefMan/RM06/Chap6.html
struct FontHeader {
  int32_t font_type;
  uint16_t num_tables;
  uint16_t search_range;
  uint16_t entry_selector;
  uint16_t range_shift;
};
static_assert(sizeof(FontHeader) == 12, "FontHeader wrong size");

struct FontDirectoryEntry {
  uint32_t tag;
  uint32_t checksum;
  uint32_t offset;
  uint32_t logical_length;
};
static_assert(sizeof(FontDirectoryEntry) == 16,
              "FontDirectoryEntry wrong size");

uint32_t CalculateChecksum(char* table, int32_t table_length) {
  uint32_t sum = 0;
  uint32_t* current = reinterpret_cast<uint32_t*>(table);
  uint32_t length = (table_length + 3) / 4;
  // Raw font data is big-endian.
  while (length-- > 0)
    sum += base::NetToHost32(*current++);
  return sum;
}

class PepperTrueTypeFontMac : public PepperTrueTypeFont {
 public:
  PepperTrueTypeFontMac();

  // PepperTrueTypeFont implementation.
  int32_t Initialize(ppapi::proxy::SerializedTrueTypeFontDesc* desc) override;
  int32_t GetTableTags(std::vector<uint32_t>* tags) override;
  int32_t GetTable(uint32_t table_tag,
                   int32_t offset,
                   int32_t max_data_length,
                   std::string* data) override;

 private:
  ~PepperTrueTypeFontMac() override;

  virtual int32_t GetEntireFont(int32_t offset,
                                int32_t max_data_length,
                                std::string* data);

  base::ScopedCFTypeRef<CTFontRef> font_ref_;

  DISALLOW_COPY_AND_ASSIGN(PepperTrueTypeFontMac);
};

PepperTrueTypeFontMac::PepperTrueTypeFontMac() {
}

PepperTrueTypeFontMac::~PepperTrueTypeFontMac() {
}

int32_t PepperTrueTypeFontMac::Initialize(
    ppapi::proxy::SerializedTrueTypeFontDesc* desc) {
  // Create the font in a nested scope, so we can use the same variable names
  // when we get the actual font characteristics.
  {
    // Create attributes and traits dictionaries.
    base::ScopedCFTypeRef<CFMutableDictionaryRef> attributes_ref(
        CFDictionaryCreateMutable(kCFAllocatorDefault,
                                  0,
                                  &kCFTypeDictionaryKeyCallBacks,
                                  &kCFTypeDictionaryValueCallBacks));

    base::ScopedCFTypeRef<CFMutableDictionaryRef> traits_ref(
        CFDictionaryCreateMutable(kCFAllocatorDefault,
                                  0,
                                  &kCFTypeDictionaryKeyCallBacks,
                                  &kCFTypeDictionaryValueCallBacks));
    if (!attributes_ref || !traits_ref)
      return PP_ERROR_FAILED;

    CFDictionaryAddValue(attributes_ref, kCTFontTraitsAttribute, traits_ref);

    // Use symbolic traits to specify traits when possible.
    CTFontSymbolicTraits symbolic_traits = 0;
    if (desc->style & PP_TRUETYPEFONTSTYLE_ITALIC)
      symbolic_traits |= kCTFontItalicTrait;
    if (desc->weight == PP_TRUETYPEFONTWEIGHT_BOLD)
      symbolic_traits |= kCTFontBoldTrait;
    if (desc->width == PP_TRUETYPEFONTWIDTH_CONDENSED)
      symbolic_traits |= kCTFontCondensedTrait;
    else if (desc->width == PP_TRUETYPEFONTWIDTH_EXPANDED)
      symbolic_traits |= kCTFontExpandedTrait;

    base::ScopedCFTypeRef<CFNumberRef> symbolic_traits_ref(CFNumberCreate(
        kCFAllocatorDefault, kCFNumberSInt32Type, &symbolic_traits));
    if (!symbolic_traits_ref)
      return PP_ERROR_FAILED;
    CFDictionaryAddValue(traits_ref, kCTFontSymbolicTrait, symbolic_traits_ref);

    // Font family matching doesn't work using family classes in symbolic
    // traits. Instead, map generic_family to font families that are always
    // available.
    std::string family(desc->family);
    if (family.empty()) {
      switch (desc->generic_family) {
        case PP_TRUETYPEFONTFAMILY_SERIF:
          family = "Times";
          break;
        case PP_TRUETYPEFONTFAMILY_SANSSERIF:
          family = "Helvetica";
          break;
        case PP_TRUETYPEFONTFAMILY_CURSIVE:
          family = "Apple Chancery";
          break;
        case PP_TRUETYPEFONTFAMILY_FANTASY:
          family = "Papyrus";
          break;
        case PP_TRUETYPEFONTFAMILY_MONOSPACE:
          family = "Courier";
          break;
      }
    }

    base::ScopedCFTypeRef<CFStringRef> name_ref(
        base::SysUTF8ToCFStringRef(family));
    if (name_ref)
      CFDictionaryAddValue(
          attributes_ref, kCTFontFamilyNameAttribute, name_ref);

    if (desc->weight != PP_TRUETYPEFONTWEIGHT_NORMAL &&
        desc->weight != PP_TRUETYPEFONTWEIGHT_BOLD) {
      float weight = GetMacWeight(desc->weight);
      base::ScopedCFTypeRef<CFNumberRef> weight_trait_ref(
          CFNumberCreate(kCFAllocatorDefault, kCFNumberFloat32Type, &weight));
      if (weight_trait_ref)
        CFDictionaryAddValue(traits_ref, kCTFontWeightTrait, weight_trait_ref);
    }

    if (desc->width != PP_TRUETYPEFONTWIDTH_NORMAL &&
        desc->width != PP_TRUETYPEFONTWIDTH_CONDENSED &&
        desc->width != PP_TRUETYPEFONTWIDTH_EXPANDED) {
      float width = GetMacWidth(desc->width);
      base::ScopedCFTypeRef<CFNumberRef> width_trait_ref(
          CFNumberCreate(kCFAllocatorDefault, kCFNumberFloat32Type, &width));
      if (width_trait_ref)
        CFDictionaryAddValue(traits_ref, kCTFontWidthTrait, width_trait_ref);
    }

    base::ScopedCFTypeRef<CTFontDescriptorRef> desc_ref(
        CTFontDescriptorCreateWithAttributes(attributes_ref));

    if (desc_ref)
      font_ref_.reset(CTFontCreateWithFontDescriptor(desc_ref, 0, NULL));

    if (!font_ref_.get())
      return PP_ERROR_FAILED;
  }

  // Now query to get the actual font characteristics.
  base::ScopedCFTypeRef<CTFontDescriptorRef> desc_ref(
      CTFontCopyFontDescriptor(font_ref_));

  base::ScopedCFTypeRef<CFStringRef> family_name_ref(
      base::mac::CFCast<CFStringRef>(
          CTFontDescriptorCopyAttribute(desc_ref, kCTFontFamilyNameAttribute)));
  desc->family = base::SysCFStringRefToUTF8(family_name_ref);

  base::ScopedCFTypeRef<CFDictionaryRef> traits_ref(
      base::mac::CFCast<CFDictionaryRef>(
          CTFontDescriptorCopyAttribute(desc_ref, kCTFontTraitsAttribute)));

  desc->style = PP_TRUETYPEFONTSTYLE_NORMAL;
  CTFontSymbolicTraits symbolic_traits(CTFontGetSymbolicTraits(font_ref_));
  if (symbolic_traits & kCTFontItalicTrait)
    desc->style = static_cast<PP_TrueTypeFontStyle_Dev>(
        desc->style | PP_TRUETYPEFONTSTYLE_ITALIC);
  if (symbolic_traits & kCTFontBoldTrait) {
    desc->weight = PP_TRUETYPEFONTWEIGHT_BOLD;
  } else {
    float weight;
    if (FindFloat(traits_ref, kCTFontWeightTrait, &weight))
      desc->weight = GetPepperWeight(weight);
  }
  if (symbolic_traits & kCTFontCondensedTrait) {
    desc->width = PP_TRUETYPEFONTWIDTH_CONDENSED;
  } else if (symbolic_traits & kCTFontExpandedTrait) {
    desc->width = PP_TRUETYPEFONTWIDTH_EXPANDED;
  } else {
    float width;
    if (FindFloat(traits_ref, kCTFontWidthTrait, &width))
      desc->width = GetPepperWidth(width);
  }

  // Character set isn't supported on Mac.
  desc->charset = PP_TRUETYPEFONTCHARSET_DEFAULT;
  return PP_OK;
}

int32_t PepperTrueTypeFontMac::GetTableTags(std::vector<uint32_t>* tags) {
  if (!font_ref_.get())
    return PP_ERROR_FAILED;
  base::ScopedCFTypeRef<CFArrayRef> tag_array(
      CTFontCopyAvailableTables(font_ref_, kCTFontTableOptionNoOptions));
  if (!tag_array)
    return PP_ERROR_FAILED;

  // Items returned by CTFontCopyAvailableTables are not boxed. Whose bright
  // idea was this?
  CFIndex length = CFArrayGetCount(tag_array);
  tags->resize(length);
  for (CFIndex i = 0; i < length; ++i) {
    (*tags)[i] =
        reinterpret_cast<uintptr_t>(CFArrayGetValueAtIndex(tag_array, i));
  }
  return length;
}

int32_t PepperTrueTypeFontMac::GetTable(uint32_t table_tag,
                                        int32_t offset,
                                        int32_t max_data_length,
                                        std::string* data) {
  if (!font_ref_.get())
    return PP_ERROR_FAILED;

  if (!table_tag)
    return GetEntireFont(offset, max_data_length, data);

  base::ScopedCFTypeRef<CFDataRef> table_ref(
      CTFontCopyTable(font_ref_,
                      static_cast<CTFontTableTag>(table_tag),
                      kCTFontTableOptionNoOptions));
  if (!table_ref)
    return PP_ERROR_FAILED;

  CFIndex table_size = CFDataGetLength(table_ref);
  CFIndex safe_offset =
      std::min(base::checked_cast<CFIndex>(offset), table_size);
  CFIndex safe_length = std::min(table_size - safe_offset,
                                 base::checked_cast<CFIndex>(max_data_length));
  data->resize(safe_length);
  CFDataGetBytes(table_ref,
                 CFRangeMake(safe_offset, safe_length),
                 reinterpret_cast<UInt8*>(&(*data)[0]));

  return safe_length;
}

int32_t PepperTrueTypeFontMac::GetEntireFont(int32_t offset,
                                             int32_t max_data_length,
                                             std::string* data) {
  // Reconstruct the font header, table directory, and tables.
  std::vector<uint32_t> table_tags;
  int32_t table_count = GetTableTags(&table_tags);
  if (table_count < 0)
    return table_count;  // PPAPI error code.

  // Allocate enough room for the header and the table directory entries.
  std::string font(
      sizeof(FontHeader) + sizeof(FontDirectoryEntry) * table_count, 0);
  // Map the OS X font type value to a TrueType scalar type.
  base::ScopedCFTypeRef<CFNumberRef> font_type_ref(
      base::mac::CFCast<CFNumberRef>(
          CTFontCopyAttribute(font_ref_, kCTFontFormatAttribute)));
  int32_t font_type;
  CFNumberGetValue(font_type_ref, kCFNumberSInt32Type, &font_type);
  switch (font_type) {
    case kCTFontFormatOpenTypePostScript:
      font_type = MAKE_TABLE_TAG('O', 'T', 'T', 'O');
      break;
    case kCTFontFormatTrueType:
    case kCTFontFormatBitmap:
      font_type = MAKE_TABLE_TAG('t', 'r', 'u', 'e');
      break;
    case kCTFontFormatPostScript:
      font_type = MAKE_TABLE_TAG('t', 'y', 'p', '1');
      break;
    case kCTFontFormatOpenTypeTrueType:
    case kCTFontFormatUnrecognized:
    default:
      font_type = MAKE_TABLE_TAG(0, 1, 0, 0);
      break;
  }

  // Calculate the rest of the header values.
  uint16_t num_tables = base::checked_cast<uint16_t>(table_count);
  uint16_t entry_selector = 0;
  uint16_t search_range = 1;
  while (search_range < (num_tables >> 1)) {
    entry_selector++;
    search_range <<= 1;
  }
  search_range <<= 4;
  uint16_t range_shift = (num_tables << 4) - search_range;

  // Write the header, with values in big-endian order.
  FontHeader* font_header = reinterpret_cast<FontHeader*>(&font[0]);
  font_header->font_type = base::HostToNet32(font_type);
  font_header->num_tables = base::HostToNet16(num_tables);
  font_header->search_range = base::HostToNet16(search_range);
  font_header->entry_selector = base::HostToNet16(entry_selector);
  font_header->range_shift = base::HostToNet16(range_shift);

  for (int32_t i = 0; i < table_count; i++) {
    // Get the table data.
    std::string table;
    int32_t table_size =
        GetTable(table_tags[i], 0, std::numeric_limits<int32_t>::max(), &table);
    if (table_size < 0)
      return table_size;  // PPAPI error code.

    // Append it to the font data so far, and zero pad so tables stay aligned.
    size_t table_offset = font.size();
    font.append(table);
    size_t padding = font.size() & 0x3;
    font.append(padding, 0);

    // Fill in the directory entry for this table.
    FontDirectoryEntry* entry = reinterpret_cast<FontDirectoryEntry*>(
        &font[0] + sizeof(FontHeader) + i * sizeof(FontDirectoryEntry));
    entry->tag = base::HostToNet32(table_tags[i]);
    entry->checksum =
        base::HostToNet32(CalculateChecksum(&font[table_offset], table_size));
    entry->offset = base::HostToNet32(table_offset);
    entry->logical_length = base::HostToNet32(table_size);
    // TODO(bbudge) set the 'head' table checksumAdjustment.
  }

  // Extract a substring if the caller specified an offset or max data length.
  int32_t font_size = base::checked_cast<int32_t>(font.size());
  int32_t safe_offset = std::min(offset, font_size);
  int32_t safe_length = std::min(font_size - safe_offset, max_data_length);
  if (safe_offset || safe_length != font_size)
    font = font.substr(safe_offset, safe_length);

  data->clear();
  data->swap(font);
  return safe_length;
}

}  // namespace

// static
PepperTrueTypeFont* PepperTrueTypeFont::Create() {
  return new PepperTrueTypeFontMac();
}

}  // namespace content
