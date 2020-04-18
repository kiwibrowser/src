// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Tests PPB_TrueTypeFont interface.

#include "ppapi/tests/test_truetype_font.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <algorithm>
#include <limits>

#include "ppapi/c/private/ppb_testing_private.h"
#include "ppapi/cpp/completion_callback.h"
#include "ppapi/cpp/dev/truetype_font_dev.h"
#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/var.h"
#include "ppapi/tests/test_utils.h"
#include "ppapi/tests/testing_instance.h"

REGISTER_TEST_CASE(TrueTypeFont);

#define MAKE_TABLE_TAG(a, b, c, d) ((a) << 24) + ((b) << 16) + ((c) << 8) + (d)

namespace {

const PP_Resource kInvalidResource = 0;
const PP_Instance kInvalidInstance = 0;

// TrueType font header and table entry structs. See
// https://developer.apple.com/fonts/TTRefMan/RM06/Chap6.html
struct FontHeader {
  int32_t font_type;
  uint16_t num_tables;
  uint16_t search_range;
  uint16_t entry_selector;
  uint16_t range_shift;
};

struct FontDirectoryEntry {
  uint32_t tag;
  uint32_t checksum;
  uint32_t offset;
  uint32_t logical_length;
};

uint32_t ReadBigEndian32(const void* ptr) {
  const uint8_t* data = reinterpret_cast<const uint8_t*>(ptr);
  return (data[3] << 0) | (data[2] << 8) | (data[1] << 16) | (data[0] << 24);
}

uint16_t ReadBigEndian16(const void* ptr) {
  const uint8_t* data = reinterpret_cast<const uint8_t*>(ptr);
  return (data[1] << 0) | (data[0] << 8);
}

}

TestTrueTypeFont::TestTrueTypeFont(TestingInstance* instance)
    : TestCase(instance),
      ppb_truetype_font_interface_(NULL),
      ppb_core_interface_(NULL),
      ppb_var_interface_(NULL) {
}

bool TestTrueTypeFont::Init() {
  ppb_truetype_font_interface_ = static_cast<const PPB_TrueTypeFont_Dev*>(
      pp::Module::Get()->GetBrowserInterface(PPB_TRUETYPEFONT_DEV_INTERFACE));
  if (!ppb_truetype_font_interface_)
    instance_->AppendError("PPB_TrueTypeFont_Dev interface not available");

  ppb_core_interface_ = static_cast<const PPB_Core*>(
      pp::Module::Get()->GetBrowserInterface(PPB_CORE_INTERFACE));
  if (!ppb_core_interface_)
    instance_->AppendError("PPB_Core interface not available");

  ppb_var_interface_ = static_cast<const PPB_Var*>(
      pp::Module::Get()->GetBrowserInterface(PPB_VAR_INTERFACE));
  if (!ppb_var_interface_)
    instance_->AppendError("PPB_Var interface not available");

  return
      ppb_truetype_font_interface_ &&
      ppb_core_interface_ &&
      ppb_var_interface_;
}

TestTrueTypeFont::~TestTrueTypeFont() {
}

void TestTrueTypeFont::RunTests(const std::string& filter) {
  RUN_TEST(GetFontFamilies, filter);
  RUN_TEST(GetFontsInFamily, filter);
  RUN_TEST(Create, filter);
  RUN_TEST(Describe, filter);
  RUN_TEST(GetTableTags, filter);
  RUN_TEST(GetTable, filter);
}

std::string TestTrueTypeFont::TestGetFontFamilies() {
  {
    // A valid instance should be able to enumerate fonts.
    TestCompletionCallbackWithOutput< std::vector<pp::Var> > cc(
        instance_->pp_instance(), false);
    cc.WaitForResult(pp::TrueTypeFont_Dev::GetFontFamilies(instance_,
                                                           cc.GetCallback()));
    const std::vector<pp::Var> font_families = cc.output();
    // We should get some font families on any platform.
    ASSERT_NE(0, font_families.size());
    ASSERT_EQ(static_cast<int32_t>(font_families.size()), cc.result());
    // Make sure at least one family is a non-empty string.
    ASSERT_NE(0, font_families[0].AsString().size());
  }
  {
    // Using an invalid instance should fail.
    TestCompletionCallbackWithOutput< std::vector<pp::Var> > cc(
        instance_->pp_instance(), false);
    cc.WaitForResult(
        ppb_truetype_font_interface_->GetFontFamilies(
            kInvalidInstance,
            cc.GetCallback().output(),
            cc.GetCallback().pp_completion_callback()));
    ASSERT_TRUE(cc.result() == PP_ERROR_FAILED ||
                cc.result() == PP_ERROR_BADARGUMENT);
    ASSERT_EQ(0, cc.output().size());
  }

  PASS();
}

std::string TestTrueTypeFont::TestGetFontsInFamily() {
  {
    // Get the list of all font families.
    TestCompletionCallbackWithOutput< std::vector<pp::Var> > cc(
        instance_->pp_instance(), false);
    cc.WaitForResult(pp::TrueTypeFont_Dev::GetFontFamilies(instance_,
                                                           cc.GetCallback()));
    // Try to use a common family that is likely to have multiple variations.
    const std::vector<pp::Var> families = cc.output();
    pp::Var family("Arial");
    if (std::find(families.begin(), families.end(), family) == families.end()) {
      family = pp::Var("Times");
      if (std::find(families.begin(), families.end(), family) == families.end())
        family = families[0];  // Just use the first family.
    }

    // GetFontsInFamily: A valid instance should be able to enumerate fonts
    // in a given family.
    TestCompletionCallbackWithOutput< std::vector<pp::TrueTypeFontDesc_Dev> >
        cc2(instance_->pp_instance(), false);
    cc2.WaitForResult(pp::TrueTypeFont_Dev::GetFontsInFamily(
        instance_,
        family,
        cc2.GetCallback()));
    std::vector<pp::TrueTypeFontDesc_Dev> fonts_in_family = cc2.output();
    ASSERT_NE(0, fonts_in_family.size());
    ASSERT_EQ(static_cast<int32_t>(fonts_in_family.size()), cc2.result());

    // We should be able to create any of the returned fonts without fallback.
    for (size_t i = 0; i < fonts_in_family.size(); ++i) {
      pp::TrueTypeFontDesc_Dev& font_in_family = fonts_in_family[i];
      pp::TrueTypeFont_Dev font(instance_, font_in_family);
      TestCompletionCallbackWithOutput<pp::TrueTypeFontDesc_Dev> cc(
          instance_->pp_instance(), false);
      cc.WaitForResult(font.Describe(cc.GetCallback()));
      const pp::TrueTypeFontDesc_Dev desc = cc.output();

      ASSERT_EQ(family, desc.family());
      ASSERT_EQ(font_in_family.style(), desc.style());
      ASSERT_EQ(font_in_family.weight(), desc.weight());
    }
  }
  {
    // Using an invalid instance should fail.
    TestCompletionCallbackWithOutput< std::vector<pp::TrueTypeFontDesc_Dev> >
        cc(instance_->pp_instance(), false);
    pp::Var family("Times");
    cc.WaitForResult(
        ppb_truetype_font_interface_->GetFontsInFamily(
            kInvalidInstance,
            family.pp_var(),
            cc.GetCallback().output(),
            cc.GetCallback().pp_completion_callback()));
    ASSERT_TRUE(cc.result() == PP_ERROR_FAILED ||
                cc.result() == PP_ERROR_BADARGUMENT);
    ASSERT_EQ(0, cc.output().size());
  }

  PASS();
}

std::string TestTrueTypeFont::TestCreate() {
  PP_Resource font;
  PP_TrueTypeFontDesc_Dev desc = {
    PP_MakeUndefined(),
    PP_TRUETYPEFONTFAMILY_SERIF,
    PP_TRUETYPEFONTSTYLE_NORMAL,
    PP_TRUETYPEFONTWEIGHT_NORMAL,
    PP_TRUETYPEFONTWIDTH_NORMAL,
    PP_TRUETYPEFONTCHARSET_DEFAULT
  };
  // Creating a font from an invalid instance returns an invalid resource.
  font = ppb_truetype_font_interface_->Create(kInvalidInstance, &desc);
  ASSERT_EQ(kInvalidResource, font);
  ASSERT_NE(PP_TRUE, ppb_truetype_font_interface_->IsTrueTypeFont(font));

  // Creating a font from a valid instance returns a font resource.
  font = ppb_truetype_font_interface_->Create(instance_->pp_instance(), &desc);
  ASSERT_NE(kInvalidResource, font);
  ASSERT_EQ(PP_TRUE, ppb_truetype_font_interface_->IsTrueTypeFont(font));

  ppb_core_interface_->ReleaseResource(font);
  // Once released, the resource shouldn't be a font.
  ASSERT_NE(PP_TRUE, ppb_truetype_font_interface_->IsTrueTypeFont(font));

  PASS();
}

std::string TestTrueTypeFont::TestDescribe() {
  pp::TrueTypeFontDesc_Dev create_desc;
  create_desc.set_generic_family(PP_TRUETYPEFONTFAMILY_SERIF);
  create_desc.set_style(PP_TRUETYPEFONTSTYLE_NORMAL);
  create_desc.set_weight(PP_TRUETYPEFONTWEIGHT_NORMAL);
  pp::TrueTypeFont_Dev font(instance_, create_desc);
  // Describe: See what font-matching did with a generic font. We should always
  // be able to Create a generic Serif font.
  TestCompletionCallbackWithOutput<pp::TrueTypeFontDesc_Dev> cc(
      instance_->pp_instance(), false);
  cc.WaitForResult(font.Describe(cc.GetCallback()));
  const pp::TrueTypeFontDesc_Dev desc = cc.output();
  ASSERT_NE(0, desc.family().AsString().size());
  ASSERT_EQ(PP_TRUETYPEFONTFAMILY_SERIF, desc.generic_family());
  ASSERT_EQ(PP_TRUETYPEFONTSTYLE_NORMAL, desc.style());
  ASSERT_EQ(PP_TRUETYPEFONTWEIGHT_NORMAL, desc.weight());

  // Describe an invalid resource should fail.
  PP_TrueTypeFontDesc_Dev fail_desc;
  memset(&fail_desc, 0, sizeof(fail_desc));
  fail_desc.family = PP_MakeUndefined();
  // Create a shallow copy to check that no data is changed.
  PP_TrueTypeFontDesc_Dev fail_desc_copy;
  memcpy(&fail_desc_copy, &fail_desc, sizeof(fail_desc));

  cc.WaitForResult(
      ppb_truetype_font_interface_->Describe(
          kInvalidResource,
          &fail_desc,
          cc.GetCallback().pp_completion_callback()));
  ASSERT_EQ(PP_ERROR_BADRESOURCE, cc.result());
  ASSERT_EQ(PP_VARTYPE_UNDEFINED, fail_desc.family.type);
  ASSERT_EQ(0, memcmp(&fail_desc, &fail_desc_copy, sizeof(fail_desc)));

  PASS();
}

std::string TestTrueTypeFont::TestGetTableTags() {
  pp::TrueTypeFontDesc_Dev desc;
  pp::TrueTypeFont_Dev font(instance_, desc);
  {
    TestCompletionCallbackWithOutput< std::vector<uint32_t> > cc(
        instance_->pp_instance(), false);
    cc.WaitForResult(font.GetTableTags(cc.GetCallback()));
    std::vector<uint32_t> tags = cc.output();
    ASSERT_NE(0, tags.size());
    ASSERT_EQ(static_cast<int32_t>(tags.size()), cc.result());
    // Tags will vary depending on the actual font that the host platform
    // chooses. Check that all required TrueType tags are present.
    const int required_tag_count = 9;
    uint32_t required_tags[required_tag_count] = {
      // Note: these must be sorted for std::includes below.
      MAKE_TABLE_TAG('c', 'm', 'a', 'p'),
      MAKE_TABLE_TAG('g', 'l', 'y', 'f'),
      MAKE_TABLE_TAG('h', 'e', 'a', 'd'),
      MAKE_TABLE_TAG('h', 'h', 'e', 'a'),
      MAKE_TABLE_TAG('h', 'm', 't', 'x'),
      MAKE_TABLE_TAG('l', 'o', 'c', 'a'),
      MAKE_TABLE_TAG('m', 'a', 'x', 'p'),
      MAKE_TABLE_TAG('n', 'a', 'm', 'e'),
      MAKE_TABLE_TAG('p', 'o', 's', 't')
    };
    std::sort(tags.begin(), tags.end());
    ASSERT_TRUE(std::includes(tags.begin(),
                              tags.end(),
                              required_tags,
                              required_tags + required_tag_count));
  }
  {
    // Invalid resource should fail and write no data.
    TestCompletionCallbackWithOutput< std::vector<uint32_t> > cc(
        instance_->pp_instance(), false);
    cc.WaitForResult(
        ppb_truetype_font_interface_->GetTableTags(
            kInvalidResource,
            cc.GetCallback().output(),
            cc.GetCallback().pp_completion_callback()));
    ASSERT_EQ(PP_ERROR_BADRESOURCE, cc.result());
    ASSERT_EQ(0, cc.output().size());
  }

  PASS();
}

std::string TestTrueTypeFont::TestGetTable() {
  pp::TrueTypeFontDesc_Dev desc;
  pp::TrueTypeFont_Dev font(instance_, desc);

  {
    // Getting a required table from a valid font should succeed.
    TestCompletionCallbackWithOutput< std::vector<char> > cc1(
        instance_->pp_instance(), false);
    cc1.WaitForResult(font.GetTable(MAKE_TABLE_TAG('c', 'm', 'a', 'p'),
                                    0, std::numeric_limits<int32_t>::max(),
                                    cc1.GetCallback()));
    const std::vector<char> cmap_data = cc1.output();
    ASSERT_NE(0, cmap_data.size());
    ASSERT_EQ(static_cast<int32_t>(cmap_data.size()), cc1.result());

    // Passing 0 for the table tag should return the entire font.
    TestCompletionCallbackWithOutput< std::vector<char> > cc2(
        instance_->pp_instance(), false);
    cc2.WaitForResult(font.GetTable(0 /* table_tag */,
                                    0, std::numeric_limits<int32_t>::max(),
                                    cc2.GetCallback()));
    const std::vector<char> entire_font = cc2.output();
    ASSERT_NE(0, entire_font.size());
    ASSERT_EQ(static_cast<int32_t>(entire_font.size()), cc2.result());

    // Verify that the CMAP table is in entire_font, and that it's identical
    // to the one we retrieved above. Note that since the font header and table
    // directory are in file (big-endian) order, we need to byte swap tags and
    // numbers.
    const size_t kHeaderSize = sizeof(FontHeader);
    const size_t kEntrySize = sizeof(FontDirectoryEntry);
    ASSERT_TRUE(kHeaderSize < entire_font.size());
    FontHeader header;
    memcpy(&header, &entire_font[0], kHeaderSize);
    uint16_t num_tables = ReadBigEndian16(&header.num_tables);
    std::vector<FontDirectoryEntry> directory(num_tables);
    size_t directory_size = kEntrySize * num_tables;
    ASSERT_TRUE(kHeaderSize + directory_size < entire_font.size());
    memcpy(&directory[0], &entire_font[kHeaderSize], directory_size);
    const FontDirectoryEntry* cmap_entry = NULL;
    for (uint16_t i = 0; i < num_tables; i++) {
      if (ReadBigEndian32(&directory[i].tag) ==
          MAKE_TABLE_TAG('c', 'm', 'a', 'p')) {
        cmap_entry = &directory[i];
        break;
      }
    }
    ASSERT_NE(NULL, cmap_entry);

    uint32_t logical_length = ReadBigEndian32(&cmap_entry->logical_length);
    uint32_t table_offset = ReadBigEndian32(&cmap_entry->offset);
    ASSERT_EQ(static_cast<size_t>(logical_length), cmap_data.size());
    ASSERT_TRUE(static_cast<size_t>(table_offset + logical_length) <
                    entire_font.size());
    const char* cmap_table = &entire_font[0] + table_offset;
    ASSERT_EQ(0, memcmp(cmap_table, &cmap_data[0], cmap_data.size()));

    // Use offset and max_data_length to restrict the data. Read a part of
    // the 'CMAP' table.
    TestCompletionCallbackWithOutput< std::vector<char> > cc3(
        instance_->pp_instance(), false);
    const int32_t kOffset = 4;
    int32_t partial_cmap_size = static_cast<int32_t>(cmap_data.size() - 64);
    cc3.WaitForResult(font.GetTable(MAKE_TABLE_TAG('c', 'm', 'a', 'p'),
                                    kOffset,
                                    partial_cmap_size,
                                    cc3.GetCallback()));
    const std::vector<char> partial_cmap_data = cc3.output();
    ASSERT_EQ(partial_cmap_data.size(), static_cast<size_t>(cc3.result()));
    ASSERT_EQ(partial_cmap_data.size(), static_cast<size_t>(partial_cmap_size));
    ASSERT_EQ(0, memcmp(cmap_table + kOffset, &partial_cmap_data[0],
                        partial_cmap_size));
  }
  {
    // Getting an invalid table should fail ('zzzz' should be safely invalid).
    TestCompletionCallbackWithOutput< std::vector<char> > cc(
        instance_->pp_instance(), false);
    cc.WaitForResult(font.GetTable(MAKE_TABLE_TAG('z', 'z', 'z', 'z'),
                                   0, std::numeric_limits<int32_t>::max(),
                                   cc.GetCallback()));
    ASSERT_EQ(0, cc.output().size());
    ASSERT_EQ(PP_ERROR_FAILED, cc.result());
  }
  {
    // GetTable on an invalid resource should fail with a bad resource error
    // and write no data.
    TestCompletionCallbackWithOutput< std::vector<char> > cc(
        instance_->pp_instance(), false);
    cc.WaitForResult(
        ppb_truetype_font_interface_->GetTable(
            kInvalidResource,
            MAKE_TABLE_TAG('c', 'm', 'a', 'p'),
            0, std::numeric_limits<int32_t>::max(),
            cc.GetCallback().output(),
            cc.GetCallback().pp_completion_callback()));
    ASSERT_EQ(PP_ERROR_BADRESOURCE, cc.result());
    ASSERT_EQ(0, cc.output().size());
  }
  {
    // Negative offset should fail with a bad argument error and write no data.
    TestCompletionCallbackWithOutput< std::vector<char> > cc(
        instance_->pp_instance(), false);
    cc.WaitForResult(font.GetTable(MAKE_TABLE_TAG('c', 'm', 'a', 'p'),
                                   -100, 0,
                                   cc.GetCallback()));
    ASSERT_EQ(PP_ERROR_BADARGUMENT, cc.result());
    ASSERT_EQ(0, cc.output().size());
  }
  {
    // Offset larger than file size succeeds but returns no data.
    TestCompletionCallbackWithOutput< std::vector<char> > cc(
        instance_->pp_instance(), false);
    cc.WaitForResult(font.GetTable(MAKE_TABLE_TAG('c', 'm', 'a', 'p'),
                                   1 << 28, 0,
                                   cc.GetCallback()));
    ASSERT_EQ(PP_OK, cc.result());
    ASSERT_EQ(0, cc.output().size());
  }
  {
    // Negative max_data_length should fail with a bad argument error and write
    // no data.
    TestCompletionCallbackWithOutput< std::vector<char> > cc(
        instance_->pp_instance(), false);
    cc.WaitForResult(font.GetTable(MAKE_TABLE_TAG('c', 'm', 'a', 'p'),
                                   0, -100,
                                   cc.GetCallback()));
    ASSERT_EQ(PP_ERROR_BADARGUMENT, cc.result());
    ASSERT_EQ(0, cc.output().size());
  }

  PASS();
}
