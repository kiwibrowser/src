// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "jingle/notifier/listener/xml_element_util.h"

#include <memory>
#include <sstream>
#include <string>

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/libjingle_xmpp/xmllite/qname.h"
#include "third_party/libjingle_xmpp/xmllite/xmlelement.h"
#include "third_party/libjingle_xmpp/xmllite/xmlprinter.h"

namespace buzz {
class XmlElement;
}

namespace notifier {
namespace {

class XmlElementUtilTest : public testing::Test {};

TEST_F(XmlElementUtilTest, XmlElementToString) {
  const buzz::QName kQName("namespace", "element");
  const buzz::XmlElement kXmlElement(kQName, true);
  std::ostringstream expected_xml_stream;
  buzz::XmlPrinter::PrintXml(&expected_xml_stream, &kXmlElement);
  EXPECT_EQ(expected_xml_stream.str(), XmlElementToString(kXmlElement));
}

TEST_F(XmlElementUtilTest, MakeBoolXmlElement) {
  std::unique_ptr<buzz::XmlElement> foo_false(MakeBoolXmlElement("foo", false));
  EXPECT_EQ("<foo xmlns=\"\" bool=\"false\"/>", XmlElementToString(*foo_false));

  std::unique_ptr<buzz::XmlElement> bar_true(MakeBoolXmlElement("bar", true));
  EXPECT_EQ("<bar xmlns=\"\" bool=\"true\"/>", XmlElementToString(*bar_true));
}

TEST_F(XmlElementUtilTest, MakeIntXmlElement) {
  std::unique_ptr<buzz::XmlElement> int_xml_element(
      MakeIntXmlElement("foo", 35));
  EXPECT_EQ("<foo xmlns=\"\" int=\"35\"/>",
            XmlElementToString(*int_xml_element));
}

TEST_F(XmlElementUtilTest, MakeStringXmlElement) {
  std::unique_ptr<buzz::XmlElement> string_xml_element(
      MakeStringXmlElement("foo", "bar"));
  EXPECT_EQ("<foo xmlns=\"\" data=\"bar\"/>",
            XmlElementToString(*string_xml_element));
}

}  // namespace
}  // namespace notifier
