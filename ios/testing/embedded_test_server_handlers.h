// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_TESTING_EMBEDDED_TEST_SERVER_HANDLERS_H_
#define IOS_TESTING_EMBEDDED_TEST_SERVER_HANDLERS_H_

#include <memory>

namespace net {
namespace test_server {
struct HttpRequest;
class HttpResponse;
}  // namespace test_server
}  // namespace net

namespace testing {

// Text returned from HandleForm handler.
extern const char kTestFormPage[];
// Field value for form returned from HandleForm handler.
extern const char kTestFormFieldValue[];

// Returns a page with iframe which uses URL from the query as src.
std::unique_ptr<net::test_server::HttpResponse> HandleIFrame(
    const net::test_server::HttpRequest& request);

// Returns a page with content of URL request query if |responds_with_content|
// is true. Closes the socket otherwise. Can be used to simulate the state where
// there is no internet connection.
std::unique_ptr<net::test_server::HttpResponse> HandleEchoQueryOrCloseSocket(
    const bool& responds_with_content,
    const net::test_server::HttpRequest& request);

// Returns a page with html form and kTestFormPage text. The form contains one
// text field with kTestFormFieldValue value.
std::unique_ptr<net::test_server::HttpResponse> HandleForm(
    const net::test_server::HttpRequest& request);

}  // namespace testing

#endif  // IOS_TESTING_EMBEDDED_TEST_SERVER_HANDLERS_H_
