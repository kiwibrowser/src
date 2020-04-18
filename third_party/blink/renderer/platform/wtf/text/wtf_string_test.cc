/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

#include <limits>
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/platform/wtf/math_extras.h"
#include "third_party/blink/renderer/platform/wtf/text/cstring.h"

namespace WTF {

TEST(StringTest, CreationFromLiteral) {
  String string_from_literal("Explicit construction syntax");
  EXPECT_EQ(strlen("Explicit construction syntax"),
            string_from_literal.length());
  EXPECT_TRUE(string_from_literal == "Explicit construction syntax");
  EXPECT_TRUE(string_from_literal.Is8Bit());
  EXPECT_TRUE(String("Explicit construction syntax") == string_from_literal);
}

TEST(StringTest, ASCII) {
  CString output;

  // Null String.
  output = String().Ascii();
  EXPECT_STREQ("", output.data());

  // Empty String.
  output = g_empty_string.Ascii();
  EXPECT_STREQ("", output.data());

  // Regular String.
  output = String("foobar").Ascii();
  EXPECT_STREQ("foobar", output.data());
}

namespace {

void TestNumberToStringECMAScript(double number, const char* reference) {
  CString number_string = String::NumberToStringECMAScript(number).Latin1();
  EXPECT_STREQ(reference, number_string.data());
}

}  // anonymous namespace

TEST(StringTest, NumberToStringECMAScriptBoundaries) {
  typedef std::numeric_limits<double> Limits;

  // Infinity.
  TestNumberToStringECMAScript(Limits::infinity(), "Infinity");
  TestNumberToStringECMAScript(-Limits::infinity(), "-Infinity");

  // NaN.
  TestNumberToStringECMAScript(-Limits::quiet_NaN(), "NaN");

  // Zeros.
  TestNumberToStringECMAScript(0, "0");
  TestNumberToStringECMAScript(-0, "0");

  // Min-Max.
  TestNumberToStringECMAScript(Limits::min(), "2.2250738585072014e-308");
  TestNumberToStringECMAScript(Limits::max(), "1.7976931348623157e+308");
}

TEST(StringTest, NumberToStringECMAScriptRegularNumbers) {
  // Pi.
  TestNumberToStringECMAScript(piDouble, "3.141592653589793");
  TestNumberToStringECMAScript(piFloat, "3.1415927410125732");
  TestNumberToStringECMAScript(piOverTwoDouble, "1.5707963267948966");
  TestNumberToStringECMAScript(piOverTwoFloat, "1.5707963705062866");
  TestNumberToStringECMAScript(piOverFourDouble, "0.7853981633974483");
  TestNumberToStringECMAScript(piOverFourFloat, "0.7853981852531433");

  // e.
  const double kE = 2.71828182845904523536028747135266249775724709369995;
  TestNumberToStringECMAScript(kE, "2.718281828459045");

  // c, speed of light in m/s.
  const double kC = 299792458;
  TestNumberToStringECMAScript(kC, "299792458");

  // Golen ratio.
  const double kPhi = 1.6180339887498948482;
  TestNumberToStringECMAScript(kPhi, "1.618033988749895");
}

TEST(StringTest, ReplaceWithLiteral) {
  // Cases for 8Bit source.
  String test_string = "1224";
  EXPECT_TRUE(test_string.Is8Bit());
  test_string.Replace('2', "");
  EXPECT_STREQ("14", test_string.Utf8().data());

  test_string = "1224";
  EXPECT_TRUE(test_string.Is8Bit());
  test_string.Replace('2', "3");
  EXPECT_STREQ("1334", test_string.Utf8().data());

  test_string = "1224";
  EXPECT_TRUE(test_string.Is8Bit());
  test_string.Replace('2', "555");
  EXPECT_STREQ("15555554", test_string.Utf8().data());

  test_string = "1224";
  EXPECT_TRUE(test_string.Is8Bit());
  test_string.Replace('3', "NotFound");
  EXPECT_STREQ("1224", test_string.Utf8().data());

  // Cases for 16Bit source.
  // U+00E9 (=0xC3 0xA9 in UTF-8) is e with accent.
  test_string = String::FromUTF8("r\xC3\xA9sum\xC3\xA9");
  EXPECT_FALSE(test_string.Is8Bit());
  test_string.Replace(UChar(0x00E9), "e");
  EXPECT_STREQ("resume", test_string.Utf8().data());

  test_string = String::FromUTF8("r\xC3\xA9sum\xC3\xA9");
  EXPECT_FALSE(test_string.Is8Bit());
  test_string.Replace(UChar(0x00E9), "");
  EXPECT_STREQ("rsum", test_string.Utf8().data());

  test_string = String::FromUTF8("r\xC3\xA9sum\xC3\xA9");
  EXPECT_FALSE(test_string.Is8Bit());
  test_string.Replace('3', "NotFound");
  EXPECT_STREQ("r\xC3\xA9sum\xC3\xA9", test_string.Utf8().data());
}

TEST(StringTest, ComparisonOfSameStringVectors) {
  Vector<String> string_vector;
  string_vector.push_back("one");
  string_vector.push_back("two");

  Vector<String> same_string_vector;
  same_string_vector.push_back("one");
  same_string_vector.push_back("two");

  EXPECT_EQ(string_vector, same_string_vector);
}

TEST(WTF, SimplifyWhiteSpace) {
  String extra_spaces("  Hello  world  ");
  EXPECT_EQ(String("Hello world"), extra_spaces.SimplifyWhiteSpace());
  EXPECT_EQ(String("  Hello  world  "),
            extra_spaces.SimplifyWhiteSpace(WTF::kDoNotStripWhiteSpace));

  String extra_spaces_and_newlines(" \nHello\n world\n ");
  EXPECT_EQ(String("Hello world"),
            extra_spaces_and_newlines.SimplifyWhiteSpace());
  EXPECT_EQ(
      String("  Hello  world  "),
      extra_spaces_and_newlines.SimplifyWhiteSpace(WTF::kDoNotStripWhiteSpace));

  String extra_spaces_and_tabs(" \nHello\t world\t ");
  EXPECT_EQ(String("Hello world"), extra_spaces_and_tabs.SimplifyWhiteSpace());
  EXPECT_EQ(
      String("  Hello  world  "),
      extra_spaces_and_tabs.SimplifyWhiteSpace(WTF::kDoNotStripWhiteSpace));
}

struct CaseFoldingTestData {
  const char* source_description;
  const char* source;
  const char** locale_list;
  size_t locale_list_length;
  const char* expected;
};

// \xC4\xB0 = U+0130 (capital dotted I)
// \xC4\xB1 = U+0131 (lowercase dotless I)
const char* g_turkic_input = "Isi\xC4\xB0 \xC4\xB0s\xC4\xB1I";
const char* g_greek_input =
    "\xCE\x9F\xCE\x94\xCE\x8C\xCE\xA3 \xCE\x9F\xCE\xB4\xCF\x8C\xCF\x82 "
    "\xCE\xA3\xCE\xBF \xCE\xA3\xCE\x9F o\xCE\xA3 \xCE\x9F\xCE\xA3 \xCF\x83 "
    "\xE1\xBC\x95\xCE\xBE";
const char* g_lithuanian_input =
    "I \xC3\x8F J J\xCC\x88 \xC4\xAE \xC4\xAE\xCC\x88 \xC3\x8C \xC3\x8D "
    "\xC4\xA8 xi\xCC\x87\xCC\x88 xj\xCC\x87\xCC\x88 x\xC4\xAF\xCC\x87\xCC\x88 "
    "xi\xCC\x87\xCC\x80 xi\xCC\x87\xCC\x81 xi\xCC\x87\xCC\x83 XI X\xC3\x8F XJ "
    "XJ\xCC\x88 X\xC4\xAE X\xC4\xAE\xCC\x88";

const char* g_turkic_locales[] = {
    "tr", "tr-TR", "tr_TR", "tr@foo=bar", "tr-US", "TR", "tr-tr", "tR",
    "az", "az-AZ", "az_AZ", "az@foo=bar", "az-US", "Az", "AZ-AZ",
};
const char* g_non_turkic_locales[] = {
    "en", "en-US", "en_US", "en@foo=bar", "EN", "En",
    "ja", "el",    "fil",   "fi",         "lt",
};
const char* g_greek_locales[] = {
    "el", "el-GR", "el_GR", "el@foo=bar", "el-US", "EL", "el-gr", "eL",
};
const char* g_non_greek_locales[] = {
    "en", "en-US", "en_US", "en@foo=bar", "EN", "En",
    "ja", "tr",    "az",    "fil",        "fi", "lt",
};
const char* g_lithuanian_locales[] = {
    "lt", "lt-LT", "lt_LT", "lt@foo=bar", "lt-US", "LT", "lt-lt", "lT",
};
// Should not have "tr" or "az" because "lt" and 'tr/az' rules conflict with
// each other.
const char* g_non_lithuanian_locales[] = {
    "en", "en-US", "en_US", "en@foo=bar", "EN", "En", "ja", "fil", "fi", "el",
};

TEST(StringTest, ToUpperLocale) {
  CaseFoldingTestData test_data_list[] = {
      {
          "Turkic input", g_turkic_input, g_turkic_locales,
          sizeof(g_turkic_locales) / sizeof(const char*),
          "IS\xC4\xB0\xC4\xB0 \xC4\xB0SII",
      },
      {
          "Turkic input", g_turkic_input, g_non_turkic_locales,
          sizeof(g_non_turkic_locales) / sizeof(const char*),
          "ISI\xC4\xB0 \xC4\xB0SII",
      },
      {
          "Greek input", g_greek_input, g_greek_locales,
          sizeof(g_greek_locales) / sizeof(const char*),
          "\xCE\x9F\xCE\x94\xCE\x9F\xCE\xA3 \xCE\x9F\xCE\x94\xCE\x9F\xCE\xA3 "
          "\xCE\xA3\xCE\x9F \xCE\xA3\xCE\x9F \x4F\xCE\xA3 \xCE\x9F\xCE\xA3 "
          "\xCE\xA3 \xCE\x95\xCE\x9E",
      },
      {
          "Greek input", g_greek_input, g_non_greek_locales,
          sizeof(g_non_greek_locales) / sizeof(const char*),
          "\xCE\x9F\xCE\x94\xCE\x8C\xCE\xA3 \xCE\x9F\xCE\x94\xCE\x8C\xCE\xA3 "
          "\xCE\xA3\xCE\x9F \xCE\xA3\xCE\x9F \x4F\xCE\xA3 \xCE\x9F\xCE\xA3 "
          "\xCE\xA3 \xE1\xBC\x9D\xCE\x9E",
      },
      {
          "Lithuanian input", g_lithuanian_input, g_lithuanian_locales,
          sizeof(g_lithuanian_locales) / sizeof(const char*),
          "I \xC3\x8F J J\xCC\x88 \xC4\xAE \xC4\xAE\xCC\x88 \xC3\x8C \xC3\x8D "
          "\xC4\xA8 XI\xCC\x88 XJ\xCC\x88 X\xC4\xAE\xCC\x88 XI\xCC\x80 "
          "XI\xCC\x81 XI\xCC\x83 XI X\xC3\x8F XJ XJ\xCC\x88 X\xC4\xAE "
          "X\xC4\xAE\xCC\x88",
      },
      {
          "Lithuanian input", g_lithuanian_input, g_non_lithuanian_locales,
          sizeof(g_non_lithuanian_locales) / sizeof(const char*),
          "I \xC3\x8F J J\xCC\x88 \xC4\xAE \xC4\xAE\xCC\x88 \xC3\x8C \xC3\x8D "
          "\xC4\xA8 XI\xCC\x87\xCC\x88 XJ\xCC\x87\xCC\x88 "
          "X\xC4\xAE\xCC\x87\xCC\x88 XI\xCC\x87\xCC\x80 XI\xCC\x87\xCC\x81 "
          "XI\xCC\x87\xCC\x83 XI X\xC3\x8F XJ XJ\xCC\x88 X\xC4\xAE "
          "X\xC4\xAE\xCC\x88",
      },
  };

  for (size_t i = 0; i < sizeof(test_data_list) / sizeof(test_data_list[0]);
       ++i) {
    const char* expected = test_data_list[i].expected;
    String source = String::FromUTF8(test_data_list[i].source);
    for (size_t j = 0; j < test_data_list[i].locale_list_length; ++j) {
      const char* locale = test_data_list[i].locale_list[j];
      EXPECT_STREQ(expected, source.UpperUnicode(locale).Utf8().data())
          << test_data_list[i].source_description << "; locale=" << locale;
    }
  }
}

TEST(StringTest, ToLowerLocale) {
  CaseFoldingTestData test_data_list[] = {
      {
          "Turkic input", g_turkic_input, g_turkic_locales,
          sizeof(g_turkic_locales) / sizeof(const char*),
          "\xC4\xB1sii is\xC4\xB1\xC4\xB1",
      },
      {
          "Turkic input", g_turkic_input, g_non_turkic_locales,
          sizeof(g_non_turkic_locales) / sizeof(const char*),
          // U+0130 is lowercased to U+0069 followed by U+0307
          "isii\xCC\x87 i\xCC\x87s\xC4\xB1i",
      },
      {
          "Greek input", g_greek_input, g_greek_locales,
          sizeof(g_greek_locales) / sizeof(const char*),
          "\xCE\xBF\xCE\xB4\xCF\x8C\xCF\x82 \xCE\xBF\xCE\xB4\xCF\x8C\xCF\x82 "
          "\xCF\x83\xCE\xBF \xCF\x83\xCE\xBF \x6F\xCF\x82 \xCE\xBF\xCF\x82 "
          "\xCF\x83 \xE1\xBC\x95\xCE\xBE",
      },
      {
          "Greek input", g_greek_input, g_non_greek_locales,
          sizeof(g_greek_locales) / sizeof(const char*),
          "\xCE\xBF\xCE\xB4\xCF\x8C\xCF\x82 \xCE\xBF\xCE\xB4\xCF\x8C\xCF\x82 "
          "\xCF\x83\xCE\xBF \xCF\x83\xCE\xBF \x6F\xCF\x82 \xCE\xBF\xCF\x82 "
          "\xCF\x83 \xE1\xBC\x95\xCE\xBE",
      },
      {
          "Lithuanian input", g_lithuanian_input, g_lithuanian_locales,
          sizeof(g_lithuanian_locales) / sizeof(const char*),
          "i \xC3\xAF j j\xCC\x87\xCC\x88 \xC4\xAF \xC4\xAF\xCC\x87\xCC\x88 "
          "i\xCC\x87\xCC\x80 i\xCC\x87\xCC\x81 i\xCC\x87\xCC\x83 "
          "xi\xCC\x87\xCC\x88 xj\xCC\x87\xCC\x88 x\xC4\xAF\xCC\x87\xCC\x88 "
          "xi\xCC\x87\xCC\x80 xi\xCC\x87\xCC\x81 xi\xCC\x87\xCC\x83 xi "
          "x\xC3\xAF xj xj\xCC\x87\xCC\x88 x\xC4\xAF x\xC4\xAF\xCC\x87\xCC\x88",
      },
      {
          "Lithuanian input", g_lithuanian_input, g_non_lithuanian_locales,
          sizeof(g_non_lithuanian_locales) / sizeof(const char*),
          "\x69 \xC3\xAF \x6A \x6A\xCC\x88 \xC4\xAF \xC4\xAF\xCC\x88 \xC3\xAC "
          "\xC3\xAD \xC4\xA9 \x78\x69\xCC\x87\xCC\x88 \x78\x6A\xCC\x87\xCC\x88 "
          "\x78\xC4\xAF\xCC\x87\xCC\x88 \x78\x69\xCC\x87\xCC\x80 "
          "\x78\x69\xCC\x87\xCC\x81 \x78\x69\xCC\x87\xCC\x83 \x78\x69 "
          "\x78\xC3\xAF \x78\x6A \x78\x6A\xCC\x88 \x78\xC4\xAF "
          "\x78\xC4\xAF\xCC\x88",
      },
  };

  for (size_t i = 0; i < sizeof(test_data_list) / sizeof(test_data_list[0]);
       ++i) {
    const char* expected = test_data_list[i].expected;
    String source = String::FromUTF8(test_data_list[i].source);
    for (size_t j = 0; j < test_data_list[i].locale_list_length; ++j) {
      const char* locale = test_data_list[i].locale_list[j];
      EXPECT_STREQ(expected, source.LowerUnicode(locale).Utf8().data())
          << test_data_list[i].source_description << "; locale=" << locale;
    }
  }
}

TEST(StringTest, StartsWithIgnoringUnicodeCase) {
  // [U+017F U+212A i a] starts with "sk".
  EXPECT_TRUE(
      String::FromUTF8("\xC5\xBF\xE2\x84\xAAia").StartsWithIgnoringCase("sk"));
}

TEST(StringTest, StartsWithIgnoringASCIICase) {
  String all_ascii("LINK");
  String all_ascii_lower_case("link");
  EXPECT_TRUE(all_ascii.StartsWithIgnoringASCIICase(all_ascii_lower_case));
  String all_ascii_mixed_case("lInK");
  EXPECT_TRUE(all_ascii.StartsWithIgnoringASCIICase(all_ascii_mixed_case));
  String all_ascii_different("foo");
  EXPECT_FALSE(all_ascii.StartsWithIgnoringASCIICase(all_ascii_different));
  String non_ascii = String::FromUTF8("LIN\xE2\x84\xAA");
  EXPECT_FALSE(all_ascii.StartsWithIgnoringASCIICase(non_ascii));
  EXPECT_TRUE(
      all_ascii.StartsWithIgnoringASCIICase(non_ascii.DeprecatedLower()));

  EXPECT_FALSE(non_ascii.StartsWithIgnoringASCIICase(all_ascii));
  EXPECT_FALSE(non_ascii.StartsWithIgnoringASCIICase(all_ascii_lower_case));
  EXPECT_FALSE(non_ascii.StartsWithIgnoringASCIICase(all_ascii_mixed_case));
  EXPECT_FALSE(non_ascii.StartsWithIgnoringASCIICase(all_ascii_different));
}

TEST(StringTest, EndsWithIgnoringASCIICase) {
  String all_ascii("LINK");
  String all_ascii_lower_case("link");
  EXPECT_TRUE(all_ascii.EndsWithIgnoringASCIICase(all_ascii_lower_case));
  String all_ascii_mixed_case("lInK");
  EXPECT_TRUE(all_ascii.EndsWithIgnoringASCIICase(all_ascii_mixed_case));
  String all_ascii_different("foo");
  EXPECT_FALSE(all_ascii.EndsWithIgnoringASCIICase(all_ascii_different));
  String non_ascii = String::FromUTF8("LIN\xE2\x84\xAA");
  EXPECT_FALSE(all_ascii.EndsWithIgnoringASCIICase(non_ascii));
  EXPECT_TRUE(all_ascii.EndsWithIgnoringASCIICase(non_ascii.DeprecatedLower()));

  EXPECT_FALSE(non_ascii.EndsWithIgnoringASCIICase(all_ascii));
  EXPECT_FALSE(non_ascii.EndsWithIgnoringASCIICase(all_ascii_lower_case));
  EXPECT_FALSE(non_ascii.EndsWithIgnoringASCIICase(all_ascii_mixed_case));
  EXPECT_FALSE(non_ascii.EndsWithIgnoringASCIICase(all_ascii_different));
}

TEST(StringTest, EqualIgnoringASCIICase) {
  String all_ascii("LINK");
  String all_ascii_lower_case("link");
  EXPECT_TRUE(EqualIgnoringASCIICase(all_ascii, all_ascii_lower_case));
  String all_ascii_mixed_case("lInK");
  EXPECT_TRUE(EqualIgnoringASCIICase(all_ascii, all_ascii_mixed_case));
  String all_ascii_different("foo");
  EXPECT_FALSE(EqualIgnoringASCIICase(all_ascii, all_ascii_different));
  String non_ascii = String::FromUTF8("LIN\xE2\x84\xAA");
  EXPECT_FALSE(EqualIgnoringASCIICase(all_ascii, non_ascii));
  EXPECT_TRUE(EqualIgnoringASCIICase(all_ascii, non_ascii.DeprecatedLower()));

  EXPECT_FALSE(EqualIgnoringASCIICase(non_ascii, all_ascii));
  EXPECT_FALSE(EqualIgnoringASCIICase(non_ascii, all_ascii_lower_case));
  EXPECT_FALSE(EqualIgnoringASCIICase(non_ascii, all_ascii_mixed_case));
  EXPECT_FALSE(EqualIgnoringASCIICase(non_ascii, all_ascii_different));
}

TEST(StringTest, FindIgnoringASCIICase) {
  String needle = String::FromUTF8("a\xCC\x88qa\xCC\x88");

  // Multiple matches, non-overlapping
  String haystack1 = String::FromUTF8(
      "aA\xCC\x88QA\xCC\x88sA\xCC\x88qa\xCC\x88rfi\xC3\xA4q\xC3\xA4");
  EXPECT_EQ(1u, haystack1.FindIgnoringASCIICase(needle));
  EXPECT_EQ(7u, haystack1.FindIgnoringASCIICase(needle, 2));
  EXPECT_EQ(kNotFound, haystack1.FindIgnoringASCIICase(needle, 8));

  // Multiple matches, overlapping
  String haystack2 = String::FromUTF8("aA\xCC\x88QA\xCC\x88qa\xCC\x88rfi");
  EXPECT_EQ(1u, haystack2.FindIgnoringASCIICase(needle));
  EXPECT_EQ(4u, haystack2.FindIgnoringASCIICase(needle, 2));
  EXPECT_EQ(kNotFound, haystack2.FindIgnoringASCIICase(needle, 5));
}

TEST(StringTest, DeprecatedLower) {
  EXPECT_STREQ("link", String("LINK").DeprecatedLower().Ascii().data());
  EXPECT_STREQ("link", String("lInk").DeprecatedLower().Ascii().data());
  EXPECT_STREQ("lin\xE1k",
               String("lIn\xC1k").DeprecatedLower().Latin1().data());
  // U+212A -> k
  EXPECT_STREQ(
      "link",
      String::FromUTF8("LIN\xE2\x84\xAA").DeprecatedLower().Utf8().data());
}

TEST(StringTest, Ensure16Bit) {
  String string8("8bit");
  EXPECT_TRUE(string8.Is8Bit());
  string8.Ensure16Bit();
  EXPECT_FALSE(string8.Is8Bit());
  EXPECT_EQ("8bit", string8);

  String string16(reinterpret_cast<const UChar*>(u"16bit"));
  EXPECT_FALSE(string16.Is8Bit());
  string16.Ensure16Bit();
  EXPECT_FALSE(string16.Is8Bit());
  EXPECT_EQ("16bit", string16);

  String empty8(StringImpl::empty_);
  EXPECT_TRUE(empty8.Is8Bit());
  empty8.Ensure16Bit();
  EXPECT_FALSE(empty8.Is8Bit());
  EXPECT_TRUE(empty8.IsEmpty());
  EXPECT_FALSE(empty8.IsNull());

  String empty16(StringImpl::empty16_bit_);
  EXPECT_FALSE(empty16.Is8Bit());
  empty16.Ensure16Bit();
  EXPECT_FALSE(empty16.Is8Bit());
  EXPECT_TRUE(empty16.IsEmpty());
  EXPECT_FALSE(empty16.IsNull());

  String null_string;
  null_string.Ensure16Bit();
  EXPECT_TRUE(null_string.IsNull());
}

CString ToCStringThroughPrinter(const String& string) {
  std::ostringstream output;
  output << string;
  const std::string& result = output.str();
  return CString(result.data(), result.length());
}

TEST(StringTest, StringPrinter) {
  EXPECT_EQ(CString("\"Hello!\""), ToCStringThroughPrinter("Hello!"));
  EXPECT_EQ(CString("\"\\\"\""), ToCStringThroughPrinter("\""));
  EXPECT_EQ(CString("\"\\\\\""), ToCStringThroughPrinter("\\"));
  EXPECT_EQ(
      CString("\"\\u0000\\u0001\\u0002\\u0003\\u0004\\u0005\\u0006\\u0007\""),
      ToCStringThroughPrinter(String("\x00\x01\x02\x03\x04\x05\x06\x07", 8)));
  EXPECT_EQ(
      CString("\"\\u0008\\t\\n\\u000B\\u000C\\r\\u000E\\u000F\""),
      ToCStringThroughPrinter(String("\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F", 8)));
  EXPECT_EQ(
      CString("\"\\u0010\\u0011\\u0012\\u0013\\u0014\\u0015\\u0016\\u0017\""),
      ToCStringThroughPrinter(String("\x10\x11\x12\x13\x14\x15\x16\x17", 8)));
  EXPECT_EQ(
      CString("\"\\u0018\\u0019\\u001A\\u001B\\u001C\\u001D\\u001E\\u001F\""),
      ToCStringThroughPrinter(String("\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F", 8)));
  EXPECT_EQ(CString("\"\\u007F\\u0080\\u0081\""),
            ToCStringThroughPrinter("\x7F\x80\x81"));
  EXPECT_EQ(CString("\"\""), ToCStringThroughPrinter(g_empty_string));
  EXPECT_EQ(CString("<null>"), ToCStringThroughPrinter(String()));

  static const UChar kUnicodeSample[] = {0x30C6, 0x30B9,
                                         0x30C8};  // "Test" in Japanese.
  EXPECT_EQ(CString("\"\\u30C6\\u30B9\\u30C8\""),
            ToCStringThroughPrinter(
                String(kUnicodeSample, arraysize(kUnicodeSample))));
}

}  // namespace WTF
