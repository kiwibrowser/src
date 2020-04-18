// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <ostream>
#include <vector>

#include "base/bind.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "tools/gn/config.h"
#include "tools/gn/header_checker.h"
#include "tools/gn/scheduler.h"
#include "tools/gn/target.h"
#include "tools/gn/test_with_scheduler.h"
#include "tools/gn/test_with_scope.h"

namespace {

class HeaderCheckerTest : public TestWithScheduler {
 public:
  HeaderCheckerTest()
      : a_(setup_.settings(), Label(SourceDir("//a/"), "a")),
        b_(setup_.settings(), Label(SourceDir("//b/"), "b")),
        c_(setup_.settings(), Label(SourceDir("//c/"), "c")),
        d_(setup_.settings(), Label(SourceDir("//d/"), "d")) {
    a_.set_output_type(Target::SOURCE_SET);
    b_.set_output_type(Target::SOURCE_SET);
    c_.set_output_type(Target::SOURCE_SET);
    d_.set_output_type(Target::SOURCE_SET);

    Err err;
    a_.SetToolchain(setup_.toolchain(), &err);
    b_.SetToolchain(setup_.toolchain(), &err);
    c_.SetToolchain(setup_.toolchain(), &err);
    d_.SetToolchain(setup_.toolchain(), &err);

    a_.public_deps().push_back(LabelTargetPair(&b_));
    b_.public_deps().push_back(LabelTargetPair(&c_));

    // Start with all public visibility.
    a_.visibility().SetPublic();
    b_.visibility().SetPublic();
    c_.visibility().SetPublic();
    d_.visibility().SetPublic();

    d_.OnResolved(&err);
    c_.OnResolved(&err);
    b_.OnResolved(&err);
    a_.OnResolved(&err);

    targets_.push_back(&a_);
    targets_.push_back(&b_);
    targets_.push_back(&c_);
    targets_.push_back(&d_);
  }

 protected:
  TestWithScope setup_;

  // Some headers that are automatically set up with a public dependency chain.
  // a -> b -> c. D is unconnected.
  Target a_;
  Target b_;
  Target c_;
  Target d_;

  std::vector<const Target*> targets_;
};

}  // namespace

void PrintTo(const SourceFile& source_file, ::std::ostream* os) {
  *os << source_file.value();
}

TEST_F(HeaderCheckerTest, IsDependencyOf) {
  scoped_refptr<HeaderChecker> checker(
      new HeaderChecker(setup_.build_settings(), targets_));

  // Add a target P ("private") that privately depends on C, and hook up the
  // chain so that A -> P -> C. A will depend on C via two different paths.
  Err err;
  Target p(setup_.settings(), Label(SourceDir("//p/"), "p"));
  p.set_output_type(Target::SOURCE_SET);
  p.SetToolchain(setup_.toolchain(), &err);
  EXPECT_FALSE(err.has_error());
  p.private_deps().push_back(LabelTargetPair(&c_));
  p.visibility().SetPublic();
  p.OnResolved(&err);

  a_.public_deps().push_back(LabelTargetPair(&p));

  // A does not depend on itself.
  bool is_permitted = false;
  HeaderChecker::Chain chain;
  EXPECT_FALSE(checker->IsDependencyOf(&a_, &a_, &chain, &is_permitted));

  // A depends publicly on B.
  chain.clear();
  is_permitted = false;
  EXPECT_TRUE(checker->IsDependencyOf(&b_, &a_, &chain, &is_permitted));
  ASSERT_EQ(2u, chain.size());
  EXPECT_EQ(HeaderChecker::ChainLink(&b_, true), chain[0]);
  EXPECT_EQ(HeaderChecker::ChainLink(&a_, true), chain[1]);
  EXPECT_TRUE(is_permitted);

  // A indirectly depends on C. The "public" dependency path through B should
  // be identified.
  chain.clear();
  is_permitted = false;
  EXPECT_TRUE(checker->IsDependencyOf(&c_, &a_, &chain, &is_permitted));
  ASSERT_EQ(3u, chain.size());
  EXPECT_EQ(HeaderChecker::ChainLink(&c_, true), chain[0]);
  EXPECT_EQ(HeaderChecker::ChainLink(&b_, true), chain[1]);
  EXPECT_EQ(HeaderChecker::ChainLink(&a_, true), chain[2]);
  EXPECT_TRUE(is_permitted);

  // C does not depend on A.
  chain.clear();
  is_permitted = false;
  EXPECT_FALSE(checker->IsDependencyOf(&a_, &c_, &chain, &is_permitted));
  EXPECT_TRUE(chain.empty());
  EXPECT_FALSE(is_permitted);

  // Remove the B -> C public dependency, leaving P's private dep on C the only
  // path from A to C. This should now be found.
  chain.clear();
  EXPECT_EQ(&c_, b_.public_deps()[0].ptr);  // Validate it's the right one.
  b_.public_deps().erase(b_.public_deps().begin());
  EXPECT_TRUE(checker->IsDependencyOf(&c_, &a_, &chain, &is_permitted));
  EXPECT_EQ(3u, chain.size());
  EXPECT_EQ(HeaderChecker::ChainLink(&c_, false), chain[0]);
  EXPECT_EQ(HeaderChecker::ChainLink(&p, true), chain[1]);
  EXPECT_EQ(HeaderChecker::ChainLink(&a_, true), chain[2]);
  EXPECT_FALSE(is_permitted);

  // P privately depends on C. That dependency should be OK since it's only
  // one hop.
  chain.clear();
  is_permitted = false;
  EXPECT_TRUE(checker->IsDependencyOf(&c_, &p, &chain, &is_permitted));
  ASSERT_EQ(2u, chain.size());
  EXPECT_EQ(HeaderChecker::ChainLink(&c_, false), chain[0]);
  EXPECT_EQ(HeaderChecker::ChainLink(&p, true), chain[1]);
  EXPECT_TRUE(is_permitted);
}

TEST_F(HeaderCheckerTest, CheckInclude) {
  InputFile input_file(SourceFile("//some_file.cc"));
  input_file.SetContents(std::string());
  LocationRange range;  // Dummy value.

  // Add a disconnected target d with a header to check that you have to have
  // to depend on a target listing a header.
  SourceFile d_header("//d_header.h");
  d_.sources().push_back(SourceFile(d_header));

  // Add a header on B and say everything in B is public.
  SourceFile b_public("//b_public.h");
  b_.sources().push_back(b_public);
  c_.set_all_headers_public(true);

  // Add a public and private header on C.
  SourceFile c_public("//c_public.h");
  SourceFile c_private("//c_private.h");
  c_.sources().push_back(c_private);
  c_.public_headers().push_back(c_public);
  c_.set_all_headers_public(false);

  // Create another toolchain.
  Settings other_settings(setup_.build_settings(), "other/");
  Toolchain other_toolchain(&other_settings,
                            Label(SourceDir("//toolchain/"), "other"));
  TestWithScope::SetupToolchain(&other_toolchain);
  other_settings.set_toolchain_label(other_toolchain.label());
  other_settings.set_default_toolchain_label(setup_.toolchain()->label());

  // Add a target in the other toolchain with a header in it that is not
  // connected to any targets in the main toolchain.
  Target otc(&other_settings,
             Label(SourceDir("//p/"), "otc", other_toolchain.label().dir(),
                   other_toolchain.label().name()));
  otc.set_output_type(Target::SOURCE_SET);
  Err err;
  EXPECT_TRUE(otc.SetToolchain(&other_toolchain, &err));
  otc.visibility().SetPublic();
  targets_.push_back(&otc);

  SourceFile otc_header("//otc_header.h");
  otc.sources().push_back(otc_header);
  EXPECT_TRUE(otc.OnResolved(&err));

  scoped_refptr<HeaderChecker> checker(
      new HeaderChecker(setup_.build_settings(), targets_));

  // A file in target A can't include a header from D because A has no
  // dependency on D.
  EXPECT_FALSE(checker->CheckInclude(&a_, input_file, d_header, range, &err));
  EXPECT_TRUE(err.has_error());

  // A can include the public header in B.
  err = Err();
  EXPECT_TRUE(checker->CheckInclude(&a_, input_file, b_public, range, &err));
  EXPECT_FALSE(err.has_error());

  // Check A depending on the public and private headers in C.
  err = Err();
  EXPECT_TRUE(checker->CheckInclude(&a_, input_file, c_public, range, &err));
  EXPECT_FALSE(err.has_error());
  EXPECT_FALSE(checker->CheckInclude(&a_, input_file, c_private, range, &err));
  EXPECT_TRUE(err.has_error());

  // A can depend on a random file unknown to the build.
  err = Err();
  EXPECT_TRUE(checker->CheckInclude(&a_, input_file, SourceFile("//random.h"),
                                    range, &err));
  EXPECT_FALSE(err.has_error());

  // A can depend on a file present only in another toolchain even with no
  // dependency path.
  err = Err();
  EXPECT_TRUE(checker->CheckInclude(&a_, input_file, otc_header, range, &err));
  EXPECT_FALSE(err.has_error());
}

// A public chain of dependencies should always be identified first, even if
// it is longer than a private one.
TEST_F(HeaderCheckerTest, PublicFirst) {
  // Now make a A -> Z -> D private dependency chain (one shorter than the
  // public one to get to D).
  Target z(setup_.settings(), Label(SourceDir("//a/"), "a"));
  z.set_output_type(Target::SOURCE_SET);
  Err err;
  EXPECT_TRUE(z.SetToolchain(setup_.toolchain(), &err));
  z.private_deps().push_back(LabelTargetPair(&d_));
  EXPECT_TRUE(z.OnResolved(&err));
  targets_.push_back(&z);

  a_.private_deps().push_back(LabelTargetPair(&z));

  // Check that D can be found from A, but since it's private, it will be
  // marked as not permitted.
  bool is_permitted = false;
  HeaderChecker::Chain chain;
  scoped_refptr<HeaderChecker> checker(
      new HeaderChecker(setup_.build_settings(), targets_));
  EXPECT_TRUE(checker->IsDependencyOf(&d_, &a_, &chain, &is_permitted));

  EXPECT_FALSE(is_permitted);
  ASSERT_EQ(3u, chain.size());
  EXPECT_EQ(HeaderChecker::ChainLink(&d_, false), chain[0]);
  EXPECT_EQ(HeaderChecker::ChainLink(&z, false), chain[1]);
  EXPECT_EQ(HeaderChecker::ChainLink(&a_, true), chain[2]);

  // Hook up D to the existing public A -> B -> C chain to make a long one, and
  // search for D again.
  c_.public_deps().push_back(LabelTargetPair(&d_));
  checker = new HeaderChecker(setup_.build_settings(), targets_);
  chain.clear();
  EXPECT_TRUE(checker->IsDependencyOf(&d_, &a_, &chain, &is_permitted));

  // This should have found the long public one.
  EXPECT_TRUE(is_permitted);
  ASSERT_EQ(4u, chain.size());
  EXPECT_EQ(HeaderChecker::ChainLink(&d_, true), chain[0]);
  EXPECT_EQ(HeaderChecker::ChainLink(&c_, true), chain[1]);
  EXPECT_EQ(HeaderChecker::ChainLink(&b_, true), chain[2]);
  EXPECT_EQ(HeaderChecker::ChainLink(&a_, true), chain[3]);
}

// Checks that the allow_circular_includes_from list works.
TEST_F(HeaderCheckerTest, CheckIncludeAllowCircular) {
  InputFile input_file(SourceFile("//some_file.cc"));
  input_file.SetContents(std::string());
  LocationRange range;  // Dummy value.

  // Add an include file to A.
  SourceFile a_public("//a_public.h");
  a_.sources().push_back(a_public);

  scoped_refptr<HeaderChecker> checker(
      new HeaderChecker(setup_.build_settings(), targets_));

  // A depends on B. So B normally can't include headers from A.
  Err err;
  EXPECT_FALSE(checker->CheckInclude(&b_, input_file, a_public, range, &err));
  EXPECT_TRUE(err.has_error());

  // Add an allow_circular_includes_from on A that lists B.
  a_.allow_circular_includes_from().insert(b_.label());

  // Now the include from B to A should be allowed.
  err = Err();
  EXPECT_TRUE(checker->CheckInclude(&b_, input_file, a_public, range, &err));
  EXPECT_FALSE(err.has_error());
}

TEST_F(HeaderCheckerTest, SourceFileForInclude) {
  using base::FilePath;
  const std::vector<SourceDir> kIncludeDirs = {
      SourceDir("/c/custom_include/"), SourceDir("//"), SourceDir("//subdir")};
  a_.sources().push_back(SourceFile("//lib/header1.h"));
  b_.sources().push_back(SourceFile("/c/custom_include/header2.h"));

  InputFile dummy_input_file(SourceFile("//some_file.cc"));
  dummy_input_file.SetContents(std::string());
  LocationRange dummy_range;

  scoped_refptr<HeaderChecker> checker(
      new HeaderChecker(setup_.build_settings(), targets_));
  {
    Err err;
    SourceFile source_file = checker->SourceFileForInclude(
        "lib/header1.h", kIncludeDirs, dummy_input_file, dummy_range, &err);
    EXPECT_FALSE(err.has_error());
    EXPECT_EQ(SourceFile("//lib/header1.h"), source_file);
  }

  {
    Err err;
    SourceFile source_file = checker->SourceFileForInclude(
        "header2.h", kIncludeDirs, dummy_input_file, dummy_range, &err);
    EXPECT_FALSE(err.has_error());
    EXPECT_EQ(SourceFile("/c/custom_include/header2.h"), source_file);
  }
}

TEST_F(HeaderCheckerTest, SourceFileForInclude_FileNotFound) {
  using base::FilePath;
  const char kFileContents[] = "Some dummy contents";
  const std::vector<SourceDir> kIncludeDirs = {SourceDir("//")};
  scoped_refptr<HeaderChecker> checker(
      new HeaderChecker(setup_.build_settings(), targets_));

  Err err;
  InputFile input_file(SourceFile("//input.cc"));
  input_file.SetContents(std::string(kFileContents));
  const int kLineNumber = 10;
  const int kColumnNumber = 16;
  const int kLength = 8;
  const int kByteNumber = 100;
  LocationRange range(
      Location(&input_file, kLineNumber, kColumnNumber, kByteNumber),
      Location(&input_file, kLineNumber, kColumnNumber + kLength, kByteNumber));

  SourceFile source_file = checker->SourceFileForInclude(
      "header.h", kIncludeDirs, input_file, range, &err);
  EXPECT_TRUE(source_file.is_null());
  EXPECT_FALSE(err.has_error());
}

TEST_F(HeaderCheckerTest, Friend) {
  // Note: we have a public dependency chain A -> B -> C set up already.
  InputFile input_file(SourceFile("//some_file.cc"));
  input_file.SetContents(std::string());
  LocationRange range;  // Dummy value.

  // Add a private header on C.
  SourceFile c_private("//c_private.h");
  c_.sources().push_back(c_private);
  c_.set_all_headers_public(false);

  // List A as a friend of C.
  Err err;
  c_.friends().push_back(
      LabelPattern::GetPattern(SourceDir("//"), Value(nullptr, "//a:*"), &err));
  ASSERT_FALSE(err.has_error());

  // Must be after setting everything up for it to find the files.
  scoped_refptr<HeaderChecker> checker(
      new HeaderChecker(setup_.build_settings(), targets_));

  // B should not be allowed to include C's private header.
  err = Err();
  EXPECT_FALSE(checker->CheckInclude(&b_, input_file, c_private, range, &err));
  EXPECT_TRUE(err.has_error());

  // A should be able to because of the friend declaration.
  err = Err();
  EXPECT_TRUE(checker->CheckInclude(&a_, input_file, c_private, range, &err));
  EXPECT_FALSE(err.has_error()) << err.message();
}
