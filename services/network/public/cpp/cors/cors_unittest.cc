// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/network/public/cpp/cors/cors.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace network {

namespace {

using CORSTest = testing::Test;

TEST_F(CORSTest, CheckAccessDetectsInvalidResponse) {
  base::Optional<mojom::CORSError> error = cors::CheckAccess(
      GURL(), 0 /* response_status_code */,
      base::nullopt /* allow_origin_header */,
      base::nullopt /* allow_credentials_header */,
      network::mojom::FetchCredentialsMode::kOmit, url::Origin());
  ASSERT_TRUE(error);
  EXPECT_EQ(mojom::CORSError::kInvalidResponse, *error);
}

// Tests if cors::CheckAccess detects kWildcardOriginNotAllowed error correctly.
TEST_F(CORSTest, CheckAccessDetectsWildcardOriginNotAllowed) {
  const GURL response_url("http://example.com/data");
  const url::Origin origin = url::Origin::Create(GURL("http://google.com"));
  const int response_status_code = 200;
  const std::string allow_all_header("*");

  // Access-Control-Allow-Origin '*' works.
  base::Optional<mojom::CORSError> error1 =
      cors::CheckAccess(response_url, response_status_code,
                        allow_all_header /* allow_origin_header */,
                        base::nullopt /* allow_credentials_header */,
                        network::mojom::FetchCredentialsMode::kOmit, origin);
  EXPECT_FALSE(error1);

  // Access-Control-Allow-Origin '*' should not be allowed if credentials mode
  // is kInclude.
  base::Optional<mojom::CORSError> error2 =
      cors::CheckAccess(response_url, response_status_code,
                        allow_all_header /* allow_origin_header */,
                        base::nullopt /* allow_credentials_header */,
                        network::mojom::FetchCredentialsMode::kInclude, origin);
  ASSERT_TRUE(error2);
  EXPECT_EQ(mojom::CORSError::kWildcardOriginNotAllowed, *error2);
}

// Tests if cors::CheckAccess detects kMissingAllowOriginHeader error correctly.
TEST_F(CORSTest, CheckAccessDetectsMissingAllowOriginHeader) {
  const GURL response_url("http://example.com/data");
  const url::Origin origin = url::Origin::Create(GURL("http://google.com"));
  const int response_status_code = 200;

  // Access-Control-Allow-Origin is missed.
  base::Optional<mojom::CORSError> error =
      cors::CheckAccess(response_url, response_status_code,
                        base::nullopt /* allow_origin_header */,
                        base::nullopt /* allow_credentials_header */,
                        network::mojom::FetchCredentialsMode::kOmit, origin);
  ASSERT_TRUE(error);
  EXPECT_EQ(mojom::CORSError::kMissingAllowOriginHeader, *error);
}

// Tests if cors::CheckAccess detects kMultipleAllowOriginValues error
// correctly.
TEST_F(CORSTest, CheckAccessDetectsMultipleAllowOriginValues) {
  const GURL response_url("http://example.com/data");
  const url::Origin origin = url::Origin::Create(GURL("http://google.com"));
  const int response_status_code = 200;

  const std::string space_separated_multiple_origins(
      "http://example.com http://another.example.com");
  base::Optional<mojom::CORSError> error1 = cors::CheckAccess(
      response_url, response_status_code,
      space_separated_multiple_origins /* allow_origin_header */,
      base::nullopt /* allow_credentials_header */,
      network::mojom::FetchCredentialsMode::kOmit, origin);
  ASSERT_TRUE(error1);
  EXPECT_EQ(mojom::CORSError::kMultipleAllowOriginValues, *error1);

  const std::string comma_separated_multiple_origins(
      "http://example.com,http://another.example.com");
  base::Optional<mojom::CORSError> error2 = cors::CheckAccess(
      response_url, response_status_code,
      comma_separated_multiple_origins /* allow_origin_header */,
      base::nullopt /* allow_credentials_header */,
      network::mojom::FetchCredentialsMode::kOmit, origin);
  ASSERT_TRUE(error2);
  EXPECT_EQ(mojom::CORSError::kMultipleAllowOriginValues, *error2);
}

// Tests if cors::CheckAccess detects kInvalidAllowOriginValue error correctly.
TEST_F(CORSTest, CheckAccessDetectsInvalidAllowOriginValue) {
  const GURL response_url("http://example.com/data");
  const url::Origin origin = url::Origin::Create(GURL("http://google.com"));
  const int response_status_code = 200;

  base::Optional<mojom::CORSError> error =
      cors::CheckAccess(response_url, response_status_code,
                        std::string("invalid.origin") /* allow_origin_header */,
                        base::nullopt /* allow_credentials_header */,
                        network::mojom::FetchCredentialsMode::kOmit, origin);
  ASSERT_TRUE(error);
  EXPECT_EQ(mojom::CORSError::kInvalidAllowOriginValue, *error);
}

// Tests if cors::CheckAccess detects kAllowOriginMismatch error correctly.
TEST_F(CORSTest, CheckAccessDetectsAllowOriginMismatch) {
  const GURL response_url("http://example.com/data");
  const url::Origin origin = url::Origin::Create(GURL("http://google.com"));
  const int response_status_code = 200;

  base::Optional<mojom::CORSError> error1 =
      cors::CheckAccess(response_url, response_status_code,
                        origin.Serialize() /* allow_origin_header */,
                        base::nullopt /* allow_credentials_header */,
                        network::mojom::FetchCredentialsMode::kOmit, origin);
  ASSERT_FALSE(error1);

  base::Optional<mojom::CORSError> error2 = cors::CheckAccess(
      response_url, response_status_code,
      std::string("http://not.google.com") /* allow_origin_header */,
      base::nullopt /* allow_credentials_header */,
      network::mojom::FetchCredentialsMode::kOmit, origin);
  ASSERT_TRUE(error2);
  EXPECT_EQ(mojom::CORSError::kAllowOriginMismatch, *error2);

  // Allow "null" value to match serialized unique origins.
  const std::string null_string("null");
  const url::Origin null_origin;
  EXPECT_EQ(null_string, null_origin.Serialize());

  base::Optional<mojom::CORSError> error3 = cors::CheckAccess(
      response_url, response_status_code, null_string /* allow_origin_header */,
      base::nullopt /* allow_credentials_header */,
      network::mojom::FetchCredentialsMode::kOmit, null_origin);
  EXPECT_FALSE(error3);
}

// Tests if cors::CheckAccess detects kInvalidAllowCredentials error correctly.
TEST_F(CORSTest, CheckAccessDetectsInvalidAllowCredential) {
  const GURL response_url("http://example.com/data");
  const url::Origin origin = url::Origin::Create(GURL("http://google.com"));
  const int response_status_code = 200;

  base::Optional<mojom::CORSError> error1 =
      cors::CheckAccess(response_url, response_status_code,
                        origin.Serialize() /* allow_origin_header */,
                        std::string("true") /* allow_credentials_header */,
                        network::mojom::FetchCredentialsMode::kInclude, origin);
  ASSERT_FALSE(error1);

  base::Optional<mojom::CORSError> error2 =
      cors::CheckAccess(response_url, response_status_code,
                        origin.Serialize() /* allow_origin_header */,
                        base::nullopt /* allow_credentials_header */,
                        network::mojom::FetchCredentialsMode::kInclude, origin);
  ASSERT_TRUE(error2);
  EXPECT_EQ(mojom::CORSError::kInvalidAllowCredentials, *error2);
}

// Tests if cors::CheckRedirectLocation detects kRedirectDisallowedScheme and
// kRedirectContainsCredentials errors correctly.
TEST_F(CORSTest, CheckRedirectLocationDetectsErrors) {
  // Following URLs should pass.
  EXPECT_FALSE(cors::CheckRedirectLocation(GURL("http://example.com/"), false));
  EXPECT_FALSE(
      cors::CheckRedirectLocation(GURL("https://example.com/"), false));
  EXPECT_FALSE(cors::CheckRedirectLocation(GURL("data:,Hello"), false));
  EXPECT_FALSE(
      cors::CheckRedirectLocation(GURL("file:///not_allow_scheme"), true));

  // Following URLs should result in kRedirectDisallowedScheme.
  base::Optional<mojom::CORSError> error1 =
      cors::CheckRedirectLocation(GURL("file:///not_allow_scheme"), false);
  ASSERT_TRUE(error1);
  EXPECT_EQ(mojom::CORSError::kRedirectDisallowedScheme, *error1);

  // Following checks should result in the kRedirectContainsCredentials error.
  base::Optional<mojom::CORSError> error2 = cors::CheckRedirectLocation(
      GURL("http://yukari:tamura@example.com/"), false);
  ASSERT_TRUE(error2);
  EXPECT_EQ(mojom::CORSError::kRedirectContainsCredentials, *error2);

  base::Optional<mojom::CORSError> error3 = cors::CheckRedirectLocation(
      GURL("http://yukari:tamura@example.com/"), true);
  ASSERT_TRUE(error3);
  EXPECT_EQ(mojom::CORSError::kRedirectContainsCredentials, *error3);

  base::Optional<mojom::CORSError> error4 =
      cors::CheckRedirectLocation(GURL("http://tamura@example.com/"), true);
  ASSERT_TRUE(error4);
  EXPECT_EQ(mojom::CORSError::kRedirectContainsCredentials, *error4);

  base::Optional<mojom::CORSError> error5 =
      cors::CheckRedirectLocation(GURL("http://yukari:@example.com/"), true);
  ASSERT_TRUE(error5);
  EXPECT_EQ(mojom::CORSError::kRedirectContainsCredentials, *error5);
}

TEST_F(CORSTest, CheckPreflightDetectsErrors) {
  EXPECT_FALSE(cors::CheckPreflight(200));
  EXPECT_FALSE(cors::CheckPreflight(299));

  base::Optional<mojom::CORSError> error1 = cors::CheckPreflight(300);
  ASSERT_TRUE(error1);
  EXPECT_EQ(mojom::CORSError::kPreflightInvalidStatus, *error1);

  EXPECT_FALSE(cors::CheckExternalPreflight(std::string("true")));

  base::Optional<mojom::CORSError> error2 =
      cors::CheckExternalPreflight(base::nullopt);
  ASSERT_TRUE(error2);
  EXPECT_EQ(mojom::CORSError::kPreflightMissingAllowExternal, *error2);

  base::Optional<mojom::CORSError> error3 =
      cors::CheckExternalPreflight(std::string("TRUE"));
  ASSERT_TRUE(error3);
  EXPECT_EQ(mojom::CORSError::kPreflightInvalidAllowExternal, *error3);
}

TEST_F(CORSTest, CheckCORSSafelist) {
  // Method check should be case-insensitive.
  EXPECT_TRUE(cors::IsCORSSafelistedMethod("get"));
  EXPECT_TRUE(cors::IsCORSSafelistedMethod("Get"));
  EXPECT_TRUE(cors::IsCORSSafelistedMethod("GET"));
  EXPECT_TRUE(cors::IsCORSSafelistedMethod("HEAD"));
  EXPECT_TRUE(cors::IsCORSSafelistedMethod("POST"));
  EXPECT_FALSE(cors::IsCORSSafelistedMethod("OPTIONS"));

  // Content-Type check should be case-insensitive, and should ignore spaces and
  // parameters such as charset after a semicolon.
  EXPECT_TRUE(
      cors::IsCORSSafelistedContentType("application/x-www-form-urlencoded"));
  EXPECT_TRUE(cors::IsCORSSafelistedContentType("multipart/form-data"));
  EXPECT_TRUE(cors::IsCORSSafelistedContentType("text/plain"));
  EXPECT_TRUE(cors::IsCORSSafelistedContentType("TEXT/PLAIN"));
  EXPECT_TRUE(cors::IsCORSSafelistedContentType("text/plain;charset=utf-8"));
  EXPECT_TRUE(cors::IsCORSSafelistedContentType(" text/plain ;charset=utf-8"));
  EXPECT_FALSE(cors::IsCORSSafelistedContentType("text/html"));

  // Header check should be case-insensitive. Value must be considered only for
  // Content-Type.
  EXPECT_TRUE(cors::IsCORSSafelistedHeader("accept", "text/html"));
  EXPECT_TRUE(cors::IsCORSSafelistedHeader("Accept-Language", "en"));
  EXPECT_TRUE(cors::IsCORSSafelistedHeader("Content-Language", "ja"));
  EXPECT_TRUE(cors::IsCORSSafelistedHeader(
      "x-devtools-emulate-network-conditions-client-id", ""));
  EXPECT_TRUE(cors::IsCORSSafelistedHeader("SAVE-DATA", "on"));
  EXPECT_TRUE(cors::IsCORSSafelistedHeader("Intervention", ""));
  EXPECT_FALSE(cors::IsCORSSafelistedHeader("Cache-Control", ""));
  EXPECT_TRUE(cors::IsCORSSafelistedHeader("Content-Type", "text/plain"));
  EXPECT_FALSE(cors::IsCORSSafelistedHeader("Content-Type", "image/png"));
}

TEST_F(CORSTest, CheckCORSClientHintsSafelist) {
  EXPECT_FALSE(cors::IsCORSSafelistedHeader("device-memory", ""));
  EXPECT_FALSE(cors::IsCORSSafelistedHeader("device-memory", "abc"));
  EXPECT_TRUE(cors::IsCORSSafelistedHeader("device-memory", "1.25"));
  EXPECT_TRUE(cors::IsCORSSafelistedHeader("DEVICE-memory", "1.25"));
  EXPECT_FALSE(cors::IsCORSSafelistedHeader("device-memory", "1.25-2.5"));
  EXPECT_FALSE(cors::IsCORSSafelistedHeader("device-memory", "-1.25"));
  EXPECT_FALSE(cors::IsCORSSafelistedHeader("device-memory", "1e2"));
  EXPECT_FALSE(cors::IsCORSSafelistedHeader("device-memory", "inf"));
  EXPECT_FALSE(cors::IsCORSSafelistedHeader("device-memory", "-2.3"));
  EXPECT_FALSE(cors::IsCORSSafelistedHeader("device-memory", "NaN"));
  EXPECT_FALSE(cors::IsCORSSafelistedHeader("DEVICE-memory", "1.25.3"));
  EXPECT_FALSE(cors::IsCORSSafelistedHeader("DEVICE-memory", "1."));
  EXPECT_FALSE(cors::IsCORSSafelistedHeader("DEVICE-memory", ".1"));
  EXPECT_FALSE(cors::IsCORSSafelistedHeader("DEVICE-memory", "."));
  EXPECT_TRUE(cors::IsCORSSafelistedHeader("DEVICE-memory", "1"));

  EXPECT_FALSE(cors::IsCORSSafelistedHeader("dpr", ""));
  EXPECT_FALSE(cors::IsCORSSafelistedHeader("dpr", "abc"));
  EXPECT_TRUE(cors::IsCORSSafelistedHeader("dpr", "1.25"));
  EXPECT_TRUE(cors::IsCORSSafelistedHeader("Dpr", "1.25"));
  EXPECT_FALSE(cors::IsCORSSafelistedHeader("dpr", "1.25-2.5"));
  EXPECT_FALSE(cors::IsCORSSafelistedHeader("dpr", "-1.25"));
  EXPECT_FALSE(cors::IsCORSSafelistedHeader("dpr", "1e2"));
  EXPECT_FALSE(cors::IsCORSSafelistedHeader("dpr", "inf"));
  EXPECT_FALSE(cors::IsCORSSafelistedHeader("dpr", "-2.3"));
  EXPECT_FALSE(cors::IsCORSSafelistedHeader("dpr", "NaN"));
  EXPECT_FALSE(cors::IsCORSSafelistedHeader("dpr", "1.25.3"));
  EXPECT_FALSE(cors::IsCORSSafelistedHeader("dpr", "1."));
  EXPECT_FALSE(cors::IsCORSSafelistedHeader("dpr", ".1"));
  EXPECT_FALSE(cors::IsCORSSafelistedHeader("dpr", "."));
  EXPECT_TRUE(cors::IsCORSSafelistedHeader("dpr", "1"));

  EXPECT_FALSE(cors::IsCORSSafelistedHeader("width", ""));
  EXPECT_FALSE(cors::IsCORSSafelistedHeader("width", "abc"));
  EXPECT_TRUE(cors::IsCORSSafelistedHeader("width", "125"));
  EXPECT_TRUE(cors::IsCORSSafelistedHeader("width", "1"));
  EXPECT_TRUE(cors::IsCORSSafelistedHeader("WIDTH", "125"));
  EXPECT_FALSE(cors::IsCORSSafelistedHeader("width", "125.2"));
  EXPECT_FALSE(cors::IsCORSSafelistedHeader("width", "-125"));
  EXPECT_TRUE(cors::IsCORSSafelistedHeader("width", "2147483648"));

  EXPECT_FALSE(cors::IsCORSSafelistedHeader("viewport-width", ""));
  EXPECT_FALSE(cors::IsCORSSafelistedHeader("viewport-width", "abc"));
  EXPECT_TRUE(cors::IsCORSSafelistedHeader("viewport-width", "125"));
  EXPECT_TRUE(cors::IsCORSSafelistedHeader("viewport-width", "1"));
  EXPECT_TRUE(cors::IsCORSSafelistedHeader("viewport-Width", "125"));
  EXPECT_FALSE(cors::IsCORSSafelistedHeader("viewport-width", "125.2"));
  EXPECT_TRUE(cors::IsCORSSafelistedHeader("viewport-width", "2147483648"));
}

}  // namespace

}  // namespace network
