// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cloud_devices/common/printer_description.h"

#include <memory>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace cloud_devices {

namespace printer {

// Replaces ' with " to allow readable JSON constants in tests.
// Makes sure that same JSON value represented by same strings to simplify
// comparison.
std::string NormalizeJson(const std::string& json) {
  std::string result = json;
  base::ReplaceChars(result, "'", "\"", &result);
  std::unique_ptr<base::Value> value = base::JSONReader::Read(result);
  base::JSONWriter::Write(*value, &result);
  return result;
}

const char kCdd[] =
    "{"
    "  'version': '1.0',"
    "  'printer': {"
    "    'supported_content_type': [ {"
    "      'content_type': 'image/pwg-raster'"
    "    }, {"
    "      'content_type': 'image/jpeg'"
    "    } ],"
    "    'pwg_raster_config': {"
    "      'document_sheet_back': 'MANUAL_TUMBLE',"
    "      'reverse_order_streaming': true"
    "    },"
    "    'color': {"
    "      'option': [ {"
    "        'is_default': true,"
    "        'type': 'STANDARD_COLOR'"
    "      }, {"
    "        'type': 'STANDARD_MONOCHROME'"
    "      }, {"
    "        'type': 'CUSTOM_MONOCHROME',"
    "        'vendor_id': '123',"
    "        'custom_display_name': 'monochrome'"
    "      } ]"
    "    },"
    "    'duplex': {"
    "      'option': [ {"
    "        'is_default': true,"
    "        'type': 'LONG_EDGE'"
    "       }, {"
    "        'type': 'SHORT_EDGE'"
    "       }, {"
    "        'type': 'NO_DUPLEX'"
    "       } ]"
    "    },"
    "    'page_orientation': {"
    "      'option': [ {"
    "        'type': 'PORTRAIT'"
    "      }, {"
    "        'type': 'LANDSCAPE'"
    "      }, {"
    "        'is_default': true,"
    "        'type': 'AUTO'"
    "      } ]"
    "    },"
    "    'copies': {"
    "    },"
    "    'margins': {"
    "      'option': [ {"
    "        'is_default': true,"
    "        'type': 'BORDERLESS',"
    "        'top_microns': 0,"
    "        'right_microns': 0,"
    "        'bottom_microns': 0,"
    "        'left_microns': 0"
    "      }, {"
    "         'type': 'STANDARD',"
    "         'top_microns': 100,"
    "         'right_microns': 200,"
    "         'bottom_microns': 300,"
    "         'left_microns': 400"
    "      }, {"
    "         'type': 'CUSTOM',"
    "         'top_microns': 1,"
    "         'right_microns': 2,"
    "         'bottom_microns': 3,"
    "         'left_microns': 4"
    "      } ]"
    "    },"
    "    'dpi': {"
    "      'option': [ {"
    "        'horizontal_dpi': 150,"
    "        'vertical_dpi': 250"
    "      }, {"
    "        'is_default': true,"
    "        'horizontal_dpi': 600,"
    "        'vertical_dpi': 1600"
    "      } ]"
    "    },"
    "    'fit_to_page': {"
    "      'option': [ {"
    "        'is_default': true,"
    "        'type': 'NO_FITTING'"
    "      }, {"
    "        'type': 'FIT_TO_PAGE'"
    "      }, {"
    "        'type': 'GROW_TO_PAGE'"
    "      }, {"
    "        'type': 'SHRINK_TO_PAGE'"
    "      }, {"
    "        'type': 'FILL_PAGE'"
    "      } ]"
    "    },"
    "    'page_range': {"
    "    },"
    "    'media_size': {"
    "      'option': [ {"
    "        'is_default': true,"
    "        'name': 'NA_LETTER',"
    "        'width_microns': 2222,"
    "        'height_microns': 3333"
    "      }, {"
    "        'name': 'ISO_A6',"
    "        'width_microns': 4444,"
    "        'height_microns': 5555"
    "      }, {"
    "        'name': 'JPN_YOU4',"
    "        'width_microns': 6666,"
    "        'height_microns': 7777"
    "      }, {"
    "        'width_microns': 1111,"
    "        'is_continuous_feed': true,"
    "        'custom_display_name': 'Feed',"
    "        'vendor_id': 'FEED'"
    "      } ]"
    "    },"
    "    'collate': {"
    "      'default': false"
    "    },"
    "    'reverse_order': {"
    "      'default': true"
    "    }"
    "  }"
    "}";

const char kDefaultCdd[] =
    "{"
    "  'version': '1.0'"
    "}";

const char kBadVersionCdd[] =
    "{"
    "  'version': '1.1',"
    "  'printer': {"
    "  }"
    "}";

const char kNoDefaultCdd[] =
    "{"
    "  'version': '1.0',"
    "  'printer': {"
    "    'color': {"
    "      'option': [ {"
    "        'type': 'STANDARD_COLOR'"
    "      }, {"
    "        'type': 'STANDARD_MONOCHROME'"
    "      } ]"
    "    }"
    "  }"
    "}";

const char kMultyDefaultCdd[] =
    "{"
    "  'version': '1.0',"
    "  'printer': {"
    "    'color': {"
    "      'option': [ {"
    "        'is_default': true,"
    "        'type': 'STANDARD_COLOR'"
    "      }, {"
    "        'is_default': true,"
    "        'type': 'STANDARD_MONOCHROME'"
    "      } ]"
    "    }"
    "  }"
    "}";

const char kDocumentTypeColorOnlyCdd[] =
    "{"
    "  'version': '1.0',"
    "  'printer': {"
    "    'pwg_raster_config': {"
    "      'document_type_supported': [ 'SRGB_8' ],"
    "      'document_sheet_back': 'ROTATED'"
    "    }"
    "  }"
    "}";

const char kDocumentTypeGrayOnlyCdd[] =
    "{"
    "  'version': '1.0',"
    "  'printer': {"
    "    'pwg_raster_config': {"
    "      'document_type_supported': [ 'SGRAY_8' ],"
    "      'document_sheet_back': 'ROTATED'"
    "    }"
    "  }"
    "}";

const char kDocumentTypeColorAndGrayCdd[] =
    "{"
    "  'version': '1.0',"
    "  'printer': {"
    "    'pwg_raster_config': {"
    "      'document_type_supported': [ 'SRGB_8', 'SGRAY_8' ],"
    "      'document_sheet_back': 'ROTATED'"
    "    }"
    "  }"
    "}";

const char kDocumentTypeColorAndUnsupportedCdd[] =
    "{"
    "  'version': '1.0',"
    "  'printer': {"
    "    'pwg_raster_config': {"
    "      'document_type_supported': [ 'SRGB_8', 'SRGB_16' ],"
    "      'document_sheet_back': 'ROTATED'"
    "    }"
    "  }"
    "}";

const char kDocumentTypeNoneCdd[] =
    "{"
    "  'version': '1.0',"
    "  'printer': {"
    "    'pwg_raster_config': {"
    "      'document_sheet_back': 'ROTATED'"
    "    }"
    "  }"
    "}";

const char kDocumentTypeNotStringCdd[] =
    "{"
    "  'version': '1.0',"
    "  'printer': {"
    "    'pwg_raster_config': {"
    "      'document_type_supported': [ 8, 16 ],"
    "      'document_sheet_back': 'ROTATED'"
    "    }"
    "  }"
    "}";

const char kDocumentTypeNotListCdd[] =
    "{"
    "  'version': '1.0',"
    "  'printer': {"
    "    'pwg_raster_config': {"
    "      'document_type_supported': 'ROTATED',"
    "      'document_sheet_back': 'ROTATED'"
    "    }"
    "  }"
    "}";

const char kCjt[] =
    "{"
    "  'version': '1.0',"
    "  'print': {"
    "    'pwg_raster_config': {"
    "      'document_sheet_back': 'MANUAL_TUMBLE',"
    "      'reverse_order_streaming': true"
    "    },"
    "    'color': {"
    "      'type': 'STANDARD_MONOCHROME'"
    "    },"
    "    'duplex': {"
    "      'type': 'NO_DUPLEX'"
    "    },"
    "    'page_orientation': {"
    "      'type': 'LANDSCAPE'"
    "    },"
    "    'copies': {"
    "      'copies': 123"
    "    },"
    "    'margins': {"
    "       'type': 'CUSTOM',"
    "       'top_microns': 7,"
    "       'right_microns': 6,"
    "       'bottom_microns': 3,"
    "       'left_microns': 1"
    "    },"
    "    'dpi': {"
    "      'horizontal_dpi': 562,"
    "      'vertical_dpi': 125"
    "    },"
    "    'fit_to_page': {"
    "      'type': 'SHRINK_TO_PAGE'"
    "    },"
    "    'page_range': {"
    "      'interval': [ {"
    "        'start': 1,"
    "        'end': 99"
    "       }, {"
    "        'start': 150"
    "       } ]"
    "    },"
    "    'media_size': {"
    "      'name': 'ISO_C7C6',"
    "      'width_microns': 4261,"
    "      'height_microns': 334"
    "    },"
    "    'collate': {"
    "      'collate': false"
    "    },"
    "    'reverse_order': {"
    "      'reverse_order': true"
    "    }"
    "  }"
    "}";

const char kDefaultCjt[] =
    "{"
    "  'version': '1.0'"
    "}";

const char kBadVersionCjt[] =
    "{"
    "  'version': '1.1',"
    "  'print': {"
    "  }"
    "}";

TEST(PrinterDescriptionTest, CddInit) {
  CloudDeviceDescription description;
  EXPECT_EQ(NormalizeJson(kDefaultCdd), NormalizeJson(description.ToString()));

  ContentTypesCapability content_types;
  PwgRasterConfigCapability pwg_raster;
  ColorCapability color;
  DuplexCapability duplex;
  OrientationCapability orientation;
  MarginsCapability margins;
  DpiCapability dpi;
  FitToPageCapability fit_to_page;
  MediaCapability media;
  CopiesCapability copies;
  PageRangeCapability page_range;
  CollateCapability collate;
  ReverseCapability reverse;

  EXPECT_FALSE(content_types.LoadFrom(description));
  EXPECT_FALSE(pwg_raster.LoadFrom(description));
  EXPECT_FALSE(color.LoadFrom(description));
  EXPECT_FALSE(duplex.LoadFrom(description));
  EXPECT_FALSE(orientation.LoadFrom(description));
  EXPECT_FALSE(copies.LoadFrom(description));
  EXPECT_FALSE(margins.LoadFrom(description));
  EXPECT_FALSE(dpi.LoadFrom(description));
  EXPECT_FALSE(fit_to_page.LoadFrom(description));
  EXPECT_FALSE(page_range.LoadFrom(description));
  EXPECT_FALSE(media.LoadFrom(description));
  EXPECT_FALSE(collate.LoadFrom(description));
  EXPECT_FALSE(reverse.LoadFrom(description));
  EXPECT_FALSE(media.LoadFrom(description));
}

TEST(PrinterDescriptionTest, CddInvalid) {
  CloudDeviceDescription description;
  ColorCapability color;

  EXPECT_FALSE(description.InitFromString(NormalizeJson(kBadVersionCdd)));

  EXPECT_TRUE(description.InitFromString(NormalizeJson(kNoDefaultCdd)));
  EXPECT_FALSE(color.LoadFrom(description));

  EXPECT_TRUE(description.InitFromString(NormalizeJson(kMultyDefaultCdd)));
  EXPECT_FALSE(color.LoadFrom(description));
}

TEST(PrinterDescriptionTest, CddSetAll) {
  CloudDeviceDescription description;

  ContentTypesCapability content_types;
  PwgRasterConfigCapability pwg_raster_config;
  ColorCapability color;
  DuplexCapability duplex;
  OrientationCapability orientation;
  MarginsCapability margins;
  DpiCapability dpi;
  FitToPageCapability fit_to_page;
  MediaCapability media;
  CopiesCapability copies;
  PageRangeCapability page_range;
  CollateCapability collate;
  ReverseCapability reverse;

  content_types.AddOption("image/pwg-raster");
  content_types.AddOption("image/jpeg");

  PwgRasterConfig custom_raster;
  custom_raster.document_sheet_back = MANUAL_TUMBLE;
  custom_raster.reverse_order_streaming = true;
  custom_raster.rotate_all_pages = false;
  pwg_raster_config.set_value(custom_raster);

  color.AddDefaultOption(Color(STANDARD_COLOR), true);
  color.AddOption(Color(STANDARD_MONOCHROME));
  Color custom(CUSTOM_MONOCHROME);
  custom.vendor_id = "123";
  custom.custom_display_name = "monochrome";
  color.AddOption(custom);

  duplex.AddDefaultOption(LONG_EDGE, true);
  duplex.AddOption(SHORT_EDGE);
  duplex.AddOption(NO_DUPLEX);

  orientation.AddOption(PORTRAIT);
  orientation.AddOption(LANDSCAPE);
  orientation.AddDefaultOption(AUTO_ORIENTATION, true);

  margins.AddDefaultOption(Margins(NO_MARGINS, 0, 0, 0, 0), true);
  margins.AddOption(Margins(STANDARD_MARGINS, 100, 200, 300, 400));
  margins.AddOption(Margins(CUSTOM_MARGINS, 1, 2, 3, 4));

  dpi.AddOption(Dpi(150, 250));
  dpi.AddDefaultOption(Dpi(600, 1600), true);

  fit_to_page.AddDefaultOption(NO_FITTING, true);
  fit_to_page.AddOption(FIT_TO_PAGE);
  fit_to_page.AddOption(GROW_TO_PAGE);
  fit_to_page.AddOption(SHRINK_TO_PAGE);
  fit_to_page.AddOption(FILL_PAGE);

  media.AddDefaultOption(Media(NA_LETTER, 2222, 3333), true);
  media.AddOption(Media(ISO_A6, 4444, 5555));
  media.AddOption(Media(JPN_YOU4, 6666, 7777));
  media.AddOption(Media("Feed", "FEED", 1111, 0));

  collate.set_default_value(false);
  reverse.set_default_value(true);

  content_types.SaveTo(&description);
  color.SaveTo(&description);
  duplex.SaveTo(&description);
  orientation.SaveTo(&description);
  copies.SaveTo(&description);
  margins.SaveTo(&description);
  dpi.SaveTo(&description);
  fit_to_page.SaveTo(&description);
  page_range.SaveTo(&description);
  media.SaveTo(&description);
  collate.SaveTo(&description);
  reverse.SaveTo(&description);
  pwg_raster_config.SaveTo(&description);

  EXPECT_EQ(NormalizeJson(kCdd), NormalizeJson(description.ToString()));
}

TEST(PrinterDescriptionTest, CddGetDocumentTypeSupported) {
  {
    CloudDeviceDescription description;
    ASSERT_TRUE(
        description.InitFromString(NormalizeJson(kDocumentTypeColorOnlyCdd)));

    PwgRasterConfigCapability pwg_raster;
    EXPECT_TRUE(pwg_raster.LoadFrom(description));
    ASSERT_EQ(1U, pwg_raster.value().document_types_supported.size());
    EXPECT_EQ(SRGB_8, pwg_raster.value().document_types_supported[0]);
    EXPECT_EQ(ROTATED, pwg_raster.value().document_sheet_back);
    EXPECT_FALSE(pwg_raster.value().reverse_order_streaming);
  }
  {
    CloudDeviceDescription description;
    ASSERT_TRUE(
        description.InitFromString(NormalizeJson(kDocumentTypeGrayOnlyCdd)));

    PwgRasterConfigCapability pwg_raster;
    EXPECT_TRUE(pwg_raster.LoadFrom(description));
    ASSERT_EQ(1U, pwg_raster.value().document_types_supported.size());
    EXPECT_EQ(SGRAY_8, pwg_raster.value().document_types_supported[0]);
    EXPECT_EQ(ROTATED, pwg_raster.value().document_sheet_back);
    EXPECT_FALSE(pwg_raster.value().reverse_order_streaming);
  }
  {
    CloudDeviceDescription description;
    ASSERT_TRUE(description.InitFromString(
        NormalizeJson(kDocumentTypeColorAndGrayCdd)));

    PwgRasterConfigCapability pwg_raster;
    EXPECT_TRUE(pwg_raster.LoadFrom(description));
    ASSERT_EQ(2U, pwg_raster.value().document_types_supported.size());
    EXPECT_EQ(SRGB_8, pwg_raster.value().document_types_supported[0]);
    EXPECT_EQ(SGRAY_8, pwg_raster.value().document_types_supported[1]);
    EXPECT_EQ(ROTATED, pwg_raster.value().document_sheet_back);
    EXPECT_FALSE(pwg_raster.value().reverse_order_streaming);
  }
  {
    CloudDeviceDescription description;
    ASSERT_TRUE(description.InitFromString(
        NormalizeJson(kDocumentTypeColorAndUnsupportedCdd)));

    PwgRasterConfigCapability pwg_raster;
    EXPECT_TRUE(pwg_raster.LoadFrom(description));
    ASSERT_EQ(1U, pwg_raster.value().document_types_supported.size());
    EXPECT_EQ(SRGB_8, pwg_raster.value().document_types_supported[0]);
    EXPECT_EQ(ROTATED, pwg_raster.value().document_sheet_back);
    EXPECT_FALSE(pwg_raster.value().reverse_order_streaming);
  }
  {
    CloudDeviceDescription description;
    ASSERT_TRUE(
        description.InitFromString(NormalizeJson(kDocumentTypeNoneCdd)));

    PwgRasterConfigCapability pwg_raster;
    EXPECT_TRUE(pwg_raster.LoadFrom(description));
    EXPECT_EQ(0U, pwg_raster.value().document_types_supported.size());
    EXPECT_EQ(ROTATED, pwg_raster.value().document_sheet_back);
    EXPECT_FALSE(pwg_raster.value().reverse_order_streaming);
  }
  {
    CloudDeviceDescription description;
    ASSERT_TRUE(
        description.InitFromString(NormalizeJson(kDocumentTypeNotStringCdd)));

    PwgRasterConfigCapability pwg_raster;
    EXPECT_FALSE(pwg_raster.LoadFrom(description));
  }
  {
    CloudDeviceDescription description;
    ASSERT_TRUE(
        description.InitFromString(NormalizeJson(kDocumentTypeNotListCdd)));

    PwgRasterConfigCapability pwg_raster;
    EXPECT_FALSE(pwg_raster.LoadFrom(description));
  }
}

TEST(PrinterDescriptionTest, CddSetDocumentTypeSupported) {
  {
    CloudDeviceDescription description;

    PwgRasterConfig custom_raster;
    custom_raster.document_types_supported.push_back(SRGB_8);
    custom_raster.document_sheet_back = ROTATED;

    PwgRasterConfigCapability pwg_raster;
    pwg_raster.set_value(custom_raster);
    pwg_raster.SaveTo(&description);

    EXPECT_EQ(NormalizeJson(kDocumentTypeColorOnlyCdd),
              NormalizeJson(description.ToString()));
  }
  {
    CloudDeviceDescription description;

    PwgRasterConfig custom_raster;
    custom_raster.document_types_supported.push_back(SGRAY_8);
    custom_raster.document_sheet_back = ROTATED;

    PwgRasterConfigCapability pwg_raster;
    pwg_raster.set_value(custom_raster);
    pwg_raster.SaveTo(&description);

    EXPECT_EQ(NormalizeJson(kDocumentTypeGrayOnlyCdd),
              NormalizeJson(description.ToString()));
  }
  {
    CloudDeviceDescription description;

    PwgRasterConfig custom_raster;
    custom_raster.document_types_supported.push_back(SRGB_8);
    custom_raster.document_types_supported.push_back(SGRAY_8);
    custom_raster.document_sheet_back = ROTATED;

    PwgRasterConfigCapability pwg_raster;
    pwg_raster.set_value(custom_raster);
    pwg_raster.SaveTo(&description);

    EXPECT_EQ(NormalizeJson(kDocumentTypeColorAndGrayCdd),
              NormalizeJson(description.ToString()));
  }
  {
    CloudDeviceDescription description;

    PwgRasterConfig custom_raster;
    custom_raster.document_sheet_back = ROTATED;

    PwgRasterConfigCapability pwg_raster;
    pwg_raster.set_value(custom_raster);
    pwg_raster.SaveTo(&description);

    EXPECT_EQ(NormalizeJson(kDocumentTypeNoneCdd),
              NormalizeJson(description.ToString()));
  }
}

TEST(PrinterDescriptionTest, CddGetAll) {
  CloudDeviceDescription description;
  ASSERT_TRUE(description.InitFromString(NormalizeJson(kCdd)));

  ContentTypesCapability content_types;
  PwgRasterConfigCapability pwg_raster_config;
  ColorCapability color;
  DuplexCapability duplex;
  OrientationCapability orientation;
  MarginsCapability margins;
  DpiCapability dpi;
  FitToPageCapability fit_to_page;
  MediaCapability media;
  CopiesCapability copies;
  PageRangeCapability page_range;
  CollateCapability collate;
  ReverseCapability reverse;

  EXPECT_TRUE(content_types.LoadFrom(description));
  EXPECT_TRUE(color.LoadFrom(description));
  EXPECT_TRUE(duplex.LoadFrom(description));
  EXPECT_TRUE(orientation.LoadFrom(description));
  EXPECT_TRUE(copies.LoadFrom(description));
  EXPECT_TRUE(margins.LoadFrom(description));
  EXPECT_TRUE(dpi.LoadFrom(description));
  EXPECT_TRUE(fit_to_page.LoadFrom(description));
  EXPECT_TRUE(page_range.LoadFrom(description));
  EXPECT_TRUE(media.LoadFrom(description));
  EXPECT_TRUE(collate.LoadFrom(description));
  EXPECT_TRUE(reverse.LoadFrom(description));
  EXPECT_TRUE(media.LoadFrom(description));
  EXPECT_TRUE(pwg_raster_config.LoadFrom(description));

  EXPECT_TRUE(content_types.Contains("image/pwg-raster"));
  EXPECT_TRUE(content_types.Contains("image/jpeg"));

  EXPECT_EQ(0U, pwg_raster_config.value().document_types_supported.size());
  EXPECT_EQ(MANUAL_TUMBLE, pwg_raster_config.value().document_sheet_back);
  EXPECT_TRUE(pwg_raster_config.value().reverse_order_streaming);
  EXPECT_FALSE(pwg_raster_config.value().rotate_all_pages);

  EXPECT_TRUE(color.Contains(Color(STANDARD_COLOR)));
  EXPECT_TRUE(color.Contains(Color(STANDARD_MONOCHROME)));
  Color custom(CUSTOM_MONOCHROME);
  custom.vendor_id = "123";
  custom.custom_display_name = "monochrome";
  EXPECT_TRUE(color.Contains(custom));
  EXPECT_EQ(Color(STANDARD_COLOR), color.GetDefault());

  EXPECT_TRUE(duplex.Contains(LONG_EDGE));
  EXPECT_TRUE(duplex.Contains(SHORT_EDGE));
  EXPECT_TRUE(duplex.Contains(NO_DUPLEX));
  EXPECT_EQ(LONG_EDGE, duplex.GetDefault());

  EXPECT_TRUE(orientation.Contains(PORTRAIT));
  EXPECT_TRUE(orientation.Contains(LANDSCAPE));
  EXPECT_TRUE(orientation.Contains(AUTO_ORIENTATION));
  EXPECT_EQ(AUTO_ORIENTATION, orientation.GetDefault());

  EXPECT_TRUE(margins.Contains(Margins(NO_MARGINS, 0, 0, 0, 0)));
  EXPECT_TRUE(margins.Contains(Margins(STANDARD_MARGINS, 100, 200, 300, 400)));
  EXPECT_TRUE(margins.Contains(Margins(CUSTOM_MARGINS, 1, 2, 3, 4)));
  EXPECT_EQ(Margins(NO_MARGINS, 0, 0, 0, 0), margins.GetDefault());

  EXPECT_TRUE(dpi.Contains(Dpi(150, 250)));
  EXPECT_TRUE(dpi.Contains(Dpi(600, 1600)));
  EXPECT_EQ(Dpi(600, 1600), dpi.GetDefault());

  EXPECT_TRUE(fit_to_page.Contains(NO_FITTING));
  EXPECT_TRUE(fit_to_page.Contains(FIT_TO_PAGE));
  EXPECT_TRUE(fit_to_page.Contains(GROW_TO_PAGE));
  EXPECT_TRUE(fit_to_page.Contains(SHRINK_TO_PAGE));
  EXPECT_TRUE(fit_to_page.Contains(FILL_PAGE));
  EXPECT_EQ(NO_FITTING, fit_to_page.GetDefault());

  EXPECT_TRUE(media.Contains(Media(NA_LETTER, 2222, 3333)));
  EXPECT_TRUE(media.Contains(Media(ISO_A6, 4444, 5555)));
  EXPECT_TRUE(media.Contains(Media(JPN_YOU4, 6666, 7777)));
  EXPECT_TRUE(media.Contains(Media("Feed", "FEED", 1111, 0)));
  EXPECT_EQ(Media(NA_LETTER, 2222, 3333), media.GetDefault());

  EXPECT_FALSE(collate.default_value());
  EXPECT_TRUE(reverse.default_value());

  EXPECT_EQ(NormalizeJson(kCdd), NormalizeJson(description.ToString()));
}

TEST(PrinterDescriptionTest, CjtInit) {
  CloudDeviceDescription description;
  EXPECT_EQ(NormalizeJson(kDefaultCjt), NormalizeJson(description.ToString()));

  PwgRasterConfigTicketItem pwg_raster_config;
  ColorTicketItem color;
  DuplexTicketItem duplex;
  OrientationTicketItem orientation;
  MarginsTicketItem margins;
  DpiTicketItem dpi;
  FitToPageTicketItem fit_to_page;
  MediaTicketItem media;
  CopiesTicketItem copies;
  PageRangeTicketItem page_range;
  CollateTicketItem collate;
  ReverseTicketItem reverse;

  EXPECT_FALSE(pwg_raster_config.LoadFrom(description));
  EXPECT_FALSE(color.LoadFrom(description));
  EXPECT_FALSE(duplex.LoadFrom(description));
  EXPECT_FALSE(orientation.LoadFrom(description));
  EXPECT_FALSE(copies.LoadFrom(description));
  EXPECT_FALSE(margins.LoadFrom(description));
  EXPECT_FALSE(dpi.LoadFrom(description));
  EXPECT_FALSE(fit_to_page.LoadFrom(description));
  EXPECT_FALSE(page_range.LoadFrom(description));
  EXPECT_FALSE(media.LoadFrom(description));
  EXPECT_FALSE(collate.LoadFrom(description));
  EXPECT_FALSE(reverse.LoadFrom(description));
  EXPECT_FALSE(media.LoadFrom(description));
}

TEST(PrinterDescriptionTest, CjtInvalid) {
  CloudDeviceDescription ticket;
  EXPECT_FALSE(ticket.InitFromString(NormalizeJson(kBadVersionCjt)));
}

TEST(PrinterDescriptionTest, CjtSetAll) {
  CloudDeviceDescription description;

  PwgRasterConfigTicketItem pwg_raster_config;
  ColorTicketItem color;
  DuplexTicketItem duplex;
  OrientationTicketItem orientation;
  MarginsTicketItem margins;
  DpiTicketItem dpi;
  FitToPageTicketItem fit_to_page;
  MediaTicketItem media;
  CopiesTicketItem copies;
  PageRangeTicketItem page_range;
  CollateTicketItem collate;
  ReverseTicketItem reverse;

  PwgRasterConfig custom_raster;
  custom_raster.document_sheet_back = MANUAL_TUMBLE;
  custom_raster.reverse_order_streaming = true;
  custom_raster.rotate_all_pages = false;
  pwg_raster_config.set_value(custom_raster);
  color.set_value(Color(STANDARD_MONOCHROME));
  duplex.set_value(NO_DUPLEX);
  orientation.set_value(LANDSCAPE);
  copies.set_value(123);
  margins.set_value(Margins(CUSTOM_MARGINS, 7, 6, 3, 1));
  dpi.set_value(Dpi(562, 125));
  fit_to_page.set_value(SHRINK_TO_PAGE);
  PageRange page_ranges;
  page_ranges.push_back(Interval(1, 99));
  page_ranges.push_back(Interval(150));
  page_range.set_value(page_ranges);
  media.set_value(Media(ISO_C7C6, 4261, 334));
  collate.set_value(false);
  reverse.set_value(true);

  pwg_raster_config.SaveTo(&description);
  color.SaveTo(&description);
  duplex.SaveTo(&description);
  orientation.SaveTo(&description);
  copies.SaveTo(&description);
  margins.SaveTo(&description);
  dpi.SaveTo(&description);
  fit_to_page.SaveTo(&description);
  page_range.SaveTo(&description);
  media.SaveTo(&description);
  collate.SaveTo(&description);
  reverse.SaveTo(&description);

  EXPECT_EQ(NormalizeJson(kCjt), NormalizeJson(description.ToString()));
}

TEST(PrinterDescriptionTest, CjtGetAll) {
  CloudDeviceDescription description;
  ASSERT_TRUE(description.InitFromString(NormalizeJson(kCjt)));

  ColorTicketItem color;
  DuplexTicketItem duplex;
  OrientationTicketItem orientation;
  MarginsTicketItem margins;
  DpiTicketItem dpi;
  FitToPageTicketItem fit_to_page;
  MediaTicketItem media;
  CopiesTicketItem copies;
  PageRangeTicketItem page_range;
  CollateTicketItem collate;
  ReverseTicketItem reverse;
  PwgRasterConfigTicketItem pwg_raster_config;

  EXPECT_TRUE(pwg_raster_config.LoadFrom(description));
  EXPECT_TRUE(color.LoadFrom(description));
  EXPECT_TRUE(duplex.LoadFrom(description));
  EXPECT_TRUE(orientation.LoadFrom(description));
  EXPECT_TRUE(copies.LoadFrom(description));
  EXPECT_TRUE(margins.LoadFrom(description));
  EXPECT_TRUE(dpi.LoadFrom(description));
  EXPECT_TRUE(fit_to_page.LoadFrom(description));
  EXPECT_TRUE(page_range.LoadFrom(description));
  EXPECT_TRUE(media.LoadFrom(description));
  EXPECT_TRUE(collate.LoadFrom(description));
  EXPECT_TRUE(reverse.LoadFrom(description));
  EXPECT_TRUE(media.LoadFrom(description));

  EXPECT_EQ(MANUAL_TUMBLE, pwg_raster_config.value().document_sheet_back);
  EXPECT_TRUE(pwg_raster_config.value().reverse_order_streaming);
  EXPECT_FALSE(pwg_raster_config.value().rotate_all_pages);
  EXPECT_EQ(color.value(), Color(STANDARD_MONOCHROME));
  EXPECT_EQ(duplex.value(), NO_DUPLEX);
  EXPECT_EQ(orientation.value(), LANDSCAPE);
  EXPECT_EQ(copies.value(), 123);
  EXPECT_EQ(margins.value(), Margins(CUSTOM_MARGINS, 7, 6, 3, 1));
  EXPECT_EQ(dpi.value(), Dpi(562, 125));
  EXPECT_EQ(fit_to_page.value(), SHRINK_TO_PAGE);
  PageRange page_ranges;
  page_ranges.push_back(Interval(1, 99));
  page_ranges.push_back(Interval(150));
  EXPECT_EQ(page_range.value(), page_ranges);
  EXPECT_EQ(media.value(), Media(ISO_C7C6, 4261, 334));
  EXPECT_FALSE(collate.value());
  EXPECT_TRUE(reverse.value());

  EXPECT_EQ(NormalizeJson(kCjt), NormalizeJson(description.ToString()));
}

}  // namespace printer

}  // namespace cloud_devices
