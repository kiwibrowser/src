// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/pepper/pepper_truetype_font.h"

#include <stddef.h>
#include <stdint.h>

#include <memory>

#include "base/compiler_specific.h"
#include "base/files/scoped_file.h"
#include "base/macros.h"
#include "base/numerics/safe_conversions.h"
#include "base/sys_byteorder.h"
#include "content/browser/renderer_host/font_utils_linux.h"
#include "content/public/common/common_sandbox_support_linux.h"
#include "ppapi/c/dev/ppb_truetype_font_dev.h"
#include "ppapi/c/pp_errors.h"
#include "ppapi/c/trusted/ppb_browser_font_trusted.h"

namespace content {

namespace {

class PepperTrueTypeFontLinux : public PepperTrueTypeFont {
 public:
  PepperTrueTypeFontLinux();

  // PepperTrueTypeFont implementation.
  int32_t Initialize(ppapi::proxy::SerializedTrueTypeFontDesc* desc) override;
  int32_t GetTableTags(std::vector<uint32_t>* tags) override;
  int32_t GetTable(uint32_t table_tag,
                   int32_t offset,
                   int32_t max_data_length,
                   std::string* data) override;

 private:
  ~PepperTrueTypeFontLinux() override;

  base::ScopedFD fd_;

  DISALLOW_COPY_AND_ASSIGN(PepperTrueTypeFontLinux);
};

PepperTrueTypeFontLinux::PepperTrueTypeFontLinux() {
}

PepperTrueTypeFontLinux::~PepperTrueTypeFontLinux() {
}

int32_t PepperTrueTypeFontLinux::Initialize(
    ppapi::proxy::SerializedTrueTypeFontDesc* desc) {
  // If no face is provided, convert family to the platform defaults. These
  // names should be mapped by FontConfig to an appropriate default font.
  if (desc->family.empty()) {
    switch (desc->generic_family) {
      case PP_TRUETYPEFONTFAMILY_SERIF:
        desc->family = "serif";
        break;
      case PP_TRUETYPEFONTFAMILY_SANSSERIF:
        desc->family = "sans-serif";
        break;
      case PP_TRUETYPEFONTFAMILY_CURSIVE:
        desc->family = "cursive";
        break;
      case PP_TRUETYPEFONTFAMILY_FANTASY:
        desc->family = "fantasy";
        break;
      case PP_TRUETYPEFONTFAMILY_MONOSPACE:
        desc->family = "monospace";
        break;
    }
  }

  fd_.reset(
      MatchFontFaceWithFallback(desc->family,
                                desc->weight >= PP_TRUETYPEFONTWEIGHT_BOLD,
                                desc->style & PP_TRUETYPEFONTSTYLE_ITALIC,
                                desc->charset,
                                PP_BROWSERFONT_TRUSTED_FAMILY_DEFAULT));
  // TODO(bbudge) Modify content API to return results of font matching and
  // fallback, so we can update |desc| to reflect that.
  return fd_.is_valid() ? PP_OK : PP_ERROR_FAILED;
}

int32_t PepperTrueTypeFontLinux::GetTableTags(std::vector<uint32_t>* tags) {
  if (!fd_.is_valid())
    return PP_ERROR_FAILED;
  // Get the 2 byte numTables field at an offset of 4 in the font.
  uint8_t num_tables_buf[2];
  size_t output_length = sizeof(num_tables_buf);
  if (!content::GetFontTable(fd_.get(), 0 /* tag */, 4 /* offset */,
                             reinterpret_cast<uint8_t*>(&num_tables_buf),
                             &output_length))
    return PP_ERROR_FAILED;
  DCHECK(output_length == sizeof(num_tables_buf));
  // Font data is stored in big-endian order.
  uint16_t num_tables = (num_tables_buf[0] << 8) | num_tables_buf[1];

  // The font has a header, followed by n table entries in its directory.
  static const size_t kFontHeaderSize = 12;
  static const size_t kTableEntrySize = 16;
  output_length = num_tables * kTableEntrySize;
  std::unique_ptr<uint8_t[]> table_entries(new uint8_t[output_length]);
  // Get the table directory entries, which follow the font header.
  if (!content::GetFontTable(fd_.get(), 0 /* tag */,
                             kFontHeaderSize /* offset */, table_entries.get(),
                             &output_length))
    return PP_ERROR_FAILED;
  DCHECK(output_length == num_tables * kTableEntrySize);

  tags->resize(num_tables);
  for (uint16_t i = 0; i < num_tables; i++) {
    uint8_t* entry = table_entries.get() + i * kTableEntrySize;
    uint32_t tag = static_cast<uint32_t>(entry[0]) << 24 |
                   static_cast<uint32_t>(entry[1]) << 16 |
                   static_cast<uint32_t>(entry[2]) << 8 |
                   static_cast<uint32_t>(entry[3]);
    (*tags)[i] = tag;
  }

  return num_tables;
}

int32_t PepperTrueTypeFontLinux::GetTable(uint32_t table_tag,
                                          int32_t offset,
                                          int32_t max_data_length,
                                          std::string* data) {
  if (!fd_.is_valid())
    return PP_ERROR_FAILED;
  // Get the size of the font data first.
  size_t table_size = 0;
  // Tags are byte swapped on Linux.
  table_tag = base::ByteSwap(table_tag);
  if (!content::GetFontTable(fd_.get(), table_tag, offset, nullptr,
                             &table_size))
    return PP_ERROR_FAILED;
  // Only retrieve as much as the caller requested.
  table_size = std::min(table_size, static_cast<size_t>(max_data_length));
  data->resize(table_size);
  if (!content::GetFontTable(fd_.get(), table_tag, offset,
                             reinterpret_cast<uint8_t*>(&(*data)[0]),
                             &table_size))
    return PP_ERROR_FAILED;

  return base::checked_cast<int32_t>(table_size);
}

}  // namespace

// static
PepperTrueTypeFont* PepperTrueTypeFont::Create() {
  return new PepperTrueTypeFontLinux();
}

}  // namespace content
