// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/script/modulator.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/renderer/platform/testing/testing_platform_support.h"

namespace blink {

TEST(ModulatorTest, resolveModuleSpecifier) {
  // Taken from examples listed in
  // https://html.spec.whatwg.org/multipage/webappapis.html#resolve-a-module-specifier

  // "The following are valid module specifiers according to the above
  // algorithm:"
  EXPECT_TRUE(Modulator::ResolveModuleSpecifier("https://example.com/apples.js",
                                                NullURL())
                  .IsValid());

  KURL resolved = Modulator::ResolveModuleSpecifier(
      "http:example.com\\pears.mjs", NullURL());
  EXPECT_TRUE(resolved.IsValid());
  EXPECT_EQ("http://example.com/pears.mjs", resolved.GetString());

  KURL base_url(NullURL(), "https://example.com");
  EXPECT_TRUE(
      Modulator::ResolveModuleSpecifier("//example.com/", base_url).IsValid());
  EXPECT_TRUE(
      Modulator::ResolveModuleSpecifier("./strawberries.js.cgi", base_url)
          .IsValid());
  EXPECT_TRUE(
      Modulator::ResolveModuleSpecifier("../lychees", base_url).IsValid());
  EXPECT_TRUE(
      Modulator::ResolveModuleSpecifier("/limes.jsx", base_url).IsValid());
  EXPECT_TRUE(Modulator::ResolveModuleSpecifier(
                  "data:text/javascript,export default 'grapes';", NullURL())
                  .IsValid());
  EXPECT_TRUE(
      Modulator::ResolveModuleSpecifier(
          "blob:https://whatwg.org/d0360e2f-caee-469f-9a2f-87d5b0456f6f",
          KURL())
          .IsValid());

  // "The following are valid module specifiers according to the above
  // algorithm, but will invariably cause failures when they are fetched:"
  EXPECT_TRUE(Modulator::ResolveModuleSpecifier(
                  "javascript:export default 'artichokes';", NullURL())
                  .IsValid());
  EXPECT_TRUE(Modulator::ResolveModuleSpecifier(
                  "data:text/plain,export default 'kale';", NullURL())
                  .IsValid());
  EXPECT_TRUE(
      Modulator::ResolveModuleSpecifier("about:legumes", NullURL()).IsValid());
  EXPECT_TRUE(
      Modulator::ResolveModuleSpecifier("wss://example.com/celery", NullURL())
          .IsValid());

  // "The following are not valid module specifiers according to the above
  // algorithm:"
  EXPECT_FALSE(
      Modulator::ResolveModuleSpecifier("https://f:b/c", NullURL()).IsValid());
  EXPECT_FALSE(
      Modulator::ResolveModuleSpecifier("pumpkins.js", NullURL()).IsValid());

  // Unprefixed module specifiers should still fail w/ a valid baseURL.
  EXPECT_FALSE(
      Modulator::ResolveModuleSpecifier("avocados.js", base_url).IsValid());
}

}  // namespace blink
