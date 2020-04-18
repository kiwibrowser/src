// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "testing/gtest/include/gtest/gtest.h"
#include "tools/gn/inherited_libraries.h"
#include "tools/gn/target.h"
#include "tools/gn/test_with_scope.h"

namespace {

// In these tests, Pair can't be used conveniently because the
// "const" won't be inferred and the types won't match. This helper makes the
// right type of pair with the const Target.
std::pair<const Target*, bool> Pair(const Target* t, bool b) {
  return std::pair<const Target*, bool>(t, b);
}

}  // namespace

TEST(InheritedLibraries, Unique) {
  TestWithScope setup;

  Target a(setup.settings(), Label(SourceDir("//foo/"), "a"));
  Target b(setup.settings(), Label(SourceDir("//foo/"), "b"));

  // Setup, add the two targets as private.
  InheritedLibraries libs;
  libs.Append(&a, false);
  libs.Append(&b, false);
  auto result = libs.GetOrderedAndPublicFlag();
  ASSERT_EQ(2u, result.size());
  EXPECT_EQ(Pair(&a, false), result[0]);
  EXPECT_EQ(Pair(&b, false), result[1]);

  // Add again as private, this should be a NOP.
  libs.Append(&a, false);
  libs.Append(&b, false);
  result = libs.GetOrderedAndPublicFlag();
  ASSERT_EQ(2u, result.size());
  EXPECT_EQ(Pair(&a, false), result[0]);
  EXPECT_EQ(Pair(&b, false), result[1]);

  // Add as public, this should make both public.
  libs.Append(&a, true);
  libs.Append(&b, true);
  result = libs.GetOrderedAndPublicFlag();
  ASSERT_EQ(2u, result.size());
  EXPECT_EQ(Pair(&a, true), result[0]);
  EXPECT_EQ(Pair(&b, true), result[1]);

  // Add again private, they should stay public.
  libs.Append(&a, false);
  libs.Append(&b, false);
  result = libs.GetOrderedAndPublicFlag();
  ASSERT_EQ(2u, result.size());
  EXPECT_EQ(Pair(&a, true), result[0]);
  EXPECT_EQ(Pair(&b, true), result[1]);
}

TEST(InheritedLibraries, AppendInherited) {
  TestWithScope setup;

  Target a(setup.settings(), Label(SourceDir("//foo/"), "a"));
  Target b(setup.settings(), Label(SourceDir("//foo/"), "b"));
  Target w(setup.settings(), Label(SourceDir("//foo/"), "w"));
  Target x(setup.settings(), Label(SourceDir("//foo/"), "x"));
  Target y(setup.settings(), Label(SourceDir("//foo/"), "y"));
  Target z(setup.settings(), Label(SourceDir("//foo/"), "z"));

  InheritedLibraries libs;
  libs.Append(&a, false);
  libs.Append(&b, false);

  // Appending these things with private inheritance should make them private,
  // no matter how they're listed in the appended class.
  InheritedLibraries append_private;
  append_private.Append(&a, true);
  append_private.Append(&b, false);
  append_private.Append(&w, true);
  append_private.Append(&x, false);
  libs.AppendInherited(append_private, false);

  auto result = libs.GetOrderedAndPublicFlag();
  ASSERT_EQ(4u, result.size());
  EXPECT_EQ(Pair(&a, false), result[0]);
  EXPECT_EQ(Pair(&b, false), result[1]);
  EXPECT_EQ(Pair(&w, false), result[2]);
  EXPECT_EQ(Pair(&x, false), result[3]);

  // Appending these things with public inheritance should convert them.
  InheritedLibraries append_public;
  append_public.Append(&a, true);
  append_public.Append(&b, false);
  append_public.Append(&y, true);
  append_public.Append(&z, false);
  libs.AppendInherited(append_public, true);

  result = libs.GetOrderedAndPublicFlag();
  ASSERT_EQ(6u, result.size());
  EXPECT_EQ(Pair(&a, true), result[0]);  // Converted to public.
  EXPECT_EQ(Pair(&b, false), result[1]);
  EXPECT_EQ(Pair(&w, false), result[2]);
  EXPECT_EQ(Pair(&x, false), result[3]);
  EXPECT_EQ(Pair(&y, true), result[4]);  // Appended as public.
  EXPECT_EQ(Pair(&z, false), result[5]);
}

TEST(InheritedLibraries, AppendPublicSharedLibraries) {
  TestWithScope setup;
  InheritedLibraries append;

  // Two source sets.
  Target set_pub(setup.settings(), Label(SourceDir("//foo/"), "set_pub"));
  set_pub.set_output_type(Target::SOURCE_SET);
  append.Append(&set_pub, true);
  Target set_priv(setup.settings(), Label(SourceDir("//foo/"), "set_priv"));
  set_priv.set_output_type(Target::SOURCE_SET);
  append.Append(&set_priv, false);

  // Two shared libraries.
  Target sh_pub(setup.settings(), Label(SourceDir("//foo/"), "sh_pub"));
  sh_pub.set_output_type(Target::SHARED_LIBRARY);
  append.Append(&sh_pub, true);
  Target sh_priv(setup.settings(), Label(SourceDir("//foo/"), "sh_priv"));
  sh_priv.set_output_type(Target::SHARED_LIBRARY);
  append.Append(&sh_priv, false);

  InheritedLibraries libs;
  libs.AppendPublicSharedLibraries(append, true);

  auto result = libs.GetOrderedAndPublicFlag();
  ASSERT_EQ(1u, result.size());
  EXPECT_EQ(Pair(&sh_pub, true), result[0]);
}
