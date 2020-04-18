// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/themes/browser_theme_pack.h"

#include <stddef.h>

#include "base/files/scoped_temp_dir.h"
#include "base/json/json_file_value_serializer.h"
#include "base/json/json_reader.h"
#include "base/path_service.h"
#include "base/values.h"
#include "build/build_config.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/grit/theme_resources.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/color_utils.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/image/image_skia.h"
#include "ui/gfx/image/image_skia_rep.h"

using extensions::Extension;

// Maps scale factors (enum values) to file path.
// A similar typedef in BrowserThemePack is private.
typedef std::map<ui::ScaleFactor, base::FilePath> TestScaleFactorToFileMap;

// Maps image ids to maps of scale factors to file paths.
// A similar typedef in BrowserThemePack is private.
typedef std::map<int, TestScaleFactorToFileMap> TestFilePathMap;

class BrowserThemePackTest : public ::testing::Test {
 public:
  BrowserThemePackTest() : theme_pack_(new BrowserThemePack()) {
    std::vector<ui::ScaleFactor> scale_factors;
    scale_factors.push_back(ui::SCALE_FACTOR_100P);
    scale_factors.push_back(ui::SCALE_FACTOR_200P);
    scoped_set_supported_scale_factors_.reset(
        new ui::test::ScopedSetSupportedScaleFactors(scale_factors));
  }
  ~BrowserThemePackTest() override {}

  // Transformation for link underline colors.
  SkColor BuildThirdOpacity(SkColor color_link) {
    return SkColorSetA(color_link, SkColorGetA(color_link) / 3);
  }

  void GenerateDefaultFrameColor(std::map<int, SkColor>* colors,
                                 int color, int tint, bool otr) {
    (*colors)[color] = HSLShift(
        ThemeProperties::GetDefaultColor(ThemeProperties::COLOR_FRAME, false),
        ThemeProperties::GetDefaultTint(tint, otr));
  }

  // Returns a mapping from each COLOR_* constant to the default value for this
  // constant. Callers get this map, and then modify expected values and then
  // run the resulting thing through VerifyColorMap().
  std::map<int, SkColor> GetDefaultColorMap() {
    std::map<int, SkColor> colors;
    GenerateDefaultFrameColor(&colors, ThemeProperties::COLOR_FRAME,
                              ThemeProperties::TINT_FRAME, false);
    GenerateDefaultFrameColor(&colors,
                              ThemeProperties::COLOR_FRAME_INACTIVE,
                              ThemeProperties::TINT_FRAME_INACTIVE, false);
    GenerateDefaultFrameColor(&colors,
                              ThemeProperties::COLOR_FRAME_INCOGNITO,
                              ThemeProperties::TINT_FRAME, true);
    GenerateDefaultFrameColor(
        &colors,
        ThemeProperties::COLOR_FRAME_INCOGNITO_INACTIVE,
        ThemeProperties::TINT_FRAME_INACTIVE, true);

    // For the rest, use default colors.
    for (int i = ThemeProperties::COLOR_FRAME_INCOGNITO_INACTIVE + 1;
         i <= ThemeProperties::COLOR_BUTTON_BACKGROUND; ++i) {
      colors[i] = ThemeProperties::GetDefaultColor(i, false);
    }

    return colors;
  }

  void VerifyColorMap(const std::map<int, SkColor>& color_map) {
    for (std::map<int, SkColor>::const_iterator it = color_map.begin();
         it != color_map.end(); ++it) {
      SkColor color;
      if (!theme_pack_->GetColor(it->first, &color))
        color = ThemeProperties::GetDefaultColor(it->first, false);
      EXPECT_EQ(it->second, color) << "Color id = " << it->first;
    }
  }

  void LoadColorJSON(const std::string& json) {
    std::unique_ptr<base::Value> value = base::JSONReader::Read(json);
    ASSERT_TRUE(value->is_dict());
    LoadColorDictionary(static_cast<base::DictionaryValue*>(value.get()));
  }

  void LoadColorDictionary(base::DictionaryValue* value) {
    theme_pack_->BuildColorsFromJSON(value);
  }

  void LoadTintJSON(const std::string& json) {
    std::unique_ptr<base::Value> value = base::JSONReader::Read(json);
    ASSERT_TRUE(value->is_dict());
    LoadTintDictionary(static_cast<base::DictionaryValue*>(value.get()));
  }

  void LoadTintDictionary(base::DictionaryValue* value) {
    theme_pack_->BuildTintsFromJSON(value);
  }

  void LoadDisplayPropertiesJSON(const std::string& json) {
    std::unique_ptr<base::Value> value = base::JSONReader::Read(json);
    ASSERT_TRUE(value->is_dict());
    LoadDisplayPropertiesDictionary(
        static_cast<base::DictionaryValue*>(value.get()));
  }

  void LoadDisplayPropertiesDictionary(base::DictionaryValue* value) {
    theme_pack_->BuildDisplayPropertiesFromJSON(value);
  }

  void ParseImageNamesJSON(const std::string& json,
                           TestFilePathMap* out_file_paths) {
    std::unique_ptr<base::Value> value = base::JSONReader::Read(json);
    ASSERT_TRUE(value->is_dict());
    ParseImageNamesDictionary(static_cast<base::DictionaryValue*>(value.get()),
                              out_file_paths);
  }

  void ParseImageNamesDictionary(
      base::DictionaryValue* value,
      TestFilePathMap* out_file_paths) {
    theme_pack_->ParseImageNamesFromJSON(value, base::FilePath(),
                                         out_file_paths);

    // Build the source image list for HasCustomImage().
    theme_pack_->BuildSourceImagesArray(*out_file_paths);
  }

  bool LoadRawBitmapsTo(const TestFilePathMap& out_file_paths) {
    return theme_pack_->LoadRawBitmapsTo(out_file_paths, &theme_pack_->images_);
  }

  // This function returns void in order to be able use ASSERT_...
  // The BrowserThemePack is returned in |pack|.
  void BuildFromUnpackedExtension(const base::FilePath& extension_path,
                                  scoped_refptr<BrowserThemePack>* pack) {
    base::FilePath manifest_path = extension_path.AppendASCII("manifest.json");
    std::string error;
    JSONFileValueDeserializer deserializer(manifest_path);
    std::unique_ptr<base::DictionaryValue> valid_value =
        base::DictionaryValue::From(deserializer.Deserialize(NULL, &error));
    EXPECT_EQ("", error);
    ASSERT_TRUE(valid_value.get());
    scoped_refptr<Extension> extension(Extension::Create(
        extension_path, extensions::Manifest::INVALID_LOCATION, *valid_value,
        Extension::REQUIRE_KEY, &error));
    ASSERT_TRUE(extension.get());
    ASSERT_EQ("", error);
    *pack = new BrowserThemePack;
    BrowserThemePack::BuildFromExtension(extension.get(), *pack);
    ASSERT_TRUE((*pack)->is_valid());
  }

  base::FilePath GetStarGazingPath() {
    base::FilePath test_path;
    if (!base::PathService::Get(chrome::DIR_TEST_DATA, &test_path)) {
      NOTREACHED();
      return test_path;
    }

    test_path = test_path.AppendASCII("profiles");
    test_path = test_path.AppendASCII("profile_with_complex_theme");
    test_path = test_path.AppendASCII("Default");
    test_path = test_path.AppendASCII("Extensions");
    test_path = test_path.AppendASCII("mblmlcbknbnfebdfjnolmcapmdofhmme");
    test_path = test_path.AppendASCII("1.1");
    return base::FilePath(test_path);
  }

  base::FilePath GetHiDpiThemePath() {
    base::FilePath test_path;
    if (!base::PathService::Get(chrome::DIR_TEST_DATA, &test_path)) {
      NOTREACHED();
      return test_path;
    }
    test_path = test_path.AppendASCII("extensions");
    test_path = test_path.AppendASCII("theme_hidpi");
    return base::FilePath(test_path);
  }

  // Verifies the data in star gazing. We do this multiple times for different
  // BrowserThemePack objects to make sure it works in generated and mmapped
  // mode correctly.
  void VerifyStarGazing(BrowserThemePack* pack) {
    // First check that values we know exist, exist.
    SkColor color;
    EXPECT_TRUE(pack->GetColor(ThemeProperties::COLOR_BOOKMARK_TEXT,
                               &color));
    EXPECT_EQ(SK_ColorBLACK, color);

    EXPECT_TRUE(pack->GetColor(ThemeProperties::COLOR_NTP_BACKGROUND,
                               &color));
    EXPECT_EQ(SkColorSetRGB(57, 137, 194), color);

    color_utils::HSL expected = { 0.6, 0.553, 0.5 };
    color_utils::HSL actual;
    EXPECT_TRUE(pack->GetTint(ThemeProperties::TINT_BUTTONS, &actual));
    EXPECT_DOUBLE_EQ(expected.h, actual.h);
    EXPECT_DOUBLE_EQ(expected.s, actual.s);
    EXPECT_DOUBLE_EQ(expected.l, actual.l);

    int val;
    EXPECT_TRUE(pack->GetDisplayProperty(
        ThemeProperties::NTP_BACKGROUND_ALIGNMENT, &val));
    EXPECT_EQ(ThemeProperties::ALIGN_TOP, val);

    // The stargazing theme defines the following images:
    EXPECT_TRUE(pack->HasCustomImage(IDR_THEME_BUTTON_BACKGROUND));
    EXPECT_TRUE(pack->HasCustomImage(IDR_THEME_FRAME));
    EXPECT_TRUE(pack->HasCustomImage(IDR_THEME_NTP_BACKGROUND));
    EXPECT_TRUE(pack->HasCustomImage(IDR_THEME_TAB_BACKGROUND));
    EXPECT_TRUE(pack->HasCustomImage(IDR_THEME_TOOLBAR));
    EXPECT_TRUE(pack->HasCustomImage(IDR_THEME_WINDOW_CONTROL_BACKGROUND));

    // Here are a few images that we shouldn't expect because even though
    // they're included in the theme pack, they were autogenerated and
    // therefore shouldn't show up when calling HasCustomImage().
    // Verify they do appear when calling GetImageNamed(), though.
    EXPECT_FALSE(pack->HasCustomImage(IDR_THEME_FRAME_INACTIVE));
    EXPECT_FALSE(pack->GetImageNamed(IDR_THEME_FRAME_INACTIVE).IsEmpty());
    EXPECT_FALSE(pack->HasCustomImage(IDR_THEME_FRAME_INCOGNITO));
    EXPECT_FALSE(pack->GetImageNamed(IDR_THEME_FRAME_INCOGNITO).IsEmpty());
    EXPECT_FALSE(pack->HasCustomImage(IDR_THEME_FRAME_INCOGNITO_INACTIVE));
    EXPECT_FALSE(
        pack->GetImageNamed(IDR_THEME_FRAME_INCOGNITO_INACTIVE).IsEmpty());

    // The overlay images are missing and they do not fall back to the active
    // frame image.
    EXPECT_FALSE(pack->HasCustomImage(IDR_THEME_FRAME_OVERLAY));
    EXPECT_TRUE(pack->GetImageNamed(IDR_THEME_FRAME_OVERLAY).IsEmpty());
    EXPECT_FALSE(pack->HasCustomImage(IDR_THEME_FRAME_OVERLAY_INACTIVE));
    EXPECT_TRUE(
        pack->GetImageNamed(IDR_THEME_FRAME_OVERLAY_INACTIVE).IsEmpty());

#if !defined(OS_MACOSX)
    // The tab background image is missing, but will still be generated in
    // CreateTabBackgroundImages based on IDR_THEME_FRAME.
    EXPECT_FALSE(pack->HasCustomImage(IDR_THEME_TAB_BACKGROUND_INCOGNITO));
    EXPECT_FALSE(
        pack->GetImageNamed(IDR_THEME_TAB_BACKGROUND_INCOGNITO).IsEmpty());
#endif

    // Make sure we don't have phantom data.
    EXPECT_FALSE(pack->GetColor(ThemeProperties::COLOR_CONTROL_BACKGROUND,
                                &color));
    EXPECT_FALSE(pack->GetTint(ThemeProperties::TINT_FRAME, &actual));
  }

  void VerifyHiDpiTheme(BrowserThemePack* pack) {
    // The high DPI theme defines the following images:
    EXPECT_TRUE(pack->HasCustomImage(IDR_THEME_FRAME));
    EXPECT_TRUE(pack->HasCustomImage(IDR_THEME_FRAME_INACTIVE));
    EXPECT_TRUE(pack->HasCustomImage(IDR_THEME_FRAME_INCOGNITO));
    EXPECT_TRUE(pack->HasCustomImage(IDR_THEME_FRAME_INCOGNITO_INACTIVE));
    EXPECT_TRUE(pack->HasCustomImage(IDR_THEME_TOOLBAR));

    // The high DPI theme does not define the following images:
    EXPECT_FALSE(pack->HasCustomImage(IDR_THEME_TAB_BACKGROUND));
#if !defined(OS_MACOSX)
    EXPECT_FALSE(pack->HasCustomImage(IDR_THEME_TAB_BACKGROUND_INCOGNITO));
#endif
    EXPECT_FALSE(pack->HasCustomImage(IDR_THEME_TAB_BACKGROUND_V));
    EXPECT_FALSE(pack->HasCustomImage(IDR_THEME_NTP_BACKGROUND));
    EXPECT_FALSE(pack->HasCustomImage(IDR_THEME_FRAME_OVERLAY));
    EXPECT_FALSE(pack->HasCustomImage(IDR_THEME_FRAME_OVERLAY_INACTIVE));
    EXPECT_FALSE(pack->HasCustomImage(IDR_THEME_BUTTON_BACKGROUND));
    EXPECT_FALSE(pack->HasCustomImage(IDR_THEME_NTP_ATTRIBUTION));
    EXPECT_FALSE(pack->HasCustomImage(IDR_THEME_WINDOW_CONTROL_BACKGROUND));

    // Compare some known pixel colors at know locations for a theme
    // image where two different PNG files were specified for scales 100%
    // and 200% respectively.
    int idr = IDR_THEME_FRAME;
    gfx::Image image = pack->GetImageNamed(idr);
    EXPECT_FALSE(image.IsEmpty());
    const gfx::ImageSkia* image_skia = image.ToImageSkia();
    ASSERT_TRUE(image_skia);
    // Scale 100%.
    const gfx::ImageSkiaRep& rep1 = image_skia->GetRepresentation(1.0f);
    ASSERT_FALSE(rep1.is_null());
    EXPECT_EQ(80, rep1.sk_bitmap().width());
    EXPECT_EQ(80, rep1.sk_bitmap().height());
    EXPECT_EQ(SkColorSetRGB(255, 255, 255), rep1.sk_bitmap().getColor(4, 4));
    EXPECT_EQ(SkColorSetRGB(255, 255, 255), rep1.sk_bitmap().getColor(8, 8));
    EXPECT_EQ(SkColorSetRGB(0, 241, 237), rep1.sk_bitmap().getColor(16, 16));
    EXPECT_EQ(SkColorSetRGB(255, 255, 255), rep1.sk_bitmap().getColor(24, 24));
    EXPECT_EQ(SkColorSetRGB(0, 241, 237), rep1.sk_bitmap().getColor(32, 32));
    // Scale 200%.
    const gfx::ImageSkiaRep& rep2 = image_skia->GetRepresentation(2.0f);
    ASSERT_FALSE(rep2.is_null());
    EXPECT_EQ(160, rep2.sk_bitmap().width());
    EXPECT_EQ(160, rep2.sk_bitmap().height());
    EXPECT_EQ(SkColorSetRGB(255, 255, 255), rep2.sk_bitmap().getColor(4, 4));
    EXPECT_EQ(SkColorSetRGB(223, 42, 0), rep2.sk_bitmap().getColor(8, 8));
    EXPECT_EQ(SkColorSetRGB(223, 42, 0), rep2.sk_bitmap().getColor(16, 16));
    EXPECT_EQ(SkColorSetRGB(223, 42, 0), rep2.sk_bitmap().getColor(24, 24));
    EXPECT_EQ(SkColorSetRGB(255, 255, 255), rep2.sk_bitmap().getColor(32, 32));

    // TODO(sschmitz): I plan to remove the following (to the end of the fct)
    // Reason: this test may be brittle. It depends on details of how we scale
    // an 100% image to an 200% image. If there's filtering etc, this test would
    // break. Also High DPI is new, but scaling from 100% to 200% is not new
    // and need not be tested here. But in the interrim it is useful to verify
    // that this image was scaled and did not appear in the input.

    // Compare pixel colors and locations for a theme image that had
    // only one PNG file specified (for scale 100%). The representation
    // for scale of 200% was computed.
    idr = IDR_THEME_FRAME_INCOGNITO_INACTIVE;
    image = pack->GetImageNamed(idr);
    EXPECT_FALSE(image.IsEmpty());
    image_skia = image.ToImageSkia();
    ASSERT_TRUE(image_skia);
    // Scale 100%.
    const gfx::ImageSkiaRep& rep3 = image_skia->GetRepresentation(1.0f);
    ASSERT_FALSE(rep3.is_null());
    EXPECT_EQ(80, rep3.sk_bitmap().width());
    EXPECT_EQ(80, rep3.sk_bitmap().height());
    // We take samples of colors and locations along the diagonal whenever
    // the color changes. Note these colors are slightly different from
    // the input PNG file due to input processing.
    std::vector<std::pair<int, SkColor> > normal;
    int xy = 0;
    SkColor color = rep3.sk_bitmap().getColor(xy, xy);
    normal.push_back(std::make_pair(xy, color));
    for (int xy = 0; xy < 40; ++xy) {
      SkColor next_color = rep3.sk_bitmap().getColor(xy, xy);
      if (next_color != color) {
        color = next_color;
        normal.push_back(std::make_pair(xy, color));
      }
    }
    EXPECT_EQ(static_cast<size_t>(9), normal.size());
    // Scale 200%.
    const gfx::ImageSkiaRep& rep4 = image_skia->GetRepresentation(2.0f);
    ASSERT_FALSE(rep4.is_null());
    EXPECT_EQ(160, rep4.sk_bitmap().width());
    EXPECT_EQ(160, rep4.sk_bitmap().height());
    // We expect the same colors and at locations scaled by 2
    // since this bitmap was scaled by 2.
    for (size_t i = 0; i < normal.size(); ++i) {
      int xy = 2 * normal[i].first;
      SkColor color = normal[i].second;
      EXPECT_EQ(color, rep4.sk_bitmap().getColor(xy, xy));
    }
  }

 protected:
  typedef std::unique_ptr<ui::test::ScopedSetSupportedScaleFactors>
      ScopedSetSupportedScaleFactors;
  ScopedSetSupportedScaleFactors scoped_set_supported_scale_factors_;

  content::TestBrowserThreadBundle thread_bundle_;
  scoped_refptr<BrowserThemePack> theme_pack_;
};

// 'ntp_section' used to correspond to ThemeProperties::COLOR_NTP_SECTION,
// but COLOR_NTP_SECTION was since removed because it was never used.
// While it was in use, COLOR_NTP_HEADER used 'ntp_section' as a fallback when
// 'ntp_header' was absent.  We still preserve this fallback for themes that
// relied on this.
TEST_F(BrowserThemePackTest, UseSectionColorAsNTPHeader) {
  std::string color_json = "{ \"ntp_section\": [190, 190, 190] }";
  LoadColorJSON(color_json);

  std::map<int, SkColor> colors = GetDefaultColorMap();
  SkColor ntp_color = SkColorSetRGB(190, 190, 190);
  colors[ThemeProperties::COLOR_NTP_HEADER] = ntp_color;
  VerifyColorMap(colors);
}

TEST_F(BrowserThemePackTest, ProvideNtpHeaderColor) {
  std::string color_json = "{ \"ntp_header\": [120, 120, 120], "
                           "  \"ntp_section\": [190, 190, 190] }";
  LoadColorJSON(color_json);

  std::map<int, SkColor> colors = GetDefaultColorMap();
  colors[ThemeProperties::COLOR_NTP_HEADER] = SkColorSetRGB(120, 120, 120);
  VerifyColorMap(colors);
}

TEST_F(BrowserThemePackTest, SupportsAlpha) {
  std::string color_json =
      "{ \"toolbar\": [0, 20, 40, 1], "
      "  \"tab_text\": [60, 80, 100, 1], "
      "  \"tab_background_text\": [120, 140, 160, 0.0], "
      "  \"bookmark_text\": [180, 200, 220, 1.0], "
      "  \"ntp_text\": [240, 255, 0, 0.5] }";
  LoadColorJSON(color_json);

  std::map<int, SkColor> colors = GetDefaultColorMap();
  // Verify that valid alpha values are parsed correctly.
  // The toolbar color's alpha value is intentionally ignored by theme provider.
  colors[ThemeProperties::COLOR_TOOLBAR] = SkColorSetARGB(255, 0, 20, 40);
  colors[ThemeProperties::COLOR_TAB_TEXT] = SkColorSetARGB(255, 60, 80, 100);
  colors[ThemeProperties::COLOR_BACKGROUND_TAB_TEXT] =
      SkColorSetARGB(0, 120, 140, 160);
  colors[ThemeProperties::COLOR_BOOKMARK_TEXT] =
      SkColorSetARGB(255, 180, 200, 220);
  colors[ThemeProperties::COLOR_NTP_TEXT] = SkColorSetARGB(128, 240, 255, 0);
  VerifyColorMap(colors);
}

TEST_F(BrowserThemePackTest, OutOfRangeColors) {
  // Ensure colors with out-of-range values are simply ignored.
  std::string color_json = "{ \"toolbar\": [0, 20, 40, -1], "
                           "  \"tab_text\": [60, 80, 100, 2], "
                           "  \"tab_background_text\": [120, 140, 160, 47.6], "
                           "  \"bookmark_text\": [256, 0, 0], "
                           "  \"ntp_text\": [0, -100, 100] }";
  LoadColorJSON(color_json);

  VerifyColorMap(GetDefaultColorMap());
}

TEST_F(BrowserThemePackTest, CanReadTints) {
  std::string tint_json = "{ \"buttons\": [ 0.5, 0.5, 0.5 ] }";
  LoadTintJSON(tint_json);

  color_utils::HSL expected = { 0.5, 0.5, 0.5 };
  color_utils::HSL actual = { -1, -1, -1 };
  EXPECT_TRUE(theme_pack_->GetTint(
      ThemeProperties::TINT_BUTTONS, &actual));
  EXPECT_DOUBLE_EQ(expected.h, actual.h);
  EXPECT_DOUBLE_EQ(expected.s, actual.s);
  EXPECT_DOUBLE_EQ(expected.l, actual.l);
}

TEST_F(BrowserThemePackTest, CanReadDisplayProperties) {
  std::string json = "{ \"ntp_background_alignment\": \"bottom\", "
                     "  \"ntp_background_repeat\": \"repeat-x\", "
                     "  \"ntp_logo_alternate\": 0 }";
  LoadDisplayPropertiesJSON(json);

  int out_val;
  EXPECT_TRUE(theme_pack_->GetDisplayProperty(
      ThemeProperties::NTP_BACKGROUND_ALIGNMENT, &out_val));
  EXPECT_EQ(ThemeProperties::ALIGN_BOTTOM, out_val);

  EXPECT_TRUE(theme_pack_->GetDisplayProperty(
      ThemeProperties::NTP_BACKGROUND_TILING, &out_val));
  EXPECT_EQ(ThemeProperties::REPEAT_X, out_val);

  EXPECT_TRUE(theme_pack_->GetDisplayProperty(
      ThemeProperties::NTP_LOGO_ALTERNATE, &out_val));
  EXPECT_EQ(0, out_val);
}

TEST_F(BrowserThemePackTest, CanParsePaths) {
  std::string path_json = "{ \"theme_button_background\": \"one\", "
                          "  \"theme_toolbar\": \"two\" }";
  TestFilePathMap out_file_paths;
  ParseImageNamesJSON(path_json, &out_file_paths);

  size_t expected_file_paths = 2u;
  EXPECT_EQ(expected_file_paths, out_file_paths.size());
  // "12" and "5" are internal constants to BrowserThemePack and are
  // PRS_THEME_BUTTON_BACKGROUND and PRS_THEME_TOOLBAR, but they are
  // implementation details that shouldn't be exported.
  // By default the scale factor is for 100%.
  EXPECT_TRUE(base::FilePath(FILE_PATH_LITERAL("one")) ==
              out_file_paths[12][ui::SCALE_FACTOR_100P]);
  EXPECT_TRUE(base::FilePath(FILE_PATH_LITERAL("two")) ==
              out_file_paths[5][ui::SCALE_FACTOR_100P]);
}

TEST_F(BrowserThemePackTest, InvalidPathNames) {
  std::string path_json = "{ \"wrong\": [1], "
                          "  \"theme_button_background\": \"one\", "
                          "  \"not_a_thing\": \"blah\" }";
  TestFilePathMap out_file_paths;
  ParseImageNamesJSON(path_json, &out_file_paths);

  // We should have only parsed one valid path out of that mess above.
  EXPECT_EQ(1u, out_file_paths.size());
}

TEST_F(BrowserThemePackTest, InvalidColors) {
  std::string invalid_color = "{ \"toolbar\": [\"dog\", \"cat\", [12]], "
                              "  \"sound\": \"woof\" }";
  LoadColorJSON(invalid_color);
  std::map<int, SkColor> colors = GetDefaultColorMap();
  VerifyColorMap(colors);
}

TEST_F(BrowserThemePackTest, InvalidTints) {
  std::string tints = "{ \"buttons\": [ \"dog\", \"cat\", [\"x\"]], "
                       " \"frame\": [-2, 2, 3],"
                       " \"frame_incognito_inactive\": [-1, 2, 0.6],"
                       " \"invalid\": \"entry\" }";
  LoadTintJSON(tints);

  // We should ignore completely invalid (non-numeric) tints.
  color_utils::HSL actual = { -1, -1, -1 };
  EXPECT_FALSE(theme_pack_->GetTint(ThemeProperties::TINT_BUTTONS, &actual));

  // We should change invalid numeric HSL tint components to the special -1 "no
  // change" value.
  EXPECT_TRUE(theme_pack_->GetTint(ThemeProperties::TINT_FRAME, &actual));
  EXPECT_EQ(-1, actual.h);
  EXPECT_EQ(-1, actual.s);
  EXPECT_EQ(-1, actual.l);

  // We should correct partially incorrect inputs as well.
  EXPECT_TRUE(theme_pack_->GetTint(
      ThemeProperties::TINT_FRAME_INCOGNITO_INACTIVE, &actual));
  EXPECT_EQ(-1, actual.h);
  EXPECT_EQ(-1, actual.s);
  EXPECT_EQ(0.6, actual.l);
}

TEST_F(BrowserThemePackTest, InvalidDisplayProperties) {
  std::string invalid_properties = "{ \"ntp_background_alignment\": [15], "
                                   "  \"junk\": [15.3] }";
  LoadDisplayPropertiesJSON(invalid_properties);

  int out_val;
  EXPECT_FALSE(theme_pack_->GetDisplayProperty(
      ThemeProperties::NTP_BACKGROUND_ALIGNMENT, &out_val));
}

// These three tests should just not cause a segmentation fault.
TEST_F(BrowserThemePackTest, NullPaths) {
  TestFilePathMap out_file_paths;
  ParseImageNamesDictionary(NULL, &out_file_paths);
}

TEST_F(BrowserThemePackTest, NullTints) {
  LoadTintDictionary(NULL);
}

TEST_F(BrowserThemePackTest, NullColors) {
  LoadColorDictionary(NULL);
}

TEST_F(BrowserThemePackTest, NullDisplayProperties) {
  LoadDisplayPropertiesDictionary(NULL);
}

TEST_F(BrowserThemePackTest, TestHasCustomImage) {
  // HasCustomImage should only return true for images that exist in the
  // extension and not for autogenerated images.
  std::string images = "{ \"theme_frame\": \"one\" }";
  TestFilePathMap out_file_paths;
  ParseImageNamesJSON(images, &out_file_paths);

  EXPECT_TRUE(theme_pack_->HasCustomImage(IDR_THEME_FRAME));
  EXPECT_FALSE(theme_pack_->HasCustomImage(IDR_THEME_FRAME_INCOGNITO));
}

TEST_F(BrowserThemePackTest, TestNonExistantImages) {
  std::string images = "{ \"theme_frame\": \"does_not_exist\" }";
  TestFilePathMap out_file_paths;
  ParseImageNamesJSON(images, &out_file_paths);

  EXPECT_FALSE(LoadRawBitmapsTo(out_file_paths));
}

// TODO(erg): This test should actually test more of the built resources from
// the extension data, but for now, exists so valgrind can test some of the
// tricky memory stuff that BrowserThemePack does.
TEST_F(BrowserThemePackTest, CanBuildAndReadPack) {
  base::ScopedTempDir dir;
  ASSERT_TRUE(dir.CreateUniqueTempDir());
  base::FilePath file = dir.GetPath().AppendASCII("data.pak");

  // Part 1: Build the pack from an extension.
  {
    base::FilePath star_gazing_path = GetStarGazingPath();
    scoped_refptr<BrowserThemePack> pack;
    BuildFromUnpackedExtension(star_gazing_path, &pack);
    ASSERT_TRUE(pack->WriteToDisk(file));
    VerifyStarGazing(pack.get());
  }

  // Part 2: Try to read back the data pack that we just wrote to disk.
  {
    scoped_refptr<BrowserThemePack> pack =
        BrowserThemePack::BuildFromDataPack(
            file, "mblmlcbknbnfebdfjnolmcapmdofhmme");
    ASSERT_TRUE(pack.get());
    VerifyStarGazing(pack.get());
  }
}

TEST_F(BrowserThemePackTest, HiDpiThemeTest) {
  base::ScopedTempDir dir;
  ASSERT_TRUE(dir.CreateUniqueTempDir());
  base::FilePath file = dir.GetPath().AppendASCII("theme_data.pak");

  // Part 1: Build the pack from an extension.
  {
    base::FilePath hidpi_path = GetHiDpiThemePath();
    scoped_refptr<BrowserThemePack> pack;
    BuildFromUnpackedExtension(hidpi_path, &pack);
    ASSERT_TRUE(pack->WriteToDisk(file));
    VerifyHiDpiTheme(pack.get());
  }

  // Part 2: Try to read back the data pack that we just wrote to disk.
  {
    scoped_refptr<BrowserThemePack> pack =
        BrowserThemePack::BuildFromDataPack(file, "gllekhaobjnhgeag");
    ASSERT_TRUE(pack.get());
    VerifyHiDpiTheme(pack.get());
  }
}
