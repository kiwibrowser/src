// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/loader/fetch/resource_loader_options.h"

#include "testing/gtest/include/gtest/gtest.h"
#include <type_traits>

namespace blink {

namespace {

TEST(ResourceLoaderOptionsTest, DeepCopy) {
  // Check that the fields of ResourceLoaderOptions are enums, except for
  // initiatorInfo and securityOrigin.
  static_assert(std::is_enum<DataBufferingPolicy>::value,
                "DataBufferingPolicy should be an enum");
  static_assert(std::is_enum<ContentSecurityPolicyDisposition>::value,
                "ContentSecurityPolicyDisposition should be an enum");
  static_assert(std::is_enum<RequestInitiatorContext>::value,
                "RequestInitiatorContext should be an enum");
  static_assert(std::is_enum<SynchronousPolicy>::value,
                "SynchronousPolicy should be an enum");
  static_assert(std::is_enum<CORSHandlingByResourceFetcher>::value,
                "CORSHandlingByResourceFetcher should be an enum");

  ResourceLoaderOptions original;
  scoped_refptr<const SecurityOrigin> security_origin =
      SecurityOrigin::CreateFromString("http://www.google.com");
  original.security_origin = security_origin;
  original.initiator_info.name = AtomicString("xmlhttprequest");

  CrossThreadResourceLoaderOptionsData copy_data =
      CrossThreadCopier<ResourceLoaderOptions>::Copy(original);
  ResourceLoaderOptions copy = copy_data;

  // Check that contents are correctly copied to |copyData|
  EXPECT_EQ(original.data_buffering_policy, copy_data.data_buffering_policy);
  EXPECT_EQ(original.content_security_policy_option,
            copy_data.content_security_policy_option);
  EXPECT_EQ(original.initiator_info.name, copy_data.initiator_info.name);
  EXPECT_EQ(original.initiator_info.position,
            copy_data.initiator_info.position);
  EXPECT_EQ(original.initiator_info.start_time,
            copy_data.initiator_info.start_time);
  EXPECT_EQ(original.request_initiator_context,
            copy_data.request_initiator_context);
  EXPECT_EQ(original.synchronous_policy, copy_data.synchronous_policy);
  EXPECT_EQ(original.cors_handling_by_resource_fetcher,
            copy_data.cors_handling_by_resource_fetcher);
  EXPECT_EQ(original.security_origin->Protocol(),
            copy_data.security_origin->Protocol());
  EXPECT_EQ(original.security_origin->Host(),
            copy_data.security_origin->Host());
  EXPECT_EQ(original.security_origin->Domain(),
            copy_data.security_origin->Domain());

  // Check that pointers are different between |original| and |copyData|
  EXPECT_NE(original.initiator_info.name.Impl(),
            copy_data.initiator_info.name.Impl());
  EXPECT_NE(original.security_origin.get(), copy_data.security_origin.get());
  EXPECT_NE(original.security_origin->Protocol().Impl(),
            copy_data.security_origin->Protocol().Impl());
  EXPECT_NE(original.security_origin->Host().Impl(),
            copy_data.security_origin->Host().Impl());
  EXPECT_NE(original.security_origin->Domain().Impl(),
            copy_data.security_origin->Domain().Impl());

  // Check that contents are correctly copied to |copy|
  EXPECT_EQ(original.data_buffering_policy, copy.data_buffering_policy);
  EXPECT_EQ(original.content_security_policy_option,
            copy.content_security_policy_option);
  EXPECT_EQ(original.initiator_info.name, copy.initiator_info.name);
  EXPECT_EQ(original.initiator_info.position, copy.initiator_info.position);
  EXPECT_EQ(original.initiator_info.start_time, copy.initiator_info.start_time);
  EXPECT_EQ(original.request_initiator_context, copy.request_initiator_context);
  EXPECT_EQ(original.synchronous_policy, copy.synchronous_policy);
  EXPECT_EQ(original.cors_handling_by_resource_fetcher,
            copy.cors_handling_by_resource_fetcher);
  EXPECT_EQ(original.security_origin->Protocol(),
            copy.security_origin->Protocol());
  EXPECT_EQ(original.security_origin->Host(), copy.security_origin->Host());
  EXPECT_EQ(original.security_origin->Domain(), copy.security_origin->Domain());

  // Check that pointers are different between |original| and |copy|
  // FIXME: When |original| and |copy| are in different threads, then
  // EXPECT_NE(original.initiatorInfo.name.impl(),
  //           copy.initiatorInfo.name.impl());
  // should pass. However, in the unit test here, these two pointers are the
  // same, because initiatorInfo.name is AtomicString.
  EXPECT_NE(original.security_origin.get(), copy.security_origin.get());
  EXPECT_NE(original.security_origin->Protocol().Impl(),
            copy.security_origin->Protocol().Impl());
  EXPECT_NE(original.security_origin->Host().Impl(),
            copy.security_origin->Host().Impl());
  EXPECT_NE(original.security_origin->Domain().Impl(),
            copy.security_origin->Domain().Impl());

  // FIXME: The checks for content equality/pointer inequality for
  // securityOrigin here is not complete (i.e. m_filePath is not checked). A
  // unit test for SecurityOrigin::isolatedCopy() that covers these checks
  // should be added.
}

}  // namespace

}  // namespace blink
