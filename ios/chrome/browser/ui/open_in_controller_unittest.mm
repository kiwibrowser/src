// Copyright 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import <Foundation/Foundation.h>

#include <memory>

#include "base/message_loop/message_loop.h"
#include "base/strings/sys_string_conversions.h"
#import "ios/chrome/browser/tabs/tab.h"
#import "ios/chrome/browser/ui/open_in_controller.h"
#import "ios/chrome/browser/ui/open_in_controller_testing.h"
#include "ios/web/public/test/test_web_thread.h"
#import "ios/web/web_state/ui/crw_web_controller.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "testing/platform_test.h"
#import "third_party/ocmock/OCMock/OCMock.h"
#include "third_party/ocmock/gtest_support.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {

class OpenInControllerTest : public PlatformTest {
 public:
  OpenInControllerTest() {
    io_thread_.reset(
        new web::TestWebThread(web::WebThread::IO, &message_loop_));
  }

  void TearDown() override { PlatformTest::TearDown(); }

  void SetUp() override {
    PlatformTest::SetUp();
    GURL documentURL = GURL("http://www.test.com/doc.pdf");
    parent_view_ = [[UIView alloc] init];
    id webController = [OCMockObject niceMockForClass:[CRWWebController class]];
    open_in_controller_ =
        [[OpenInController alloc] initWithRequestContext:nil
                                           webController:webController];
    [open_in_controller_ enableWithDocumentURL:documentURL
                             suggestedFilename:@"doc.pdf"];
  }
  // |TestURLFetcher| requires a |MessageLoop|.
  base::MessageLoopForUI message_loop_;
  // Add |io_thread_| to release |URLRequestContextGetter| in
  // |URLFetcher::Core|.
  std::unique_ptr<web::TestWebThread> io_thread_;
  // Creates a |TestURLFetcherFactory|, which automatically sets itself as
  // |URLFetcher|'s factory.
  net::TestURLFetcherFactory factory_;
  OpenInController* open_in_controller_;
  UIView* parent_view_;
};

TEST_F(OpenInControllerTest, DISABLED_TestDisplayOpenInMenu) {
  id documentController =
      [OCMockObject niceMockForClass:[UIDocumentInteractionController class]];
  [open_in_controller_ setDocumentInteractionController:documentController];
  [open_in_controller_ startDownload];
  [[[documentController expect] andReturnValue:[NSNumber numberWithBool:YES]]
      presentOpenInMenuFromRect:CGRectMake(0, 0, 0, 0)
                         inView:OCMOCK_ANY
                       animated:YES];

  net::TestURLFetcher* fetcher = factory_.GetFetcherByID(0);
  DCHECK(fetcher);
  DCHECK(fetcher->delegate());
  fetcher->set_response_code(200);
  // Set the response for the set URLFetcher to be a blank PDF.
  NSMutableData* pdfData = [NSMutableData data];
  UIGraphicsBeginPDFContextToData(pdfData, CGRectMake(0, 0, 100, 100), @{});
  UIGraphicsBeginPDFPage();
  UIGraphicsEndPDFContext();
  unsigned char* array = (unsigned char*)[pdfData bytes];
  fetcher->SetResponseString(std::string((char*)array, sizeof(pdfData)));
  fetcher->SetResponseFilePath(base::FilePath("path/to/file.pdf"));
  fetcher->delegate()->OnURLFetchComplete(fetcher);
  EXPECT_OCMOCK_VERIFY(documentController);
}

}  // namespace
