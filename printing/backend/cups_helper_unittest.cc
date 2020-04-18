// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "printing/backend/cups_helper.h"
#include "printing/backend/print_backend.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(PrintBackendCupsHelperTest, TestPpdParsingNoColorDuplexShortEdge) {
  const char kTestPpdData[] =
      "*PPD-Adobe: \"4.3\"\n\n"
      "*OpenGroup: General/General\n\n"
      "*OpenUI *ColorModel/Color Model: PickOne\n"
      "*DefaultColorModel: Gray\n"
      "*ColorModel Gray/Grayscale: \""
      "<</cupsColorSpace 0/cupsColorOrder 0>>"
      "setpagedevice\"\n"
      "*ColorModel Black/Inverted Grayscale: \""
      "<</cupsColorSpace 3/cupsColorOrder 0>>"
      "setpagedevice\"\n"
      "*CloseUI: *ColorModel\n"
      "*OpenUI *Duplex/2-Sided Printing: PickOne\n"
      "*DefaultDuplex: DuplexTumble\n"
      "*Duplex None/Off: \"<</Duplex false>>"
      "setpagedevice\"\n"
      "*Duplex DuplexNoTumble/LongEdge: \""
      "<</Duplex true/Tumble false>>setpagedevice\"\n"
      "*Duplex DuplexTumble/ShortEdge: \""
      "<</Duplex true/Tumble true>>setpagedevice\"\n"
      "*CloseUI: *Duplex\n\n"
      "*CloseGroup: General\n";

  printing::PrinterSemanticCapsAndDefaults caps;
  EXPECT_TRUE(printing::ParsePpdCapabilities("test", kTestPpdData, &caps));
  EXPECT_TRUE(caps.collate_capable);
  EXPECT_TRUE(caps.collate_default);
  EXPECT_TRUE(caps.copies_capable);
  EXPECT_TRUE(caps.duplex_capable);
  EXPECT_EQ(printing::SHORT_EDGE, caps.duplex_default);
  EXPECT_FALSE(caps.color_changeable);
  EXPECT_FALSE(caps.color_default);
}

// Test duplex detection code, which regressed in http://crbug.com/103999.
TEST(PrintBackendCupsHelperTest, TestPpdParsingNoColorDuplexSimples) {
  const char kTestPpdData[] =
      "*PPD-Adobe: \"4.3\"\n\n"
      "*OpenGroup: General/General\n\n"
      "*OpenUI *Duplex/Double-Sided Printing: PickOne\n"
      "*DefaultDuplex: None\n"
      "*Duplex None/Off: "
      "\"<</Duplex false>>setpagedevice\"\n"
      "*Duplex DuplexNoTumble/Long Edge (Standard): "
      "\"<</Duplex true/Tumble false>>setpagedevice\"\n"
      "*Duplex DuplexTumble/Short Edge (Flip): "
      "\"<</Duplex true/Tumble true>>setpagedevice\"\n"
      "*CloseUI: *Duplex\n\n"
      "*CloseGroup: General\n";

  printing::PrinterSemanticCapsAndDefaults caps;
  EXPECT_TRUE(printing::ParsePpdCapabilities("test", kTestPpdData, &caps));
  EXPECT_TRUE(caps.collate_capable);
  EXPECT_TRUE(caps.collate_default);
  EXPECT_TRUE(caps.copies_capable);
  EXPECT_TRUE(caps.duplex_capable);
  EXPECT_EQ(printing::SIMPLEX, caps.duplex_default);
  EXPECT_FALSE(caps.color_changeable);
  EXPECT_FALSE(caps.color_default);
}

TEST(PrintBackendCupsHelperTest, TestPpdParsingNoColorNoDuplex) {
  const char kTestPpdData[] =
      "*PPD-Adobe: \"4.3\"\n\n"
      "*OpenGroup: General/General\n\n"
      "*OpenUI *ColorModel/Color Model: PickOne\n"
      "*DefaultColorModel: Gray\n"
      "*ColorModel Gray/Grayscale: \""
      "<</cupsColorSpace 0/cupsColorOrder 0>>"
      "setpagedevice\"\n"
      "*ColorModel Black/Inverted Grayscale: \""
      "<</cupsColorSpace 3/cupsColorOrder 0>>"
      "setpagedevice\"\n"
      "*CloseUI: *ColorModel\n"
      "*CloseGroup: General\n";

  printing::PrinterSemanticCapsAndDefaults caps;
  EXPECT_TRUE(printing::ParsePpdCapabilities("test", kTestPpdData, &caps));
  EXPECT_TRUE(caps.collate_capable);
  EXPECT_TRUE(caps.collate_default);
  EXPECT_TRUE(caps.copies_capable);
  EXPECT_FALSE(caps.duplex_capable);
  EXPECT_EQ(printing::UNKNOWN_DUPLEX_MODE, caps.duplex_default);
  EXPECT_FALSE(caps.color_changeable);
  EXPECT_FALSE(caps.color_default);
}

TEST(PrintBackendCupsHelperTest, TestPpdParsingColorTrueDuplexShortEdge) {
  const char kTestPpdData[] =
      "*PPD-Adobe: \"4.3\"\n\n"
      "*ColorDevice: True\n"
      "*DefaultColorSpace: CMYK\n\n"
      "*OpenGroup: General/General\n\n"
      "*OpenUI *ColorModel/Color Model: PickOne\n"
      "*DefaultColorModel: CMYK\n"
      "*ColorModel CMYK/Color: "
      "\"(cmyk) RCsetdevicecolor\"\n"
      "*ColorModel Gray/Black and White: "
      "\"(gray) RCsetdevicecolor\"\n"
      "*CloseUI: *ColorModel\n"
      "*OpenUI *Duplex/2-Sided Printing: PickOne\n"
      "*DefaultDuplex: DuplexTumble\n"
      "*Duplex None/Off: \"<</Duplex false>>"
      "setpagedevice\"\n"
      "*Duplex DuplexNoTumble/LongEdge: \""
      "<</Duplex true/Tumble false>>setpagedevice\"\n"
      "*Duplex DuplexTumble/ShortEdge: \""
      "<</Duplex true/Tumble true>>setpagedevice\"\n"
      "*CloseUI: *Duplex\n\n"
      "*CloseGroup: General\n";

  printing::PrinterSemanticCapsAndDefaults caps;
  EXPECT_TRUE(printing::ParsePpdCapabilities("test", kTestPpdData, &caps));
  EXPECT_TRUE(caps.collate_capable);
  EXPECT_TRUE(caps.collate_default);
  EXPECT_TRUE(caps.copies_capable);
  EXPECT_TRUE(caps.duplex_capable);
  EXPECT_EQ(printing::SHORT_EDGE, caps.duplex_default);
  EXPECT_TRUE(caps.color_changeable);
  EXPECT_TRUE(caps.color_default);
}

TEST(PrintBackendCupsHelperTest, TestPpdParsingColorFalseDuplexLongEdge) {
  const char kTestPpdData[] =
      "*PPD-Adobe: \"4.3\"\n\n"
      "*ColorDevice: True\n"
      "*DefaultColorSpace: CMYK\n\n"
      "*OpenGroup: General/General\n\n"
      "*OpenUI *ColorModel/Color Model: PickOne\n"
      "*DefaultColorModel: Grayscale\n"
      "*ColorModel Color/Color: "
      "\"%% FoomaticRIPOptionSetting: ColorModel=Color\"\n"
      "*FoomaticRIPOptionSetting ColorModel=Color: "
      "\"JCLDatamode=Color GSCmdLine=Color\"\n"
      "*ColorModel Grayscale/Grayscale: "
      "\"%% FoomaticRIPOptionSetting: ColorModel=Grayscale\"\n"
      "*FoomaticRIPOptionSetting ColorModel=Grayscale: "
      "\"JCLDatamode=Grayscale GSCmdLine=Grayscale\"\n"
      "*CloseUI: *ColorModel\n"
      "*OpenUI *Duplex/2-Sided Printing: PickOne\n"
      "*DefaultDuplex: DuplexNoTumble\n"
      "*Duplex None/Off: \"<</Duplex false>>"
      "setpagedevice\"\n"
      "*Duplex DuplexNoTumble/LongEdge: \""
      "<</Duplex true/Tumble false>>setpagedevice\"\n"
      "*Duplex DuplexTumble/ShortEdge: \""
      "<</Duplex true/Tumble true>>setpagedevice\"\n"
      "*CloseUI: *Duplex\n\n"
      "*CloseGroup: General\n";

  printing::PrinterSemanticCapsAndDefaults caps;
  EXPECT_TRUE(printing::ParsePpdCapabilities("test", kTestPpdData, &caps));
  EXPECT_TRUE(caps.collate_capable);
  EXPECT_TRUE(caps.collate_default);
  EXPECT_TRUE(caps.copies_capable);
  EXPECT_TRUE(caps.duplex_capable);
  EXPECT_EQ(printing::LONG_EDGE, caps.duplex_default);
  EXPECT_TRUE(caps.color_changeable);
  EXPECT_FALSE(caps.color_default);
}

TEST(PrintBackendCupsHelperTest, TestPpdParsingPageSize) {
  const char kTestPpdData[] =
      "*PPD-Adobe: \"4.3\"\n\n"
      "*OpenUI *PageSize: PickOne\n"
      "*DefaultPageSize: Letter\n"
      "*PageSize Letter/US Letter: \""
      "  <</DeferredMediaSelection true /PageSize [612 792] "
      "  /ImagingBBox null /MediaClass null >> setpagedevice\"\n"
      "*End\n"
      "*PageSize Legal/US Legal: \""
      "  <</DeferredMediaSelection true /PageSize [612 1008] "
      "  /ImagingBBox null /MediaClass null >> setpagedevice\"\n"
      "*End\n"
      "*DefaultPaperDimension: Letter\n"
      "*PaperDimension Letter/US Letter: \"612   792\"\n"
      "*PaperDimension Legal/US Legal: \"612  1008\"\n\n"
      "*CloseUI: *PageSize\n\n";

  printing::PrinterSemanticCapsAndDefaults caps;
  EXPECT_TRUE(printing::ParsePpdCapabilities("test", kTestPpdData, &caps));
  ASSERT_EQ(2UL, caps.papers.size());
  EXPECT_EQ("Letter", caps.papers[0].vendor_id);
  EXPECT_EQ("US Letter", caps.papers[0].display_name);
  EXPECT_EQ(215900, caps.papers[0].size_um.width());
  EXPECT_EQ(279400, caps.papers[0].size_um.height());
  EXPECT_EQ("Legal", caps.papers[1].vendor_id);
  EXPECT_EQ("US Legal", caps.papers[1].display_name);
  EXPECT_EQ(215900, caps.papers[1].size_um.width());
  EXPECT_EQ(355600, caps.papers[1].size_um.height());
}

TEST(PrintBackendCupsHelperTest, TestPpdParsingBrotherPrinters) {
  {
    const char kTestPpdData[] =
        "*PPD-Adobe: \"4.3\"\n\n"
        "*ColorDevice: True\n"
        "*OpenUI *BRPrintQuality/Color/Mono: PickOne\n"
        "*DefaultBRPrintQuality: Auto\n"
        "*BRPrintQuality Auto/Auto: \"\"\n"
        "*BRPrintQuality Color/Color: \"\"\n"
        "*BRPrintQuality Black/Mono: \"\"\n"
        "*CloseUI: *BRPrintQuality\n\n";

    printing::PrinterSemanticCapsAndDefaults caps;
    EXPECT_TRUE(printing::ParsePpdCapabilities("test", kTestPpdData, &caps));
    EXPECT_TRUE(caps.color_changeable);
    EXPECT_TRUE(caps.color_default);
    EXPECT_EQ(printing::BROTHER_BRSCRIPT3_COLOR, caps.color_model);
    EXPECT_EQ(printing::BROTHER_BRSCRIPT3_BLACK, caps.bw_model);
  }
  {
    const char kTestPpdData[] =
        "*PPD-Adobe: \"4.3\"\n\n"
        "*ColorDevice: True\n"
        "*OpenUI *BRMonoColor/Color / Mono: PickOne\n"
        "*DefaultBRMonoColor: Auto\n"
        "*BRMonoColor Auto/Auto: \"\"\n"
        "*BRMonoColor FullColor/Color: \"\"\n"
        "*BRMonoColor Mono/Mono: \"\"\n"
        "*CloseUI: *BRMonoColor\n\n";

    printing::PrinterSemanticCapsAndDefaults caps;
    EXPECT_TRUE(printing::ParsePpdCapabilities("test", kTestPpdData, &caps));
    EXPECT_TRUE(caps.color_changeable);
    EXPECT_TRUE(caps.color_default);
    EXPECT_EQ(printing::BROTHER_CUPS_COLOR, caps.color_model);
    EXPECT_EQ(printing::BROTHER_CUPS_MONO, caps.bw_model);
  }
  {
    const char kTestPpdData[] =
        "*PPD-Adobe: \"4.3\"\n\n"
        "*ColorDevice: True\n"
        "*OpenUI *BRDuplex/Two-Sided Printing: PickOne\n"
        "*DefaultBRDuplex: DuplexTumble\n"
        "*BRDuplex DuplexTumble/Short-Edge Binding: \"\"\n"
        "*BRDuplex DuplexNoTumble/Long-Edge Binding: \"\"\n"
        "*BRDuplex None/Off: \"\"\n"
        "*CloseUI: *BRDuplex\n\n";

    printing::PrinterSemanticCapsAndDefaults caps;
    EXPECT_TRUE(printing::ParsePpdCapabilities("test", kTestPpdData, &caps));
    EXPECT_TRUE(caps.duplex_capable);
    EXPECT_EQ(printing::SHORT_EDGE, caps.duplex_default);
  }
}

TEST(PrintBackendCupsHelperTest, TestPpdParsingSamsungPrinters) {
  {
    const char kTestPpdData[] =
        "*PPD-Adobe: \"4.3\"\n\n"
        "*ColorDevice: True\n"
        "*OpenUI *ColorMode/Color Mode:  Boolean\n"
        "*DefaultColorMode: True\n"
        "*ColorMode False/Grayscale: \"\"\n"
        "*ColorMode True/Color: \"\"\n"
        "*CloseUI: *ColorMode\n\n";

    printing::PrinterSemanticCapsAndDefaults caps;
    EXPECT_TRUE(printing::ParsePpdCapabilities("test", kTestPpdData, &caps));
    EXPECT_TRUE(caps.color_changeable);
    EXPECT_TRUE(caps.color_default);
    EXPECT_EQ(printing::COLORMODE_COLOR, caps.color_model);
    EXPECT_EQ(printing::COLORMODE_MONOCHROME, caps.bw_model);
  }
}
