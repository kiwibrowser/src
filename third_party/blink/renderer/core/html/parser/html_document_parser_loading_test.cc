// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/html/parser/html_document_parser.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/html_names.h"
#include "third_party/blink/renderer/core/testing/sim/sim_request.h"
#include "third_party/blink/renderer/core/testing/sim/sim_test.h"
#include "third_party/blink/renderer/platform/testing/histogram_tester.h"
#include "third_party/blink/renderer/platform/testing/unit_test_helpers.h"

namespace blink {

using namespace HTMLNames;

class HTMLDocumentParserSimTest : public SimTest {
 protected:
  HTMLDocumentParserSimTest() {
    Document::SetThreadedParsingEnabledForTesting(true);
  }
  HistogramTester histogram_;
};

class HTMLDocumentParserLoadingTest : public HTMLDocumentParserSimTest,
                                      public testing::WithParamInterface<bool> {
 protected:
  HTMLDocumentParserLoadingTest() {
    Document::SetThreadedParsingEnabledForTesting(GetParam());
  }
};

INSTANTIATE_TEST_CASE_P(Threaded,
                        HTMLDocumentParserLoadingTest,
                        testing::Values(true));
INSTANTIATE_TEST_CASE_P(NotThreaded,
                        HTMLDocumentParserLoadingTest,
                        testing::Values(false));

TEST_P(HTMLDocumentParserLoadingTest,
       ShouldNotPauseParsingForExternalStylesheetsInHead) {
  SimRequest main_resource("https://example.com/test.html", "text/html");
  SimRequest css_head_resource("https://example.com/testHead.css", "text/css");

  LoadURL("https://example.com/test.html");

  main_resource.Complete(R"HTML(
    <!DOCTYPE html>
    <html><head>
    <link rel=stylesheet href=testHead.css>
    </head><body>
    <div id="bodyDiv"></div>
    </body></html>
  )HTML");

  test::RunPendingTasks();
  EXPECT_TRUE(GetDocument().getElementById("bodyDiv"));
  css_head_resource.Complete("");
  test::RunPendingTasks();
  EXPECT_TRUE(GetDocument().getElementById("bodyDiv"));
}

TEST_P(HTMLDocumentParserLoadingTest,
       ShouldNotPauseParsingForExternalStylesheetsImportedInHead) {
  SimRequest main_resource("https://example.com/test.html", "text/html");
  SimRequest css_head_resource("https://example.com/testHead.css", "text/css");

  LoadURL("https://example.com/test.html");

  main_resource.Complete(R"HTML(
    <!DOCTYPE html>
    <html><head>
    <style>
    @import 'testHead.css'
    </style>
    </head><body>
    <div id="bodyDiv"></div>
    </body></html>
  )HTML");

  test::RunPendingTasks();
  EXPECT_TRUE(GetDocument().getElementById("bodyDiv"));
  css_head_resource.Complete("");
  test::RunPendingTasks();
  EXPECT_TRUE(GetDocument().getElementById("bodyDiv"));
}

TEST_P(HTMLDocumentParserLoadingTest,
       ShouldPauseParsingForExternalStylesheetsInBody) {
  SimRequest main_resource("https://example.com/test.html", "text/html");
  SimRequest css_head_resource("https://example.com/testHead.css", "text/css");
  SimRequest css_body_resource("https://example.com/testBody.css", "text/css");

  LoadURL("https://example.com/test.html");

  main_resource.Complete(R"HTML(
    <!DOCTYPE html>
    <html><head>
    <link rel=stylesheet href=testHead.css>
    </head><body>
    <div id="before"></div>
    <link rel=stylesheet href=testBody.css>
    <div id="after"></div>
    </body></html>
  )HTML");

  test::RunPendingTasks();
  EXPECT_TRUE(GetDocument().getElementById("before"));
  EXPECT_FALSE(GetDocument().getElementById("after"));

  // Completing the head css shouldn't change anything
  css_head_resource.Complete("");
  test::RunPendingTasks();
  EXPECT_TRUE(GetDocument().getElementById("before"));
  EXPECT_FALSE(GetDocument().getElementById("after"));

  // Completing the body resource and pumping the tasks should continue parsing
  // and create the "after" div.
  css_body_resource.Complete("");
  test::RunPendingTasks();
  EXPECT_TRUE(GetDocument().getElementById("before"));
  EXPECT_TRUE(GetDocument().getElementById("after"));
}

TEST_P(HTMLDocumentParserLoadingTest,
       ShouldPauseParsingForExternalStylesheetsInBodyIncremental) {
  SimRequest main_resource("https://example.com/test.html", "text/html");
  SimRequest css_head_resource("https://example.com/testHead.css", "text/css");
  SimRequest css_body_resource1("https://example.com/testBody1.css",
                                "text/css");
  SimRequest css_body_resource2("https://example.com/testBody2.css",
                                "text/css");
  SimRequest css_body_resource3("https://example.com/testBody3.css",
                                "text/css");

  LoadURL("https://example.com/test.html");

  main_resource.Start();
  main_resource.Write(R"HTML(
    <!DOCTYPE html>
    <html><head>
    <link rel=stylesheet href=testHead.css>
    </head><body>
    <div id="before"></div>
    <link rel=stylesheet href=testBody1.css>
    <div id="after1"></div>
  )HTML");

  test::RunPendingTasks();
  EXPECT_TRUE(GetDocument().getElementById("before"));
  EXPECT_FALSE(GetDocument().getElementById("after1"));
  EXPECT_FALSE(GetDocument().getElementById("after2"));
  EXPECT_FALSE(GetDocument().getElementById("after3"));

  main_resource.Write(
      "<link rel=stylesheet href=testBody2.css>"
      "<div id=\"after2\"></div>");

  test::RunPendingTasks();
  EXPECT_TRUE(GetDocument().getElementById("before"));
  EXPECT_FALSE(GetDocument().getElementById("after1"));
  EXPECT_FALSE(GetDocument().getElementById("after2"));
  EXPECT_FALSE(GetDocument().getElementById("after3"));

  main_resource.Complete(R"HTML(
    <link rel=stylesheet href=testBody3.css>
    <div id="after3"></div>
    </body></html>
  )HTML");

  test::RunPendingTasks();
  EXPECT_TRUE(GetDocument().getElementById("before"));
  EXPECT_FALSE(GetDocument().getElementById("after1"));
  EXPECT_FALSE(GetDocument().getElementById("after2"));
  EXPECT_FALSE(GetDocument().getElementById("after3"));

  // Completing the head css shouldn't change anything
  css_head_resource.Complete("");
  test::RunPendingTasks();
  EXPECT_TRUE(GetDocument().getElementById("before"));
  EXPECT_FALSE(GetDocument().getElementById("after1"));
  EXPECT_FALSE(GetDocument().getElementById("after2"));
  EXPECT_FALSE(GetDocument().getElementById("after3"));

  // Completing the second css shouldn't change anything
  css_body_resource2.Complete("");
  test::RunPendingTasks();
  EXPECT_TRUE(GetDocument().getElementById("before"));
  EXPECT_FALSE(GetDocument().getElementById("after1"));
  EXPECT_FALSE(GetDocument().getElementById("after2"));
  EXPECT_FALSE(GetDocument().getElementById("after3"));

  // Completing the first css should allow the parser to continue past it and
  // the second css which was already completed and then pause again before the
  // third css.
  css_body_resource1.Complete("");
  test::RunPendingTasks();
  EXPECT_TRUE(GetDocument().getElementById("before"));
  EXPECT_TRUE(GetDocument().getElementById("after1"));
  EXPECT_TRUE(GetDocument().getElementById("after2"));
  EXPECT_FALSE(GetDocument().getElementById("after3"));

  // Completing the third css should let it continue to the end.
  css_body_resource3.Complete("");
  test::RunPendingTasks();
  EXPECT_TRUE(GetDocument().getElementById("before"));
  EXPECT_TRUE(GetDocument().getElementById("after1"));
  EXPECT_TRUE(GetDocument().getElementById("after2"));
  EXPECT_TRUE(GetDocument().getElementById("after3"));
}

TEST_P(HTMLDocumentParserLoadingTest,
       ShouldNotPauseParsingForExternalNonMatchingStylesheetsInBody) {
  SimRequest main_resource("https://example.com/test.html", "text/html");
  SimRequest css_head_resource("https://example.com/testHead.css", "text/css");

  LoadURL("https://example.com/test.html");

  main_resource.Complete(R"HTML(
    <!DOCTYPE html>
    <html><head>
    <link rel=stylesheet href=testHead.css>
    </head><body>
    <div id="before"></div>
    <link rel=stylesheet href=testBody.css type='print'>
    <div id="after"></div>
    </body></html>
  )HTML");

  test::RunPendingTasks();
  EXPECT_TRUE(GetDocument().getElementById("before"));
  EXPECT_TRUE(GetDocument().getElementById("after"));

  // Completing the head css shouldn't change anything
  css_head_resource.Complete("");
}

TEST_P(HTMLDocumentParserLoadingTest,
       ShouldPauseParsingForExternalStylesheetsImportedInBody) {
  SimRequest main_resource("https://example.com/test.html", "text/html");
  SimRequest css_head_resource("https://example.com/testHead.css", "text/css");
  SimRequest css_body_resource("https://example.com/testBody.css", "text/css");

  LoadURL("https://example.com/test.html");

  main_resource.Complete(R"HTML(
    <!DOCTYPE html>
    <html><head>
    <link rel=stylesheet href=testHead.css>
    </head><body>
    <div id="before"></div>
    <style>
    @import 'testBody.css'
    </style>
    <div id="after"></div>
    </body></html>
  )HTML");

  test::RunPendingTasks();
  EXPECT_TRUE(GetDocument().getElementById("before"));
  EXPECT_FALSE(GetDocument().getElementById("after"));

  // Completing the head css shouldn't change anything
  css_head_resource.Complete("");
  test::RunPendingTasks();
  EXPECT_TRUE(GetDocument().getElementById("before"));
  EXPECT_FALSE(GetDocument().getElementById("after"));

  // Completing the body resource and pumping the tasks should continue parsing
  // and create the "after" div.
  css_body_resource.Complete("");
  test::RunPendingTasks();
  EXPECT_TRUE(GetDocument().getElementById("before"));
  EXPECT_TRUE(GetDocument().getElementById("after"));
}

TEST_P(HTMLDocumentParserLoadingTest,
       ShouldPauseParsingForExternalStylesheetsWrittenInBody) {
  SimRequest main_resource("https://example.com/test.html", "text/html");
  SimRequest css_head_resource("https://example.com/testHead.css", "text/css");
  SimRequest css_body_resource("https://example.com/testBody.css", "text/css");

  LoadURL("https://example.com/test.html");

  main_resource.Complete(R"HTML(
    <!DOCTYPE html>
    <html><head>
    <link rel=stylesheet href=testHead.css>
    </head><body>
    <div id="before"></div>
    <script>
    document.write('<link rel=stylesheet href=testBody.css>');
    </script>
    <div id="after"></div>
    </body></html>
  )HTML");

  test::RunPendingTasks();
  EXPECT_TRUE(GetDocument().getElementById("before"));
  EXPECT_FALSE(GetDocument().getElementById("after"));

  // Completing the head css shouldn't change anything
  css_head_resource.Complete("");
  test::RunPendingTasks();
  EXPECT_TRUE(GetDocument().getElementById("before"));
  EXPECT_FALSE(GetDocument().getElementById("after"));

  // Completing the body resource and pumping the tasks should continue parsing
  // and create the "after" div.
  css_body_resource.Complete("");
  test::RunPendingTasks();
  EXPECT_TRUE(GetDocument().getElementById("before"));
  EXPECT_TRUE(GetDocument().getElementById("after"));
}

TEST_P(HTMLDocumentParserLoadingTest,
       PendingHeadStylesheetShouldNotBlockParserForBodyInlineStyle) {
  SimRequest main_resource("https://example.com/test.html", "text/html");
  SimRequest css_head_resource("https://example.com/testHead.css", "text/css");

  LoadURL("https://example.com/test.html");

  main_resource.Complete(R"HTML(
    <!DOCTYPE html>
    <html><head>
    <link rel=stylesheet href=testHead.css>
    </head><body>
    <div id="before"></div>
    <style>
    </style>
    <div id="after"></div>
    </body></html>
  )HTML");

  test::RunPendingTasks();
  EXPECT_TRUE(GetDocument().getElementById("before"));
  EXPECT_TRUE(GetDocument().getElementById("after"));
  css_head_resource.Complete("");
}

TEST_P(HTMLDocumentParserLoadingTest,
       PendingHeadStylesheetShouldNotBlockParserForBodyShadowDom) {
  SimRequest main_resource("https://example.com/test.html", "text/html");
  SimRequest css_head_resource("https://example.com/testHead.css", "text/css");

  LoadURL("https://example.com/test.html");

  // The marquee tag has a shadow DOM that synchronously applies a stylesheet.
  main_resource.Complete(R"HTML(
    <!DOCTYPE html>
    <html><head>
    <link rel=stylesheet href=testHead.css>
    </head><body>
    <div id="before"></div>
    <marquee>Marquee</marquee>
    <div id="after"></div>
    </body></html>
  )HTML");

  test::RunPendingTasks();
  EXPECT_TRUE(GetDocument().getElementById("before"));
  EXPECT_TRUE(GetDocument().getElementById("after"));
  css_head_resource.Complete("");
}

TEST_P(HTMLDocumentParserLoadingTest,
       ShouldNotPauseParsingForExternalStylesheetsAttachedInBody) {
  SimRequest main_resource("https://example.com/test.html", "text/html");
  SimRequest css_async_resource("https://example.com/testAsync.css",
                                "text/css");

  LoadURL("https://example.com/test.html");

  main_resource.Complete(R"HTML(
    <!DOCTYPE html>
    <html><head>
    </head><body>
    <div id="before"></div>
    <script>
    var attach  = document.getElementsByTagName('script')[0];
    var link  = document.createElement('link');
    link.rel  = 'stylesheet';
    link.type = 'text/css';
    link.href = 'testAsync.css';
    link.media = 'all';
    attach.appendChild(link);
    </script>
    <div id="after"></div>
    </body></html>
  )HTML");

  test::RunPendingTasks();
  EXPECT_TRUE(GetDocument().getElementById("before"));
  EXPECT_TRUE(GetDocument().getElementById("after"));

  css_async_resource.Complete("");
}

TEST_F(HTMLDocumentParserSimTest, NoRewindNoDocWrite) {
  SimRequest main_resource("https://example.com/test.html", "text/html");
  LoadURL("https://example.com/test.html");

  main_resource.Complete(R"HTML(
    <!DOCTYPE html>
    <html><body>no doc write
    </body></html>
  )HTML");

  test::RunPendingTasks();
  histogram_.ExpectTotalCount("Parser.DiscardedTokenCount", 0);
}

TEST_F(HTMLDocumentParserSimTest, RewindBrokenToken) {
  SimRequest main_resource("https://example.com/test.html", "text/html");
  LoadURL("https://example.com/test.html");

  main_resource.Complete(R"HTML(
    <!DOCTYPE html>
    <script>
    document.write('<a');
    </script>
  )HTML");

  test::RunPendingTasks();
  histogram_.ExpectTotalCount("Parser.DiscardedTokenCount", 1);
}

TEST_F(HTMLDocumentParserSimTest, RewindDifferentNamespace) {
  SimRequest main_resource("https://example.com/test.html", "text/html");
  LoadURL("https://example.com/test.html");

  main_resource.Complete(R"HTML(
    <!DOCTYPE html>
    <script>
    document.write('<svg>');
    </script>
  )HTML");

  test::RunPendingTasks();
  histogram_.ExpectTotalCount("Parser.DiscardedTokenCount", 1);
}

TEST_F(HTMLDocumentParserSimTest, NoRewindSaneDocWrite1) {
  SimRequest main_resource("https://example.com/test.html", "text/html");
  LoadURL("https://example.com/test.html");

  main_resource.Complete(
      "<!DOCTYPE html>"
      "<script>"
      "document.write('<script>console.log(\'hello world\');<\\/script>');"
      "</script>");

  test::RunPendingTasks();
  histogram_.ExpectTotalCount("Parser.DiscardedTokenCount", 0);
}

TEST_F(HTMLDocumentParserSimTest, NoRewindSaneDocWrite2) {
  SimRequest main_resource("https://example.com/test.html", "text/html");
  LoadURL("https://example.com/test.html");

  main_resource.Complete(R"HTML(
    <!DOCTYPE html>
    <script>
    document.write('<p>hello world<\\/p><a>yo');
    </script>
  )HTML");

  test::RunPendingTasks();
  histogram_.ExpectTotalCount("Parser.DiscardedTokenCount", 0);
}

TEST_F(HTMLDocumentParserSimTest, NoRewindSaneDocWriteWithTitle) {
  SimRequest main_resource("https://example.com/test.html", "text/html");
  LoadURL("https://example.com/test.html");

  main_resource.Complete(R"HTML(
    <!DOCTYPE html>
    <html>
    <head>
    <title></title>
    <script>document.write('<p>testing');</script>
    </head>
    <body>
    </body>
    </html>
  )HTML");

  test::RunPendingTasks();
  histogram_.ExpectTotalCount("Parser.DiscardedTokenCount", 0);
}

}  // namespace blink
