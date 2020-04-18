/*
 * Copyright (c) 2013, Opera Software ASA. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of Opera Software ASA nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/dom/dom_implementation.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

TEST(DOMImplementationTest, TextMIMEType) {
  EXPECT_TRUE(DOMImplementation::IsTextMIMEType("text/plain"));
  EXPECT_TRUE(DOMImplementation::IsTextMIMEType("text/javascript"));
  EXPECT_TRUE(DOMImplementation::IsTextMIMEType("TEXT/JavaScript"));
  EXPECT_TRUE(DOMImplementation::IsTextMIMEType("application/json"));
  EXPECT_TRUE(DOMImplementation::IsTextMIMEType("application/jSON"));
  EXPECT_TRUE(DOMImplementation::IsTextMIMEType("application/json;foo=2"));
  EXPECT_TRUE(DOMImplementation::IsTextMIMEType("application/json  "));
  EXPECT_TRUE(DOMImplementation::IsTextMIMEType("application/+json"));
  EXPECT_TRUE(DOMImplementation::IsTextMIMEType(
      "application/x-javascript-like+json;a=2;c=4"));
  EXPECT_TRUE(DOMImplementation::IsTextMIMEType("application/javascript"));
  EXPECT_TRUE(DOMImplementation::IsTextMIMEType("Application/Javascript"));
  EXPECT_TRUE(
      DOMImplementation::IsTextMIMEType("application/x-custom+json;b=3"));
  EXPECT_TRUE(DOMImplementation::IsTextMIMEType("application/x-custom+json"));
  // Outside of RFC-2045 grammar, but robustly accept/allow.
  EXPECT_TRUE(DOMImplementation::IsTextMIMEType("application/x-what+json;"));
  EXPECT_TRUE(DOMImplementation::IsTextMIMEType("application/json;"));
  EXPECT_TRUE(DOMImplementation::IsTextMIMEType("application/json "));

  EXPECT_FALSE(
      DOMImplementation::IsTextMIMEType("application/x-custom;a=a+json"));
  EXPECT_FALSE(
      DOMImplementation::IsTextMIMEType("application/x-custom;a=a+json ;"));
  EXPECT_FALSE(
      DOMImplementation::IsTextMIMEType("application/x-custom+jsonsoup"));
  EXPECT_FALSE(
      DOMImplementation::IsTextMIMEType("application/x-custom+jsonsoup  "));
  EXPECT_FALSE(DOMImplementation::IsTextMIMEType("text/html"));
  EXPECT_FALSE(DOMImplementation::IsTextMIMEType("text/xml"));
  EXPECT_FALSE(DOMImplementation::IsTextMIMEType("text/xsl"));
}

TEST(DOMImplementationTest, TextXMLType) {
  EXPECT_TRUE(DOMImplementation::IsXMLMIMEType("text/xml"));
  EXPECT_TRUE(DOMImplementation::IsXMLMIMEType("Text/xml"));
  EXPECT_TRUE(DOMImplementation::IsXMLMIMEType("tEXt/XML"));
  EXPECT_TRUE(DOMImplementation::IsXMLMIMEType("application/xml"));
  EXPECT_TRUE(DOMImplementation::IsXMLMIMEType("application/XML"));
  EXPECT_TRUE(DOMImplementation::IsXMLMIMEType("application/x-tra+xML"));
  EXPECT_TRUE(DOMImplementation::IsXMLMIMEType("application/xslt+xml"));
  EXPECT_TRUE(DOMImplementation::IsXMLMIMEType("application/rdf+Xml"));
  EXPECT_TRUE(DOMImplementation::IsXMLMIMEType("image/svg+xml"));
  EXPECT_TRUE(DOMImplementation::IsXMLMIMEType("text/xsl"));
  EXPECT_TRUE(DOMImplementation::IsXMLMIMEType("text/XSL"));
  EXPECT_TRUE(DOMImplementation::IsXMLMIMEType("application/x+xml"));

  EXPECT_FALSE(
      DOMImplementation::IsXMLMIMEType("application/x-custom;a=a+xml"));
  EXPECT_FALSE(
      DOMImplementation::IsXMLMIMEType("application/x-custom;a=a+xml ;"));
  EXPECT_FALSE(DOMImplementation::IsXMLMIMEType("application/x-custom+xml2"));
  EXPECT_FALSE(DOMImplementation::IsXMLMIMEType("application/x-custom+xml2  "));
  EXPECT_FALSE(DOMImplementation::IsXMLMIMEType("application/x-custom+exml"));
  EXPECT_FALSE(DOMImplementation::IsXMLMIMEType("text/html"));
  EXPECT_FALSE(DOMImplementation::IsXMLMIMEType("application/xml;"));
  EXPECT_FALSE(DOMImplementation::IsXMLMIMEType("application/xml "));
  EXPECT_FALSE(DOMImplementation::IsXMLMIMEType("application/x-what+xml;"));
  EXPECT_FALSE(DOMImplementation::IsXMLMIMEType("application/x-tra+xML;a=2"));
  EXPECT_FALSE(DOMImplementation::IsXMLMIMEType("application/+xML"));
  EXPECT_FALSE(DOMImplementation::IsXMLMIMEType("application/+xml"));
}

}  // namespace blink
