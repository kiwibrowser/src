// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <utility>
#include <vector>

#include "base/strings/string_piece.h"
#include "services/network/cross_origin_read_blocking.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::StringPiece;
using MimeType = network::CrossOriginReadBlocking::MimeType;
using SniffingResult = network::CrossOriginReadBlocking::SniffingResult;

namespace network {

TEST(CrossOriginReadBlockingTest, IsBlockableScheme) {
  GURL data_url("data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAA==");
  GURL ftp_url("ftp://google.com");
  GURL mailto_url("mailto:google@google.com");
  GURL about_url("about:chrome");
  GURL http_url("http://google.com");
  GURL https_url("https://google.com");

  EXPECT_FALSE(CrossOriginReadBlocking::IsBlockableScheme(data_url));
  EXPECT_FALSE(CrossOriginReadBlocking::IsBlockableScheme(ftp_url));
  EXPECT_FALSE(CrossOriginReadBlocking::IsBlockableScheme(mailto_url));
  EXPECT_FALSE(CrossOriginReadBlocking::IsBlockableScheme(about_url));
  EXPECT_TRUE(CrossOriginReadBlocking::IsBlockableScheme(http_url));
  EXPECT_TRUE(CrossOriginReadBlocking::IsBlockableScheme(https_url));
}

TEST(CrossOriginReadBlockingTest, IsValidCorsHeaderSet) {
  url::Origin frame_origin = url::Origin::Create(GURL("http://www.google.com"));

  EXPECT_TRUE(CrossOriginReadBlocking::IsValidCorsHeaderSet(frame_origin, "*"));
  EXPECT_FALSE(
      CrossOriginReadBlocking::IsValidCorsHeaderSet(frame_origin, "\"*\""));
  EXPECT_FALSE(CrossOriginReadBlocking::IsValidCorsHeaderSet(
      frame_origin, "http://mail.google.com"));
  EXPECT_TRUE(CrossOriginReadBlocking::IsValidCorsHeaderSet(
      frame_origin, "http://www.google.com"));
  EXPECT_FALSE(CrossOriginReadBlocking::IsValidCorsHeaderSet(
      frame_origin, "https://www.google.com"));
  EXPECT_FALSE(CrossOriginReadBlocking::IsValidCorsHeaderSet(
      frame_origin, "http://yahoo.com"));
  EXPECT_FALSE(CrossOriginReadBlocking::IsValidCorsHeaderSet(frame_origin,
                                                             "www.google.com"));
}

TEST(CrossOriginReadBlockingTest, SniffForHTML) {
  using CORB = CrossOriginReadBlocking;

  // Something that technically matches the start of a valid HTML tag.
  EXPECT_EQ(SniffingResult::kYes,
            CORB::SniffForHTML("  \t\r\n    <HtMladfokadfkado"));

  // HTML comment followed by whitespace and valid HTML tags.
  EXPECT_EQ(SniffingResult::kYes,
            CORB::SniffForHTML(" <!-- this is comment -->\n<html><body>"));

  // HTML comment, whitespace, more HTML comments, HTML tags.
  EXPECT_EQ(
      SniffingResult::kYes,
      CORB::SniffForHTML(
          "<!-- this is comment -->\n<!-- this is comment -->\n<html><body>"));

  // HTML comment followed by valid HTML tag.
  EXPECT_EQ(
      SniffingResult::kYes,
      CORB::SniffForHTML("<!-- this is comment <!-- -->\n<script></script>"));

  // Whitespace followed by valid Javascript.
  EXPECT_EQ(SniffingResult::kNo,
            CORB::SniffForHTML("        var name=window.location;\nadfadf"));

  // HTML comment followed by valid Javascript.
  EXPECT_EQ(
      SniffingResult::kNo,
      CORB::SniffForHTML(
          " <!-- this is comment\n document.write(1);\n// -->\nwindow.open()"));

  // HTML/Javascript polyglot should return kNo.
  EXPECT_EQ(SniffingResult::kNo,
            CORB::SniffForHTML(
                "<!--/*--><html><body><script type='text/javascript'><!--//*/\n"
                "var blah = 123;\n"
                "//--></script></body></html>"));

  // Tests to cover more of MaybeSkipHtmlComment.
  EXPECT_EQ(SniffingResult::kMaybe, CORB::SniffForHTML("<!-- -/* --><html>"));
  EXPECT_EQ(SniffingResult::kMaybe, CORB::SniffForHTML("<!-- --/* --><html>"));
  EXPECT_EQ(SniffingResult::kYes, CORB::SniffForHTML("<!-- -/* -->\n<html>"));
  EXPECT_EQ(SniffingResult::kYes, CORB::SniffForHTML("<!-- --/* -->\n<html>"));
  EXPECT_EQ(SniffingResult::kMaybe, CORB::SniffForHTML("<!----> <html>"));
  EXPECT_EQ(SniffingResult::kYes, CORB::SniffForHTML("<!---->\n<html>"));
  EXPECT_EQ(SniffingResult::kYes, CORB::SniffForHTML("<!---->\r<html>"));
  EXPECT_EQ(SniffingResult::kYes,
            CORB::SniffForHTML("<!-- ---/-->\n<html><body>"));

  // HTML spec only allows *ASCII* whitespace before the first html element.
  // See also https://html.spec.whatwg.org/multipage/syntax.html and
  // https://infra.spec.whatwg.org/#ascii-whitespace.
  EXPECT_EQ(SniffingResult::kNo, CORB::SniffForHTML("<!---->\u2028<html>"));
  EXPECT_EQ(SniffingResult::kNo, CORB::SniffForHTML("<!---->\u2029<html>"));

  // Order of line terminators.
  EXPECT_EQ(SniffingResult::kYes, CORB::SniffForHTML("<!-- -->\n<b>\rx"));
  EXPECT_EQ(SniffingResult::kYes, CORB::SniffForHTML("<!-- -->\r<b>\nx"));
  EXPECT_EQ(SniffingResult::kNo, CORB::SniffForHTML("<!-- -->\nx\r<b>"));
  EXPECT_EQ(SniffingResult::kNo, CORB::SniffForHTML("<!-- -->\rx\n<b>"));
  EXPECT_EQ(SniffingResult::kYes, CORB::SniffForHTML("<!-- -->\n<b>\u2028x"));
  EXPECT_EQ(SniffingResult::kNo, CORB::SniffForHTML("<!-- -->\u2028<b>\n<b>"));

  // In UTF8 encoding <LS> is 0xE2 0x80 0xA8 and <PS> is 0xE2 0x80 0xA9.
  // Let's verify that presence of 0xE2 alone doesn't throw
  // FindFirstJavascriptLineTerminator into an infinite loop.
  EXPECT_EQ(SniffingResult::kYes, CORB::SniffForHTML("<!-- --> \xe2 \n<b"));
  EXPECT_EQ(SniffingResult::kYes, CORB::SniffForHTML("<!-- --> \xe2\x80 \n<b"));
  EXPECT_EQ(SniffingResult::kYes, CORB::SniffForHTML("<!-- --> \x80 \n<b"));

  // Commented out html tag followed by non-html (" x").
  StringPiece commented_out_html_tag_data("<!-- <html> <?xml> \n<html>-->\nx");
  EXPECT_EQ(SniffingResult::kNo,
            CORB::SniffForHTML(commented_out_html_tag_data));

  // Prefixes of |commented_out_html_tag_data| should be indeterminate.
  // This covers testing "<!-" as well as "<!-- not terminated yet...".
  StringPiece almost_html = commented_out_html_tag_data;
  while (!almost_html.empty()) {
    almost_html.remove_suffix(1);
    EXPECT_EQ(SniffingResult::kMaybe, CORB::SniffForHTML(almost_html))
        << almost_html;
  }

  // Explicit tests for an unfinished comment (some also covered by the prefix
  // tests above).
  EXPECT_EQ(SniffingResult::kMaybe, CORB::SniffForHTML(""));
  EXPECT_EQ(SniffingResult::kMaybe, CORB::SniffForHTML("<!"));
  EXPECT_EQ(SniffingResult::kMaybe, CORB::SniffForHTML("<!-- unterminated..."));
  EXPECT_EQ(SniffingResult::kMaybe,
            CORB::SniffForHTML("<!-- blah --> <html> no newline yet"));
}

TEST(CrossOriginReadBlockingTest, SniffForXML) {
  StringPiece xml_data("   \t \r \n     <?xml version=\"1.0\"?>\n <catalog");
  StringPiece non_xml_data("        var name=window.location;\nadfadf");
  StringPiece empty_data("");

  EXPECT_EQ(SniffingResult::kYes,
            CrossOriginReadBlocking::SniffForXML(xml_data));
  EXPECT_EQ(SniffingResult::kNo,
            CrossOriginReadBlocking::SniffForXML(non_xml_data));

  // Empty string should be indeterminate.
  EXPECT_EQ(SniffingResult::kMaybe,
            CrossOriginReadBlocking::SniffForXML(empty_data));
}

TEST(CrossOriginReadBlockingTest, SniffForJSON) {
  StringPiece json_data("\t\t\r\n   { \"name\" : \"chrome\", ");
  StringPiece json_corrupt_after_first_key(
      "\t\t\r\n   { \"name\" :^^^^!!@#\1\", ");
  StringPiece json_data2("{ \"key   \\\"  \"          \t\t\r\n:");
  StringPiece non_json_data0("\t\t\r\n   { name : \"chrome\", ");
  StringPiece non_json_data1("\t\t\r\n   foo({ \"name\" : \"chrome\", ");

  EXPECT_EQ(SniffingResult::kYes,
            CrossOriginReadBlocking::SniffForJSON(json_data));
  EXPECT_EQ(SniffingResult::kYes, CrossOriginReadBlocking::SniffForJSON(
                                      json_corrupt_after_first_key));

  EXPECT_EQ(SniffingResult::kYes,
            CrossOriginReadBlocking::SniffForJSON(json_data2));

  // All prefixes prefixes of |json_data2| ought to be indeterminate.
  StringPiece almost_json = json_data2;
  while (!almost_json.empty()) {
    almost_json.remove_suffix(1);
    EXPECT_EQ(SniffingResult::kMaybe,
              CrossOriginReadBlocking::SniffForJSON(almost_json))
        << almost_json;
  }

  EXPECT_EQ(SniffingResult::kNo,
            CrossOriginReadBlocking::SniffForJSON(non_json_data0));
  EXPECT_EQ(SniffingResult::kNo,
            CrossOriginReadBlocking::SniffForJSON(non_json_data1));

  EXPECT_EQ(SniffingResult::kYes,
            CrossOriginReadBlocking::SniffForJSON(R"({"" : 1})"))
      << "Empty strings are accepted";
  EXPECT_EQ(SniffingResult::kNo,
            CrossOriginReadBlocking::SniffForJSON(R"({'' : 1})"))
      << "Single quotes are not accepted";
  EXPECT_EQ(SniffingResult::kYes,
            CrossOriginReadBlocking::SniffForJSON("{\"\\\"\" : 1}"))
      << "Escaped quotes are recognized";
  EXPECT_EQ(SniffingResult::kYes,
            CrossOriginReadBlocking::SniffForJSON(R"({"\\\u000a" : 1})"))
      << "Escaped control characters are recognized";
  EXPECT_EQ(SniffingResult::kMaybe,
            CrossOriginReadBlocking::SniffForJSON(R"({"\\\u00)"))
      << "Incomplete escape results in maybe";
  EXPECT_EQ(SniffingResult::kMaybe,
            CrossOriginReadBlocking::SniffForJSON("{\"\\"))
      << "Incomplete escape results in maybe";
  EXPECT_EQ(SniffingResult::kMaybe,
            CrossOriginReadBlocking::SniffForJSON("{\"\\\""))
      << "Incomplete escape results in maybe";
  EXPECT_EQ(SniffingResult::kNo,
            CrossOriginReadBlocking::SniffForJSON("{\"\n\" : true}"))
      << "Unescaped control characters are rejected";
  EXPECT_EQ(SniffingResult::kNo, CrossOriginReadBlocking::SniffForJSON("{}"))
      << "Empty dictionary is not recognized (since it's valid JS too)";
  EXPECT_EQ(SniffingResult::kNo,
            CrossOriginReadBlocking::SniffForJSON("[true, false, 1, 2]"))
      << "Lists dictionary are not recognized (since they're valid JS too)";
  EXPECT_EQ(SniffingResult::kNo,
            CrossOriginReadBlocking::SniffForJSON(R"({":"})"))
      << "A colon character inside a string does not trigger a match";
}

TEST(CrossOriginReadBlockingTest, GetCanonicalMimeType) {
  std::vector<std::pair<const char*, MimeType>> tests = {
      // Basic tests for things in the original implementation:
      {"text/html", MimeType::kHtml},
      {"text/xml", MimeType::kXml},
      {"application/rss+xml", MimeType::kXml},
      {"application/xml", MimeType::kXml},
      {"application/json", MimeType::kJson},
      {"text/json", MimeType::kJson},
      {"text/plain", MimeType::kPlain},

      // Other mime types:
      {"application/foobar", MimeType::kOthers},

      // Regression tests for https://crbug.com/799155 (prefix/suffix matching):
      {"application/activity+json", MimeType::kJson},
      {"text/foobar+xml", MimeType::kXml},
      // No match without a '+' character:
      {"application/jsonfoobar", MimeType::kOthers},
      {"application/foobarjson", MimeType::kOthers},
      {"application/xmlfoobar", MimeType::kOthers},
      {"application/foobarxml", MimeType::kOthers},

      // Case-insensitive comparison:
      {"APPLICATION/JSON", MimeType::kJson},
      {"APPLICATION/ACTIVITY+JSON", MimeType::kJson},

      // Images are allowed cross-site, and SVG is an image, so we should
      // classify SVG as "other" instead of "xml" (even though it technically is
      // an xml document).
      {"image/svg+xml", MimeType::kOthers},

      // Javascript should not be blocked.
      {"application/javascript", MimeType::kOthers},
      {"application/jsonp", MimeType::kOthers},

      // TODO(lukasza): Remove in the future, once this MIME type is not used in
      // practice.  See also https://crbug.com/826756#c3
      {"application/json+protobuf", MimeType::kJson},
      {"APPLICATION/JSON+PROTOBUF", MimeType::kJson},

      // According to specs, these types are not XML or JSON.  See also:
      // - https://mimesniff.spec.whatwg.org/#xml-mime-type
      // - https://mimesniff.spec.whatwg.org/#json-mime-type
      {"text/x-json", MimeType::kOthers},
      {"text/json+blah", MimeType::kOthers},
      {"application/json+blah", MimeType::kOthers},
      {"text/xml+blah", MimeType::kOthers},
      {"application/xml+blah", MimeType::kOthers},
  };

  for (const auto& test : tests) {
    const char* input = test.first;  // e.g. "text/html"
    MimeType expected = test.second;
    MimeType actual = CrossOriginReadBlocking::GetCanonicalMimeType(input);
    EXPECT_EQ(expected, actual)
        << "when testing with the following input: " << input;
  }
}

}  // namespace network
