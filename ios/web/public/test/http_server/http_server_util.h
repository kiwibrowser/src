// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_WEB_PUBLIC_TEST_HTTP_SERVER_HTTP_SERVER_UTIL_H_
#define IOS_WEB_PUBLIC_TEST_HTTP_SERVER_HTTP_SERVER_UTIL_H_

#include <map>

#include "url/gurl.h"

namespace web {

class ResponseProvider;

namespace test {

// Sets up a web::test::HttpServer with a simple HtmlResponseProvider. The
// HtmlResponseProvider will use the |responses| map to resolve URLs.
void SetUpSimpleHttpServer(const std::map<GURL, std::string>& responses);

// Sets up a web::test::HttpServer with a simple HtmlResponseProvider. The
// HtmlResponseProvider will use the |responses| map to resolve URLs. The value
// of |responses| is the cookie and response body pair where the first string is
// the cookie and the second string is the response body. Set the cookie string
// as empty string if cookie is not needed in the response headers.
void SetUpSimpleHttpServerWithSetCookies(
    const std::map<GURL, std::pair<std::string, std::string>>& responses);

// Sets up a web::test::HttpServer with a FileBasedResponseProvider. The
// server will try to resolve URLs as file paths relative to the application
// bundle path. web::test::MakeUrl should be used to rewrite URLs before doing
// a request.
void SetUpFileBasedHttpServer();

// Sets up a web::test::HttpServer with a single custom provider.
// Takes ownership of the provider.
void SetUpHttpServer(std::unique_ptr<web::ResponseProvider> provider);

// Adds a single custom provider.
// Takes ownership of the provider.
void AddResponseProvider(std::unique_ptr<web::ResponseProvider> provider);
}  // namespace test
}  // namespace web

#endif  // IOS_WEB_PUBLIC_TEST_HTTP_SERVER_HTTP_SERVER_UTIL_H_
