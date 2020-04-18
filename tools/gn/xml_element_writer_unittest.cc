// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "tools/gn/xml_element_writer.h"

#include <sstream>

#include "testing/gtest/include/gtest/gtest.h"

namespace {

class MockValueWriter {
 public:
  explicit MockValueWriter(const std::string& value) : value_(value) {}
  void operator()(std::ostream& out) const { out << value_; }

 private:
  std::string value_;
};

}  // namespace

TEST(XmlElementWriter, EmptyElement) {
  std::ostringstream out;
  { XmlElementWriter writer(out, "foo", XmlAttributes()); }
  EXPECT_EQ("<foo />\n", out.str());

  std::ostringstream out_attr;
  {
    XmlElementWriter writer(out_attr, "foo",
                            XmlAttributes("bar", "abc").add("baz", "123"));
  }
  EXPECT_EQ("<foo bar=\"abc\" baz=\"123\" />\n", out_attr.str());

  std::ostringstream out_indent;
  {
    XmlElementWriter writer(out_indent, "foo", XmlAttributes("bar", "baz"), 2);
  }
  EXPECT_EQ("  <foo bar=\"baz\" />\n", out_indent.str());

  std::ostringstream out_writer;
  {
    XmlElementWriter writer(out_writer, "foo", "bar", MockValueWriter("baz"),
                            2);
  }
  EXPECT_EQ("  <foo bar=\"baz\" />\n", out_writer.str());
}

TEST(XmlElementWriter, ElementWithText) {
  std::ostringstream out;
  {
    XmlElementWriter writer(out, "foo", XmlAttributes("bar", "baz"));
    writer.Text("Hello world!");
  }
  EXPECT_EQ("<foo bar=\"baz\">Hello world!</foo>\n", out.str());
}

TEST(XmlElementWriter, SubElements) {
  std::ostringstream out;
  {
    XmlElementWriter writer(out, "root", XmlAttributes("aaa", "000"));
    writer.SubElement("foo", XmlAttributes());
    writer.SubElement("bar", XmlAttributes("bbb", "111"))->Text("hello");
    writer.SubElement("baz", "ccc", MockValueWriter("222"))
        ->SubElement("grandchild");
  }
  std::string expected =
      "<root aaa=\"000\">\n"
      "  <foo />\n"
      "  <bar bbb=\"111\">hello</bar>\n"
      "  <baz ccc=\"222\">\n"
      "    <grandchild />\n"
      "  </baz>\n"
      "</root>\n";
  EXPECT_EQ(expected, out.str());
}

TEST(XmlElementWriter, StartContent) {
  std::ostringstream out;
  {
    XmlElementWriter writer(out, "foo", XmlAttributes("bar", "baz"));
    writer.StartContent(false) << "Hello world!";
  }
  EXPECT_EQ("<foo bar=\"baz\">Hello world!</foo>\n", out.str());
}

TEST(XmlElementWriter, TestXmlEscape) {
  std::string input = "\r \n \t & < > \"";
  std::string output = XmlEscape(input);
  std::string expected = "&#13; &#10; &#9; &amp; &lt; &gt; &quot;";
  EXPECT_EQ(expected, output);
}
