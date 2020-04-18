// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <functional>

#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/threading/thread_restrictions.h"
#include "build/build_config.h"
#include "content/public/test/browser_test.h"
#include "headless/public/devtools/domains/dom_snapshot.h"
#include "headless/public/devtools/domains/page.h"
#include "headless/public/devtools/domains/runtime.h"
#include "headless/public/headless_devtools_client.h"
#include "headless/test/headless_render_test.h"
#include "net/test/embedded_test_server/http_response.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

#define HEADLESS_RENDER_BROWSERTEST(clazz)                  \
  class HeadlessRenderBrowserTest##clazz : public clazz {}; \
  HEADLESS_ASYNC_DEVTOOLED_TEST_F(HeadlessRenderBrowserTest##clazz)

#define DISABLED_HEADLESS_RENDER_BROWSERTEST(clazz)         \
  class HeadlessRenderBrowserTest##clazz : public clazz {}; \
  DISABLED_HEADLESS_ASYNC_DEVTOOLED_TEST_F(HeadlessRenderBrowserTest##clazz)

// TODO(dats): For some reason we are missing all HTTP redirects.
// crbug.com/789298
#define DISABLE_HTTP_REDIRECTS_CHECKS

namespace headless {

namespace {

constexpr char kSomeUrl[] = "http://example.com/foobar";
constexpr char kTextHtml[] = "text/html";
constexpr char kApplicationOctetStream[] = "application/octet-stream";
constexpr char kImagePng[] = "image/png";
constexpr char kImageSvgXml[] = "image/svg+xml";

using dom_snapshot::GetSnapshotResult;
using dom_snapshot::DOMNode;
using dom_snapshot::LayoutTreeNode;
using net::test_server::HttpRequest;
using net::test_server::HttpResponse;
using net::test_server::BasicHttpResponse;
using net::test_server::RawHttpResponse;
using page::FrameScheduledNavigationReason;
using testing::ElementsAre;
using testing::UnorderedElementsAre;
using testing::Eq;
using testing::Ne;
using testing::StartsWith;

template <typename T, typename V>
std::vector<T> ElementsView(const std::vector<std::unique_ptr<V>>& elements,
                            std::function<bool(const V&)> filter,
                            std::function<T(const V&)> transform) {
  std::vector<T> result;
  for (const auto& element : elements) {
    if (filter(*element))
      result.push_back(transform(*element));
  }
  return result;
}

bool HasType(int type, const DOMNode& node) {
  return node.GetNodeType() == type;
}
bool HasName(const char* name, const DOMNode& node) {
  return node.GetNodeName() == name;
}
bool IsTag(const DOMNode& node) {
  return HasType(1, node);
}
bool IsText(const DOMNode& node) {
  return HasType(3, node);
}

std::vector<std::string> TextLayout(const GetSnapshotResult* snapshot) {
  return ElementsView<std::string, LayoutTreeNode>(
      *snapshot->GetLayoutTreeNodes(),
      [](const auto& node) { return node.HasLayoutText(); },
      [](const auto& node) { return node.GetLayoutText(); });
}

std::vector<const DOMNode*> FilterDOM(
    const GetSnapshotResult* snapshot,
    std::function<bool(const DOMNode&)> filter) {
  return ElementsView<const DOMNode*, DOMNode>(
      *snapshot->GetDomNodes(), filter, [](const auto& n) { return &n; });
}

std::vector<const DOMNode*> FindTags(const GetSnapshotResult* snapshot,
                                     const char* name = nullptr) {
  return FilterDOM(snapshot, [name](const auto& n) {
    return IsTag(n) && (!name || HasName(name, n));
  });
}

size_t IndexInDOM(const GetSnapshotResult* snapshot, const DOMNode* node) {
  for (size_t i = 0; i < snapshot->GetDomNodes()->size(); ++i) {
    if (snapshot->GetDomNodes()->at(i).get() == node)
      return i;
  }
  CHECK(false);
  return static_cast<size_t>(-1);
}

const DOMNode* GetAt(const GetSnapshotResult* snapshot, size_t index) {
  CHECK_LE(index, snapshot->GetDomNodes()->size());
  return snapshot->GetDomNodes()->at(index).get();
}

const DOMNode* NextNode(const GetSnapshotResult* snapshot,
                        const DOMNode* node) {
  return GetAt(snapshot, IndexInDOM(snapshot, node) + 1);
}

MATCHER_P(NodeName, expected, "") {
  return arg->GetNodeName() == expected;
}
MATCHER_P(NodeValue, expected, "") {
  return arg->GetNodeValue() == expected;
}
MATCHER_P(NodeType, expected, 0) {
  return arg->GetNodeType() == expected;
}

MATCHER_P(RemoteString, expected, "") {
  return arg->GetType() == runtime::RemoteObjectType::STRING &&
         arg->GetValue()->GetString() == expected;
}

MATCHER_P(RequestPath, expected, "") {
  return arg.relative_url == expected;
}

MATCHER_P(RedirectUrl, expected, "") {
  return arg.first == expected;
}

MATCHER_P(RedirectReason, expected, "") {
  return arg.second == expected;
}

MATCHER_P(CookieValue, expected, "") {
  return arg->GetValue() == expected;
}

const DOMNode* FindTag(const GetSnapshotResult* snapshot, const char* name) {
  auto tags = FindTags(snapshot, name);
  if (tags.empty())
    return nullptr;
  EXPECT_THAT(tags, ElementsAre(NodeName(name)));
  return tags[0];
}

TestInMemoryProtocolHandler::Response HttpRedirect(
    int code,
    const std::string& url,
    const std::string& status = "Moved") {
  CHECK(code >= 300 && code < 400);
  std::stringstream str;
  str << "HTTP/1.1 " << code << " " << status << "\r\nLocation: " << url
      << "\r\n\r\n";
  return TestInMemoryProtocolHandler::Response(str.str());
}

TestInMemoryProtocolHandler::Response HttpOk(
    const std::string& html,
    const std::string& mime_type = kTextHtml) {
  return TestInMemoryProtocolHandler::Response(html, mime_type);
}

TestInMemoryProtocolHandler::Response ResponseFromFile(
    const std::string& file_name,
    const std::string& mime_type) {
  static const base::FilePath kTestDataDirectory(
      FILE_PATH_LITERAL("headless/test/data"));

  base::ScopedAllowBlockingForTesting allow_blocking;

  base::FilePath src_dir;
  CHECK(base::PathService::Get(base::DIR_SOURCE_ROOT, &src_dir));
  base::FilePath file_path =
      src_dir.Append(kTestDataDirectory).Append(file_name);
  std::string contents;
  CHECK(base::ReadFileToString(file_path, &contents));

  return TestInMemoryProtocolHandler::Response(contents, mime_type);
}

}  // namespace

class HelloWorldTest : public HeadlessRenderTest {
 private:
  GURL GetPageUrl(HeadlessDevToolsClient* client) override {
    GetProtocolHandler()->InsertResponse(kSomeUrl, HttpOk(R"|(<!doctype html>
<h1>Hello headless world!</h1>
)|"));
    return GURL(kSomeUrl);
  }

  void VerifyDom(GetSnapshotResult* dom_snapshot) override {
    EXPECT_THAT(FindTags(dom_snapshot),
                ElementsAre(NodeName("HTML"), NodeName("HEAD"),
                            NodeName("BODY"), NodeName("H1")));
    EXPECT_THAT(
        FilterDOM(dom_snapshot, IsText),
        ElementsAre(NodeValue("Hello headless world!"), NodeValue("\n")));
    EXPECT_THAT(TextLayout(dom_snapshot), ElementsAre("Hello headless world!"));
    EXPECT_THAT(GetProtocolHandler()->urls_requested(), ElementsAre(kSomeUrl));
    EXPECT_FALSE(main_frame_.empty());
    EXPECT_TRUE(unconfirmed_frame_redirects_.empty());
    EXPECT_TRUE(confirmed_frame_redirects_.empty());
    EXPECT_THAT(frames_[main_frame_].size(), Eq(1u));
    const auto& frame = frames_[main_frame_][0];
    EXPECT_THAT(frame->GetUrl(), Eq(kSomeUrl));
  }
};
HEADLESS_RENDER_BROWSERTEST(HelloWorldTest);

class TimeoutTest : public HelloWorldTest {
 private:
  void OnPageRenderCompleted() override {
    // Never complete.
  }

  void VerifyDom(GetSnapshotResult* dom_snapshot) override {
    FAIL() << "Should not reach here";
  }

  void OnTimeout() override { SetTestCompleted(); }
};
HEADLESS_RENDER_BROWSERTEST(TimeoutTest);

class JavaScriptOverrideTitle_JsEnabled : public HeadlessRenderTest {
 private:
  GURL GetPageUrl(HeadlessDevToolsClient* client) override {
    GetProtocolHandler()->InsertResponse(kSomeUrl, HttpOk(R"|(
<html>
  <head>
    <title>JavaScript is off</title>
    <script language="JavaScript">
      <!-- Begin
        document.title = 'JavaScript is on';
      //  End -->
    </script>
  </head>
  <body onload="settitle()">
    Hello, World!
  </body>
</html>
)|"));
    return GURL(kSomeUrl);
  }

  void VerifyDom(GetSnapshotResult* dom_snapshot) override {
    const DOMNode* value =
        NextNode(dom_snapshot, FindTag(dom_snapshot, "TITLE"));
    EXPECT_THAT(value, NodeValue("JavaScript is on"));
  }
};
HEADLESS_RENDER_BROWSERTEST(JavaScriptOverrideTitle_JsEnabled);

class JavaScriptOverrideTitle_JsDisabled
    : public JavaScriptOverrideTitle_JsEnabled {
 private:
  void OverrideWebPreferences(WebPreferences* preferences) override {
    JavaScriptOverrideTitle_JsEnabled::OverrideWebPreferences(preferences);
    preferences->javascript_enabled = false;
  }

  void VerifyDom(GetSnapshotResult* dom_snapshot) override {
    const DOMNode* value =
        NextNode(dom_snapshot, FindTag(dom_snapshot, "TITLE"));
    EXPECT_THAT(value, NodeValue("JavaScript is off"));
  }
};
HEADLESS_RENDER_BROWSERTEST(JavaScriptOverrideTitle_JsDisabled);

class JavaScriptConsoleErrors : public HeadlessRenderTest {
 private:
  GURL GetPageUrl(HeadlessDevToolsClient* client) override {
    GetProtocolHandler()->InsertResponse(kSomeUrl, HttpOk(R"|(
<html>
  <head>
    <script language="JavaScript">
      <![CDATA[
        function image() {
          window.open('<xsl:value-of select="/IMAGE/@href" />');
        }
      ]]>
    </script>
  </head>
  <body onload="func3()">
    <script type="text/javascript">
      func1()
    </script>
    <script type="text/javascript">
      func2();
    </script>
    <script type="text/javascript">
      console.log("Hello, Script!");
    </script>
  </body>
</html>
)|"));
    return GURL(kSomeUrl);
  }

  void VerifyDom(GetSnapshotResult* dom_snapshot) override {
    EXPECT_THAT(console_log_, ElementsAre("L Hello, Script!"));
    EXPECT_THAT(js_exceptions_,
                ElementsAre(StartsWith("Uncaught SyntaxError:"),
                            StartsWith("Uncaught ReferenceError: func1"),
                            StartsWith("Uncaught ReferenceError: func2"),
                            StartsWith("Uncaught ReferenceError: func3")));
  }
};
HEADLESS_RENDER_BROWSERTEST(JavaScriptConsoleErrors);

class DelayedCompletion : public HeadlessRenderTest {
 private:
  base::TimeTicks start_;

  GURL GetPageUrl(HeadlessDevToolsClient* client) override {
    GetProtocolHandler()->InsertResponse(kSomeUrl, HttpOk(R"|(
<html>
  <body>
   <script type="text/javascript">
     setTimeout(() => {
       var div = document.getElementById('content');
       var p = document.createElement('p');
       p.textContent = 'delayed text';
       div.appendChild(p);
     }, 3000);
   </script>
    <div id="content"/>
  </body>
</html>
)|"));
    start_ = base::TimeTicks::Now();
    return GURL(kSomeUrl);
  }

  void VerifyDom(GetSnapshotResult* dom_snapshot) override {
    base::TimeTicks end = base::TimeTicks::Now();
    EXPECT_THAT(
        FindTags(dom_snapshot),
        ElementsAre(NodeName("HTML"), NodeName("HEAD"), NodeName("BODY"),
                    NodeName("SCRIPT"), NodeName("DIV"), NodeName("P")));
    const DOMNode* value = NextNode(dom_snapshot, FindTag(dom_snapshot, "P"));
    EXPECT_THAT(value, NodeValue("delayed text"));
    // The page delays output for 3 seconds. Due to virtual time this should
    // take significantly less actual time.
    base::TimeDelta passed = end - start_;
    EXPECT_THAT(passed.InSecondsF(), testing::Le(2.9f));
  }
};
HEADLESS_RENDER_BROWSERTEST(DelayedCompletion);

class ClientRedirectChain : public HeadlessRenderTest {
 private:
  GURL GetPageUrl(HeadlessDevToolsClient* client) override {
    GetProtocolHandler()->InsertResponse("http://www.example.com/", HttpOk(R"|(
<html>
  <head>
    <meta http-equiv="refresh" content="0; url=http://www.example.com/1"/>
    <title>Hello, World 0</title>
  </head>
  <body>http://www.example.com/</body>
</html>
)|"));
    GetProtocolHandler()->InsertResponse("http://www.example.com/1", HttpOk(R"|(
<html>
  <head>
    <title>Hello, World 1</title>
    <script>
      document.location='http://www.example.com/2';
    </script>
  </head>
  <body>http://www.example.com/1</body>
</html>
)|"));
    GetProtocolHandler()->InsertResponse("http://www.example.com/2", HttpOk(R"|(
<html>
  <head>
    <title>Hello, World 2</title>
    <script>
      setTimeout("document.location='http://www.example.com/3'", 1000);
    </script>
  </head>
  <body>http://www.example.com/2</body>
</html>
)|"));
    GetProtocolHandler()->InsertResponse("http://www.example.com/3", HttpOk(R"|(
<html>
  <head>
    <title>Pass</title>
  </head>
  <body>
    http://www.example.com/3
    <img src="pass">
  </body>
</html>
)|"));
    return GURL("http://www.example.com/");
  }

  void VerifyDom(GetSnapshotResult* dom_snapshot) override {
    EXPECT_THAT(
        GetProtocolHandler()->urls_requested(),
        ElementsAre("http://www.example.com/", "http://www.example.com/1",
                    "http://www.example.com/2", "http://www.example.com/3",
                    "http://www.example.com/pass"));
    const DOMNode* value =
        NextNode(dom_snapshot, FindTag(dom_snapshot, "TITLE"));
    EXPECT_THAT(value, NodeValue("Pass"));
    EXPECT_THAT(
        confirmed_frame_redirects_[main_frame_],
        ElementsAre(
            RedirectReason(FrameScheduledNavigationReason::META_TAG_REFRESH),
            RedirectReason(FrameScheduledNavigationReason::SCRIPT_INITIATED),
            RedirectReason(FrameScheduledNavigationReason::SCRIPT_INITIATED)));
    EXPECT_THAT(frames_[main_frame_].size(), Eq(4u));
  }
};
HEADLESS_RENDER_BROWSERTEST(ClientRedirectChain);

class ClientRedirectChain_NoJs : public ClientRedirectChain {
 private:
  void OverrideWebPreferences(WebPreferences* preferences) override {
    ClientRedirectChain::OverrideWebPreferences(preferences);
    preferences->javascript_enabled = false;
  }

  void VerifyDom(GetSnapshotResult* dom_snapshot) override {
    EXPECT_THAT(
        GetProtocolHandler()->urls_requested(),
        ElementsAre("http://www.example.com/", "http://www.example.com/1"));
    const DOMNode* value =
        NextNode(dom_snapshot, FindTag(dom_snapshot, "TITLE"));
    EXPECT_THAT(value, NodeValue("Hello, World 1"));
    EXPECT_THAT(confirmed_frame_redirects_[main_frame_],
                ElementsAre(RedirectReason(
                    FrameScheduledNavigationReason::META_TAG_REFRESH)));
    EXPECT_THAT(frames_[main_frame_].size(), Eq(2u));
  }
};
HEADLESS_RENDER_BROWSERTEST(ClientRedirectChain_NoJs);

class ServerRedirectChain : public HeadlessRenderTest {
 private:
  GURL GetPageUrl(HeadlessDevToolsClient* client) override {
    GetProtocolHandler()->InsertResponse(
        "http://www.example.com/",
        HttpRedirect(302, "http://www.example.com/1"));
    GetProtocolHandler()->InsertResponse(
        "http://www.example.com/1",
        HttpRedirect(301, "http://www.example.com/2"));
    GetProtocolHandler()->InsertResponse(
        "http://www.example.com/2",
        HttpRedirect(302, "http://www.example.com/3"));
    GetProtocolHandler()->InsertResponse("http://www.example.com/3",
                                         HttpOk("<p>Pass</p>"));
    return GURL("http://www.example.com/");
  }

  void VerifyDom(GetSnapshotResult* dom_snapshot) override {
    EXPECT_THAT(
        GetProtocolHandler()->urls_requested(),
        ElementsAre("http://www.example.com/", "http://www.example.com/1",
                    "http://www.example.com/2", "http://www.example.com/3"));
    const DOMNode* value = NextNode(dom_snapshot, FindTag(dom_snapshot, "P"));
    EXPECT_THAT(value, NodeValue("Pass"));
#ifndef DISABLE_HTTP_REDIRECTS_CHECKS
    EXPECT_THAT(
        confirmed_frame_redirects_[main_frame_],
        ElementsAre(
            RedirectReason(FrameScheduledNavigationReason::HTTP_HEADER_REFRESH),
            RedirectReason(FrameScheduledNavigationReason::HTTP_HEADER_REFRESH),
            RedirectReason(
                FrameScheduledNavigationReason::HTTP_HEADER_REFRESH)));
    EXPECT_THAT(frames_[main_frame_].size(), Eq(4u));
#endif  // #ifndef DISABLE_HTTP_REDIRECTS_CHECKS
  }
};
HEADLESS_RENDER_BROWSERTEST(ServerRedirectChain);

class ServerRedirectToFailure : public HeadlessRenderTest {
 private:
  GURL GetPageUrl(HeadlessDevToolsClient* client) override {
    GetProtocolHandler()->InsertResponse(
        "http://www.example.com/",
        HttpRedirect(302, "http://www.example.com/1"));
    GetProtocolHandler()->InsertResponse(
        "http://www.example.com/1",
        HttpRedirect(301, "http://www.example.com/FAIL"));
    return GURL("http://www.example.com/");
  }

  void VerifyDom(GetSnapshotResult* dom_snapshot) override {
    EXPECT_THAT(
        GetProtocolHandler()->urls_requested(),
        ElementsAre("http://www.example.com/", "http://www.example.com/1",
                    "http://www.example.com/FAIL"));
  }
};
// Flaky on Linux. https://crbug.com/839747
#if defined(OS_LINUX)
#define MAYBE_HEADLESS_RENDER_BROWSERTEST DISABLED_HEADLESS_RENDER_BROWSERTEST
#else
#define MAYBE_HEADLESS_RENDER_BROWSERTEST HEADLESS_RENDER_BROWSERTEST
#endif
MAYBE_HEADLESS_RENDER_BROWSERTEST(ServerRedirectToFailure);

class ServerRedirectRelativeChain : public HeadlessRenderTest {
 private:
  GURL GetPageUrl(HeadlessDevToolsClient* client) override {
    GetProtocolHandler()->InsertResponse(
        "http://www.example.com/",
        HttpRedirect(302, "http://www.mysite.com/1"));
    GetProtocolHandler()->InsertResponse("http://www.mysite.com/1",
                                         HttpRedirect(301, "/2"));
    GetProtocolHandler()->InsertResponse("http://www.mysite.com/2",
                                         HttpOk("<p>Pass</p>"));
    return GURL("http://www.example.com/");
  }

  void VerifyDom(GetSnapshotResult* dom_snapshot) override {
    EXPECT_THAT(
        GetProtocolHandler()->urls_requested(),
        ElementsAre("http://www.example.com/", "http://www.mysite.com/1",
                    "http://www.mysite.com/2"));
    const DOMNode* value = NextNode(dom_snapshot, FindTag(dom_snapshot, "P"));
    EXPECT_THAT(value, NodeValue("Pass"));
  }
};
HEADLESS_RENDER_BROWSERTEST(ServerRedirectRelativeChain);

class MixedRedirectChain : public HeadlessRenderTest {
 private:
  GURL GetPageUrl(HeadlessDevToolsClient* client) override {
    GetProtocolHandler()->InsertResponse("http://www.example.com/", HttpOk(R"|(
 <html>
   <head>
     <meta http-equiv="refresh" content="0; url=http://www.example.com/1"/>
     <title>Hello, World 0</title>
   </head>
   <body>http://www.example.com/</body>
 </html>
 )|"));
    GetProtocolHandler()->InsertResponse("http://www.example.com/1", HttpOk(R"|(
 <html>
   <head>
     <title>Hello, World 1</title>
     <script>
       document.location='http://www.example.com/2';
     </script>
   </head>
   <body>http://www.example.com/1</body>
 </html>
 )|"));
    GetProtocolHandler()->InsertResponse("http://www.example.com/2",
                                         HttpRedirect(302, "3"));
    GetProtocolHandler()->InsertResponse(
        "http://www.example.com/3",
        HttpRedirect(301, "http://www.example.com/4"));
    GetProtocolHandler()->InsertResponse("http://www.example.com/4",
                                         HttpOk("<p>Pass</p>"));
    return GURL("http://www.example.com/");
  }

  void VerifyDom(GetSnapshotResult* dom_snapshot) override {
    EXPECT_THAT(
        GetProtocolHandler()->urls_requested(),
        ElementsAre("http://www.example.com/", "http://www.example.com/1",
                    "http://www.example.com/2", "http://www.example.com/3",
                    "http://www.example.com/4"));
    const DOMNode* value = NextNode(dom_snapshot, FindTag(dom_snapshot, "P"));
    EXPECT_THAT(value, NodeValue("Pass"));
  }
};
HEADLESS_RENDER_BROWSERTEST(MixedRedirectChain);

class FramesRedirectChain : public HeadlessRenderTest {
 private:
  GURL GetPageUrl(HeadlessDevToolsClient* client) override {
    GetProtocolHandler()->InsertResponse(
        "http://www.example.com/",
        HttpRedirect(302, "http://www.example.com/1"));
    GetProtocolHandler()->InsertResponse("http://www.example.com/1", HttpOk(R"|(
<html>
 <frameset>
  <frame src="http://www.example.com/frameA/">
  <frame src="http://www.example.com/frameB/">
 </frameset>
</html>
)|"));

    // Frame A
    GetProtocolHandler()->InsertResponse("http://www.example.com/frameA/",
                                         HttpOk(R"|(
<html>
 <head>
  <script>document.location='http://www.example.com/frameA/1'</script>
 </head>
 <body>HELLO WORLD 1</body>
</html>
)|"));
    GetProtocolHandler()->InsertResponse("http://www.example.com/frameA/1",
                                         HttpRedirect(301, "/frameA/2"));
    GetProtocolHandler()->InsertResponse("http://www.example.com/frameA/2",
                                         HttpOk("<p>FRAME A</p>"));

    // Frame B
    GetProtocolHandler()->InsertResponse("http://www.example.com/frameB/",
                                         HttpOk(R"|(
<html>
 <head><title>HELLO WORLD 2</title></head>
 <body>
  <iframe src="http://www.example.com/iframe/"></iframe>
 </body>
</html>
)|"));
    GetProtocolHandler()->InsertResponse("http://www.example.com/iframe/",
                                         HttpOk(R"|(
<html>
 <head>
  <script>document.location='http://www.example.com/iframe/1'</script>
 </head>
 <body>HELLO WORLD 1</body>
</html>
)|"));
    GetProtocolHandler()->InsertResponse("http://www.example.com/iframe/1",
                                         HttpRedirect(302, "/iframe/2"));
    GetProtocolHandler()->InsertResponse("http://www.example.com/iframe/2",
                                         HttpRedirect(301, "3"));
    GetProtocolHandler()->InsertResponse("http://www.example.com/iframe/3",
                                         HttpOk("<p>IFRAME B</p>"));
    return GURL("http://www.example.com/");
  }

  void VerifyDom(GetSnapshotResult* dom_snapshot) override {
    EXPECT_THAT(
        GetProtocolHandler()->urls_requested(),
        UnorderedElementsAre(
            "http://www.example.com/", "http://www.example.com/1",
            "http://www.example.com/frameA/", "http://www.example.com/frameA/1",
            "http://www.example.com/frameA/2", "http://www.example.com/frameB/",
            "http://www.example.com/iframe/", "http://www.example.com/iframe/1",
            "http://www.example.com/iframe/2",
            "http://www.example.com/iframe/3"));
    auto dom = FindTags(dom_snapshot, "P");
    EXPECT_THAT(dom, ElementsAre(NodeName("P"), NodeName("P")));
    EXPECT_THAT(NextNode(dom_snapshot, dom[0]), NodeValue("FRAME A"));
    EXPECT_THAT(NextNode(dom_snapshot, dom[1]), NodeValue("IFRAME B"));

    const page::Frame* main_frame = nullptr;
    const page::Frame* a_frame = nullptr;
    const page::Frame* b_frame = nullptr;
    const page::Frame* i_frame = nullptr;
    EXPECT_THAT(frames_.size(), Eq(4u));
    for (const auto& it : frames_) {
      if (it.second.back()->GetUrl() == "http://www.example.com/1")
        main_frame = it.second.back().get();
      else if (it.second.back()->GetUrl() == "http://www.example.com/frameA/2")
        a_frame = it.second.back().get();
      else if (it.second.back()->GetUrl() == "http://www.example.com/frameB/")
        b_frame = it.second.back().get();
      else if (it.second.back()->GetUrl() == "http://www.example.com/iframe/3")
        i_frame = it.second.back().get();
      else
        ADD_FAILURE() << "Unexpected frame URL: " << it.second.back()->GetUrl();
    }

#ifndef DISABLE_HTTP_REDIRECTS_CHECKS
    EXPECT_THAT(frames_[main_frame->GetId()].size(), Eq(2u));
    EXPECT_THAT(frames_[a_frame->GetId()].size(), Eq(3u));
    EXPECT_THAT(frames_[b_frame->GetId()].size(), Eq(1u));
    EXPECT_THAT(frames_[i_frame->GetId()].size(), Eq(4u));
    EXPECT_THAT(confirmed_frame_redirects_[main_frame->GetId()],
                ElementsAre(RedirectReason(
                    FrameScheduledNavigationReason::HTTP_HEADER_REFRESH)));
    EXPECT_THAT(
        confirmed_frame_redirects_[a_frame->GetId()],
        ElementsAre(
            RedirectReason(FrameScheduledNavigationReason::SCRIPT_INITIATED),
            RedirectReason(
                FrameScheduledNavigationReason::HTTP_HEADER_REFRESH)));
    EXPECT_THAT(
        confirmed_frame_redirects_[i_frame->GetId()],
        ElementsAre(
            RedirectReason(FrameScheduledNavigationReason::SCRIPT_INITIATED),
            RedirectReason(FrameScheduledNavigationReason::HTTP_HEADER_REFRESH),
            RedirectReason(
                FrameScheduledNavigationReason::HTTP_HEADER_REFRESH)));
#endif  // #ifndef DISABLE_HTTP_REDIRECTS_CHECKS
  }
};
HEADLESS_RENDER_BROWSERTEST(FramesRedirectChain);

class DoubleRedirect : public HeadlessRenderTest {
 private:
  GURL GetPageUrl(HeadlessDevToolsClient* client) override {
    GetProtocolHandler()->InsertResponse("http://www.example.com/", HttpOk(R"|(
<html>
  <head>
    <title>Hello, World 1</title>
    <script>
      document.location='http://www.example.com/1';
      document.location='http://www.example.com/2';
    </script>
  </head>
  <body>http://www.example.com/1</body>
</html>
)|"));
    GetProtocolHandler()->InsertResponse("http://www.example.com/2",
                                         HttpOk("<p>Pass</p>"));
    return GURL("http://www.example.com/");
  }

  void VerifyDom(GetSnapshotResult* dom_snapshot) override {
    EXPECT_THAT(
        GetProtocolHandler()->urls_requested(),
        ElementsAre("http://www.example.com/", "http://www.example.com/2"));
    EXPECT_THAT(NextNode(dom_snapshot, FindTag(dom_snapshot, "P")),
                NodeValue("Pass"));
    EXPECT_THAT(confirmed_frame_redirects_[main_frame_],
                ElementsAre(RedirectReason(
                    FrameScheduledNavigationReason::SCRIPT_INITIATED)));
    EXPECT_THAT(frames_[main_frame_].size(), Eq(2u));
  }
};
HEADLESS_RENDER_BROWSERTEST(DoubleRedirect);

class RedirectAfterCompletion : public HeadlessRenderTest {
 private:
  GURL GetPageUrl(HeadlessDevToolsClient* client) override {
    GetProtocolHandler()->InsertResponse("http://www.example.com/", HttpOk(R"|(
<html>
 <head>
  <meta http-equiv='refresh' content='120; url=http://www.example.com/1'>
 </head>
 <body><p>Pass</p></body>
</html>
)|"));
    GetProtocolHandler()->InsertResponse("http://www.example.com/1",
                                         HttpOk("<p>Fail</p>"));
    return GURL("http://www.example.com/");
  }

  void VerifyDom(GetSnapshotResult* dom_snapshot) override {
    EXPECT_THAT(GetProtocolHandler()->urls_requested(),
                ElementsAre("http://www.example.com/"));
    EXPECT_THAT(NextNode(dom_snapshot, FindTag(dom_snapshot, "P")),
                NodeValue("Pass"));
    EXPECT_THAT(confirmed_frame_redirects_[main_frame_], ElementsAre());
    EXPECT_THAT(frames_[main_frame_].size(), Eq(1u));
  }
};
HEADLESS_RENDER_BROWSERTEST(RedirectAfterCompletion);

class Redirect307PostMethod : public HeadlessRenderTest {
 private:
  GURL GetPageUrl(HeadlessDevToolsClient* client) override {
    GetProtocolHandler()->InsertResponse("http://www.example.com/", HttpOk(R"|(
<html>
 <body onload='document.forms[0].submit();'>
  <form action='1' method='post'>
   <input name='foo' value='bar'>
  </form>
 </body>
</html>
)|"));
    GetProtocolHandler()->InsertResponse("http://www.example.com/1",
                                         HttpRedirect(307, "/2"));
    GetProtocolHandler()->InsertResponse("http://www.example.com/2",
                                         HttpOk("<p>Pass</p>"));
    return GURL("http://www.example.com/");
  }

  void VerifyDom(GetSnapshotResult* dom_snapshot) override {
    EXPECT_THAT(
        GetProtocolHandler()->urls_requested(),
        ElementsAre("http://www.example.com/", "http://www.example.com/1",
                    "http://www.example.com/2"));
    EXPECT_THAT(GetProtocolHandler()->methods_requested(),
                ElementsAre("GET", "POST", "POST"));
    EXPECT_THAT(NextNode(dom_snapshot, FindTag(dom_snapshot, "P")),
                NodeValue("Pass"));
  }
};
HEADLESS_RENDER_BROWSERTEST(Redirect307PostMethod);

class RedirectPostChain : public HeadlessRenderTest {
 private:
  GURL GetPageUrl(HeadlessDevToolsClient* client) override {
    GetProtocolHandler()->InsertResponse("http://www.example.com/", HttpOk(R"|(
<html>
 <body onload='document.forms[0].submit();'>
  <form action='1' method='post'>
   <input name='foo' value='bar'>
  </form>
 </body>
</html>
)|"));
    GetProtocolHandler()->InsertResponse("http://www.example.com/1",
                                         HttpRedirect(307, "/2"));
    GetProtocolHandler()->InsertResponse("http://www.example.com/2", HttpOk(R"|(
<html>
 <body onload='document.forms[0].submit();'>
  <form action='3' method='post'>
  </form>
 </body>
</html>
)|"));
    GetProtocolHandler()->InsertResponse("http://www.example.com/3",
                                         HttpRedirect(307, "/4"));
    GetProtocolHandler()->InsertResponse("http://www.example.com/4",
                                         HttpOk("<p>Pass</p>"));
    return GURL("http://www.example.com/");
  }

  void VerifyDom(GetSnapshotResult* dom_snapshot) override {
    EXPECT_THAT(
        GetProtocolHandler()->urls_requested(),
        ElementsAre("http://www.example.com/", "http://www.example.com/1",
                    "http://www.example.com/2", "http://www.example.com/3",
                    "http://www.example.com/4"));
    EXPECT_THAT(GetProtocolHandler()->methods_requested(),
                ElementsAre("GET", "POST", "POST", "POST", "POST"));
    EXPECT_THAT(NextNode(dom_snapshot, FindTag(dom_snapshot, "P")),
                NodeValue("Pass"));
  }
};
HEADLESS_RENDER_BROWSERTEST(RedirectPostChain);

class Redirect307PutMethod : public HeadlessRenderTest {
 private:
  GURL GetPageUrl(HeadlessDevToolsClient* client) override {
    GetProtocolHandler()->InsertResponse("http://www.example.com/", HttpOk(R"|(
 <html>
  <head>
   <script>
    function doPut() {
      var xhr = new XMLHttpRequest();
      xhr.open('PUT', 'http://www.example.com/1');
      xhr.setRequestHeader('Content-Type', 'text/plain');
      xhr.addEventListener('load', function() {
        document.getElementById('content').textContent = this.responseText;
      });
      xhr.send('some data');
    }
   </script>
  </head>
  <body onload='doPut();'>
   <p id="content"></p>
  </body>
 </html>
 )|"));
    GetProtocolHandler()->InsertResponse("http://www.example.com/1",
                                         HttpRedirect(307, "/2"));
    GetProtocolHandler()->InsertResponse("http://www.example.com/2",
                                         {"Pass", "text/plain"});
    return GURL("http://www.example.com/");
  }

  void VerifyDom(GetSnapshotResult* dom_snapshot) override {
    EXPECT_THAT(
        GetProtocolHandler()->urls_requested(),
        ElementsAre("http://www.example.com/", "http://www.example.com/1",
                    "http://www.example.com/2"));
    EXPECT_THAT(GetProtocolHandler()->methods_requested(),
                ElementsAre("GET", "PUT", "PUT"));
    EXPECT_THAT(NextNode(dom_snapshot, FindTag(dom_snapshot, "P")),
                NodeValue("Pass"));
  }
};
HEADLESS_RENDER_BROWSERTEST(Redirect307PutMethod);

class Redirect303PutGet : public HeadlessRenderTest {
 private:
  GURL GetPageUrl(HeadlessDevToolsClient* client) override {
    GetProtocolHandler()->InsertResponse("http://www.example.com/", HttpOk(R"|(
<html>
 <head>
  <script>
   function doPut() {
     var xhr = new XMLHttpRequest();
     xhr.open('PUT', 'http://www.example.com/1');
     xhr.setRequestHeader('Content-Type', 'text/plain');
     xhr.addEventListener('load', function() {
       document.getElementById('content').textContent = this.responseText;
     });
     xhr.send('some data');
   }
  </script>
 </head>
 <body onload='doPut();'>
  <p id="content"></p>
 </body>
</html>
)|"));
    GetProtocolHandler()->InsertResponse("http://www.example.com/1",
                                         HttpRedirect(303, "/2"));
    GetProtocolHandler()->InsertResponse("http://www.example.com/2",
                                         {"Pass", "text/plain"});
    return GURL("http://www.example.com/");
  }

  void VerifyDom(GetSnapshotResult* dom_snapshot) override {
    EXPECT_THAT(
        GetProtocolHandler()->urls_requested(),
        ElementsAre("http://www.example.com/", "http://www.example.com/1",
                    "http://www.example.com/2"));
    EXPECT_THAT(GetProtocolHandler()->methods_requested(),
                ElementsAre("GET", "PUT", "GET"));
    EXPECT_THAT(NextNode(dom_snapshot, FindTag(dom_snapshot, "P")),
                NodeValue("Pass"));
  }
};
HEADLESS_RENDER_BROWSERTEST(Redirect303PutGet);

class RedirectBaseUrl : public HeadlessRenderTest {
 private:
  GURL GetPageUrl(HeadlessDevToolsClient* client) override {
    GetProtocolHandler()->InsertResponse("http://foo.com/",
                                         HttpRedirect(302, "http://bar.com/"));
    GetProtocolHandler()->InsertResponse("http://bar.com/",
                                         HttpOk("<img src=\"pass\">"));
    return GURL("http://foo.com/");
  }

  void VerifyDom(GetSnapshotResult* dom_snapshot) override {
    EXPECT_THAT(GetProtocolHandler()->urls_requested(),
                ElementsAre("http://foo.com/", "http://bar.com/",
                            "http://bar.com/pass"));
  }
};
HEADLESS_RENDER_BROWSERTEST(RedirectBaseUrl);

class RedirectNonAsciiUrl : public HeadlessRenderTest {
 private:
  GURL GetPageUrl(HeadlessDevToolsClient* client) override {
    // "中文" is 0xE4 0xB8 0xAD, 0xE6 0x96 0x87
    GetProtocolHandler()->InsertResponse(
        "http://www.example.com/",
        HttpRedirect(302, "http://www.example.com/中文"));
    GetProtocolHandler()->InsertResponse(
        "http://www.example.com/%E4%B8%AD%E6%96%87",
        HttpRedirect(303, "http://www.example.com/pass#中文"));
    GetProtocolHandler()->InsertResponse(
        "http://www.example.com/pass#%E4%B8%AD%E6%96%87",
        HttpOk("<p>Pass</p>"));
    GetProtocolHandler()->InsertResponse(
        "http://www.example.com/%C3%A4%C2%B8%C2%AD%C3%A6%C2%96%C2%87",
        {"HTTP/1.1 500 Bad Response\r\nContent-Type: text/html\r\n\r\nFail"});
    return GURL("http://www.example.com/");
  }

  void VerifyDom(GetSnapshotResult* dom_snapshot) override {
    EXPECT_THAT(GetProtocolHandler()->urls_requested(),
                ElementsAre("http://www.example.com/",
                            "http://www.example.com/%E4%B8%AD%E6%96%87",
                            "http://www.example.com/pass#%E4%B8%AD%E6%96%87"));
    EXPECT_THAT(NextNode(dom_snapshot, FindTag(dom_snapshot, "P")),
                NodeValue("Pass"));
  }
};
HEADLESS_RENDER_BROWSERTEST(RedirectNonAsciiUrl);

class RedirectEmptyUrl : public HeadlessRenderTest {
 private:
  GURL GetPageUrl(HeadlessDevToolsClient* client) override {
    GetProtocolHandler()->InsertResponse(
        "http://www.example.com/",
        {"HTTP/1.1 302 Found\r\nLocation: \r\n\r\n<!DOCTYPE html><p>Pass</p>"});
    return GURL("http://www.example.com/");
  }

  void VerifyDom(GetSnapshotResult* dom_snapshot) override {
    EXPECT_THAT(GetProtocolHandler()->urls_requested(),
                ElementsAre("http://www.example.com/"));
    EXPECT_THAT(NextNode(dom_snapshot, FindTag(dom_snapshot, "P")),
                NodeValue("Pass"));
  }
};
HEADLESS_RENDER_BROWSERTEST(RedirectEmptyUrl);

class RedirectInvalidUrl : public HeadlessRenderTest {
 private:
  GURL GetPageUrl(HeadlessDevToolsClient* client) override {
    GetProtocolHandler()->InsertResponse(
        "http://www.example.com/",
        {"HTTP/1.1 302 Found\r\nLocation: http://\r\n\r\n"
         "<!DOCTYPE html><p>Pass</p>"});
    return GURL("http://www.example.com/");
  }

  void VerifyDom(GetSnapshotResult* dom_snapshot) override {
    EXPECT_THAT(GetProtocolHandler()->urls_requested(),
                ElementsAre("http://www.example.com/"));
  }
};
// Flaky on Linux. https://crbug.com/839747
MAYBE_HEADLESS_RENDER_BROWSERTEST(RedirectInvalidUrl);

class RedirectKeepsFragment : public HeadlessRenderTest {
 private:
  GURL GetPageUrl(HeadlessDevToolsClient* client) override {
    GetProtocolHandler()->InsertResponse("http://www.example.com/#foo",
                                         HttpRedirect(302, "/1"));
    GetProtocolHandler()->InsertResponse("http://www.example.com/1#foo",
                                         HttpRedirect(302, "/2"));
    GetProtocolHandler()->InsertResponse("http://www.example.com/2#foo",
                                         HttpOk(R"|(
<body>
 <p id="content"></p>
 <script>
  document.getElementById('content').textContent = window.location.href;
 </script>
</body>
)|"));
    return GURL("http://www.example.com/#foo");
  }

  void VerifyDom(GetSnapshotResult* dom_snapshot) override {
    EXPECT_THAT(GetProtocolHandler()->urls_requested(),
                ElementsAre("http://www.example.com/#foo",
                            "http://www.example.com/1#foo",
                            "http://www.example.com/2#foo"));
    EXPECT_THAT(NextNode(dom_snapshot, FindTag(dom_snapshot, "P")),
                NodeValue("http://www.example.com/2#foo"));
  }
};
HEADLESS_RENDER_BROWSERTEST(RedirectKeepsFragment);

class RedirectReplacesFragment : public HeadlessRenderTest {
 private:
  GURL GetPageUrl(HeadlessDevToolsClient* client) override {
    GetProtocolHandler()->InsertResponse("http://www.example.com/#foo",
                                         HttpRedirect(302, "/1#bar"));
    GetProtocolHandler()->InsertResponse("http://www.example.com/1#bar",
                                         HttpRedirect(302, "/2"));
    GetProtocolHandler()->InsertResponse("http://www.example.com/2#bar",
                                         HttpOk(R"|(
<body>
 <p id="content"></p>
 <script>
  document.getElementById('content').textContent = window.location.href;
 </script>
</body>
)|"));
    return GURL("http://www.example.com/#foo");
  }

  void VerifyDom(GetSnapshotResult* dom_snapshot) override {
    EXPECT_THAT(GetProtocolHandler()->urls_requested(),
                ElementsAre("http://www.example.com/#foo",
                            "http://www.example.com/1#bar",
                            "http://www.example.com/2#bar"));
    EXPECT_THAT(NextNode(dom_snapshot, FindTag(dom_snapshot, "P")),
                NodeValue("http://www.example.com/2#bar"));
  }
};
HEADLESS_RENDER_BROWSERTEST(RedirectReplacesFragment);

class RedirectNewFragment : public HeadlessRenderTest {
 private:
  GURL GetPageUrl(HeadlessDevToolsClient* client) override {
    GetProtocolHandler()->InsertResponse("http://www.example.com/",
                                         HttpRedirect(302, "/1#foo"));
    GetProtocolHandler()->InsertResponse("http://www.example.com/1#foo",
                                         HttpRedirect(302, "/2"));
    GetProtocolHandler()->InsertResponse("http://www.example.com/2#foo",
                                         HttpOk(R"|(
<body>
 <p id="content"></p>
 <script>
  document.getElementById('content').textContent = window.location.href;
 </script>
</body>
)|"));
    return GURL("http://www.example.com/");
  }

  void VerifyDom(GetSnapshotResult* dom_snapshot) override {
    EXPECT_THAT(
        GetProtocolHandler()->urls_requested(),
        ElementsAre("http://www.example.com/", "http://www.example.com/1#foo",
                    "http://www.example.com/2#foo"));
    EXPECT_THAT(NextNode(dom_snapshot, FindTag(dom_snapshot, "P")),
                NodeValue("http://www.example.com/2#foo"));
  }
};
HEADLESS_RENDER_BROWSERTEST(RedirectNewFragment);

class WindowLocationFragments : public HeadlessRenderTest {
 private:
  GURL GetPageUrl(HeadlessDevToolsClient* client) override {
    GetProtocolHandler()->InsertResponse("http://www.example.com/#fragment1",
                                         HttpOk(R"|(
 <script>
   if (window.location.hash == '#fragment1') {
     document.write('<iframe src="iframe#fragment2"></iframe>');
   }
 </script>)|"));
    GetProtocolHandler()->InsertResponse(
        "http://www.example.com/iframe#fragment2", HttpOk(R"|(
 <script>
   if (window.location.hash == '#fragment2') {
     document.location = 'http://www.example.com/pass';
   }
 </script>)|"));
    GetProtocolHandler()->InsertResponse("http://www.example.com/pass",
                                         HttpOk("<p>Pass</p>"));
    return GURL("http://www.example.com/#fragment1");
  }

  void VerifyDom(GetSnapshotResult* dom_snapshot) override {
    EXPECT_THAT(GetProtocolHandler()->urls_requested(),
                ElementsAre("http://www.example.com/#fragment1",
                            "http://www.example.com/iframe#fragment2",
                            "http://www.example.com/pass"));
    EXPECT_THAT(NextNode(dom_snapshot, FindTag(dom_snapshot, "P")),
                NodeValue("Pass"));
  }
};
HEADLESS_RENDER_BROWSERTEST(WindowLocationFragments);

class CookieSetFromJs : public HeadlessRenderTest {
 private:
  GURL GetPageUrl(HeadlessDevToolsClient* client) override {
    GetProtocolHandler()->InsertResponse("http://www.example.com/", HttpOk(R"|(
<html><head><script>
document.cookie = 'SessionID=123';
n = document.cookie.indexOf('SessionID');
if (n < 0) {
  top.location = '/epicfail';
}
</script></head><body>Pass</body></html>)|"));
    return GURL("http://www.example.com/");
  }

  void VerifyDom(GetSnapshotResult* dom_snapshot) override {
    EXPECT_THAT(GetProtocolHandler()->urls_requested(),
                ElementsAre("http://www.example.com/"));
    EXPECT_THAT(NextNode(dom_snapshot, FindTag(dom_snapshot, "BODY")),
                NodeValue("Pass"));
  }
};
HEADLESS_RENDER_BROWSERTEST(CookieSetFromJs);

class CookieSetFromJs_NoCookies : public CookieSetFromJs {
 private:
  void OverrideWebPreferences(WebPreferences* preferences) override {
    HeadlessRenderTest::OverrideWebPreferences(preferences);
    preferences->cookie_enabled = false;
  }

  void VerifyDom(GetSnapshotResult* dom_snapshot) override {
    EXPECT_THAT(GetProtocolHandler()->urls_requested(),
                ElementsAre("http://www.example.com/",
                            "http://www.example.com/epicfail"));
  }
};

// Flaky on Linux. https://crbug.com/839747
MAYBE_HEADLESS_RENDER_BROWSERTEST(CookieSetFromJs_NoCookies);

class CookieUpdatedFromJs : public HeadlessRenderTest {
 private:
  GURL GetPageUrl(HeadlessDevToolsClient* client) override {
    client->GetNetwork()->SetCookie(network::SetCookieParams::Builder()
                                        .SetUrl("http://www.example.com/")
                                        .SetName("foo")
                                        .SetValue("bar")
                                        .Build());
    GetProtocolHandler()->InsertResponse("http://www.example.com/", HttpOk(R"|(
<html><head><script>
var x = document.cookie;
document.cookie = x + 'baz';
</script></head><body>Pass</body></html>)|"));
    return GURL("http://www.example.com/");
  }

  void OnPageRenderCompleted() override {
    devtools_client_->GetNetwork()->GetCookies(
        network::GetCookiesParams::Builder()
            .SetUrls({"http://www.example.com/"})
            .Build(),
        base::BindOnce(&CookieUpdatedFromJs::OnGetCookies,
                       base::Unretained(this)));
  }

  void OnGetCookies(std::unique_ptr<network::GetCookiesResult> result) {
    const auto& cookies = *result->GetCookies();
    EXPECT_THAT(cookies, ElementsAre(CookieValue("barbaz")));
    HeadlessRenderTest::OnPageRenderCompleted();
  }

  void VerifyDom(GetSnapshotResult* dom_snapshot) override {
    EXPECT_THAT(NextNode(dom_snapshot, FindTag(dom_snapshot, "BODY")),
                NodeValue("Pass"));
  }
};
HEADLESS_RENDER_BROWSERTEST(CookieUpdatedFromJs);

class InCrossOriginObject : public HeadlessRenderTest {
 private:
  GURL GetPageUrl(HeadlessDevToolsClient* client) override {
    GetProtocolHandler()->InsertResponse("http://foo.com/", HttpOk(R"|(
 <html><body>
  <iframe id='myframe' src='http://bar.com/'></iframe>
   <script>
    window.onload = function() {
      try {
        var a = 0 in document.getElementById('myframe').contentWindow;
      } catch (e) {
        console.log(e.message);
      }
    };
 </script><p>Pass</p></body></html>)|"));
    GetProtocolHandler()->InsertResponse("http://bar.com/",
                                         HttpOk(R"|(<html></html>)|"));
    return GURL("http://foo.com/");
  }

  void VerifyDom(GetSnapshotResult* dom_snapshot) override {
    EXPECT_THAT(NextNode(dom_snapshot, FindTag(dom_snapshot, "P")),
                NodeValue("Pass"));
    EXPECT_THAT(console_log_,
                ElementsAre(StartsWith("L Blocked a frame with origin "
                                       "\"http://foo.com\" from accessing")));
  }
};
HEADLESS_RENDER_BROWSERTEST(InCrossOriginObject);

class ContentSecurityPolicy : public HeadlessRenderTest {
 private:
  GURL GetPageUrl(HeadlessDevToolsClient* client) override {
    // Only first 3 scripts of 4 on the page are whitelisted for execution.
    // Therefore only 3 lines in the log are expected.
    GetProtocolHandler()->InsertResponse(
        "http://example.com/",
        {"HTTP/1.1 200 OK\r\n"
         "Content-Type: text/html\r\n"
         "Content-Security-Policy: script-src"
         " 'sha256-INSsCHXoo4K3+jDRF8FSvl13GP22I9vcqcJjkq35Y20='"
         " 'sha384-77lSn5Q6V979pJ8W2TXc6Lrj98LughR0ofkFwa+"
         "qOEtlcofEdLPkOPtpJF8QQMev'"
         " 'sha512-"
         "2cS3KZwfnxFo6lvBvAl113f5N3QCRgtRJBbtFaQHKOhk36sdYYKFvhCqGTvbN7pBKUfsj"
         "fCQgFF4MSbCQuvT8A=='\r\n\r\n"
         "<!DOCTYPE html>\n"
         "<script>console.log('pass256');</script>\n"
         "<script>console.log('pass384');</script>\n"
         "<script>console.log('pass512');</script>\n"
         "<script>console.log('fail');</script>"});
    // For example, regenerate sha256 hash with:
    // echo -n "console.log('pass256');" \
    //   | openssl sha256 -binary \
    //   | openssl base64
    return GURL("http://example.com/");
  }

  void VerifyDom(GetSnapshotResult* dom_snapshot) override {
    EXPECT_THAT(console_log_,
                ElementsAre("L pass256", "L pass384", "L pass512"));
  }
};
HEADLESS_RENDER_BROWSERTEST(ContentSecurityPolicy);

class FrameLoadEvents : public HeadlessRenderTest {
 private:
  std::map<std::string, std::string> frame_navigated_;
  std::map<std::string, std::string> frame_scheduled_;

  GURL GetPageUrl(HeadlessDevToolsClient* client) override {
    GetProtocolHandler()->InsertResponse(
        "http://example.com/", HttpRedirect(302, "http://example.com/1"));

    GetProtocolHandler()->InsertResponse("http://example.com/1", HttpOk(R"|(
<html><frameset>
 <frame src="http://example.com/frameA/" id="frameA">
 <frame src="http://example.com/frameB/" id="frameB">
</frameset></html>
)|"));

    GetProtocolHandler()->InsertResponse("http://example.com/frameA/",
                                         HttpOk(R"|(
<html><head><script>
 document.location="http://example.com/frameA/1"
</script></head></html>
)|"));

    GetProtocolHandler()->InsertResponse("http://example.com/frameB/",
                                         HttpOk(R"|(
<html><head><script>
 document.location="http://example.com/frameB/1"
</script></head></html>
)|"));

    GetProtocolHandler()->InsertResponse(
        "http://example.com/frameA/1",
        HttpOk("<html><body>FRAME A 1</body></html>"));

    GetProtocolHandler()->InsertResponse("http://example.com/frameB/1",
                                         HttpOk(R"|(
<html><body>FRAME B 1
 <iframe src="http://example.com/frameB/1/iframe/" id="iframe"></iframe>
</body></html>
)|"));

    GetProtocolHandler()->InsertResponse("http://example.com/frameB/1/iframe/",
                                         HttpOk(R"|(
<html><head><script>
 document.location="http://example.com/frameB/1/iframe/1"
</script></head></html>
)|"));

    GetProtocolHandler()->InsertResponse(
        "http://example.com/frameB/1/iframe/1",
        HttpOk("<html><body>IFRAME 1</body><html>"));

    return GURL("http://example.com/");
  }

  void OnFrameNavigated(const page::FrameNavigatedParams& params) override {
    frame_navigated_.insert(std::make_pair(params.GetFrame()->GetId(),
                                           params.GetFrame()->GetUrl()));
    HeadlessRenderTest::OnFrameNavigated(params);
  }

  void OnFrameScheduledNavigation(
      const page::FrameScheduledNavigationParams& params) override {
    frame_scheduled_.insert(
        std::make_pair(params.GetFrameId(), params.GetUrl()));
    HeadlessRenderTest::OnFrameScheduledNavigation(params);
  }

  void VerifyDom(GetSnapshotResult* dom_snapshot) override {
    std::vector<std::string> urls;
    for (const auto& kv : frame_navigated_) {
      urls.push_back(kv.second);
    }
    EXPECT_THAT(urls, UnorderedElementsAre(
                          "http://example.com/1", "http://example.com/frameA/",
                          "http://example.com/frameB/",
                          "http://example.com/frameB/1/iframe/"));
    urls.clear();
    for (const auto& kv : frame_scheduled_) {
      urls.push_back(kv.second);
    }
    EXPECT_THAT(urls,
                UnorderedElementsAre("http://example.com/frameA/1",
                                     "http://example.com/frameB/1",
                                     "http://example.com/frameB/1/iframe/1"));
  }
};
HEADLESS_RENDER_BROWSERTEST(FrameLoadEvents);

class CustomFont : public HeadlessRenderTest {
 private:
  GURL GetPageUrl(HeadlessDevToolsClient* client) override {
    GetProtocolHandler()->InsertResponse("http://www.example.com/", HttpOk(R"|(
<html>
  <head>
    <style>
      @font-face {
        font-family: testfont;
        src: url("font.ttf");
      }
      span.test {
        font-family: testfont;
        font-size: 200px;
      }
    </style>
  </head>
  <body>
    <span class="test">Hello</span>
  </body>
</html>
)|"));
    GetProtocolHandler()->InsertResponse(
        "http://www.example.com/font.ttf",
        ResponseFromFile("font.ttf", kApplicationOctetStream));
    return GURL("http://www.example.com/");
  }

  base::Optional<ScreenshotOptions> GetScreenshotOptions() override {
    return ScreenshotOptions("custom_font.png", 0, 0, 500, 250, 1);
  }
};
HEADLESS_RENDER_BROWSERTEST(CustomFont);

// Ensures that "filter: url(...)" does not get into an infinite style update
// loop.
class CssUrlFilter : public HeadlessRenderTest {
 private:
  GURL GetPageUrl(HeadlessDevToolsClient* client) override {
    // The image from circle.svg will be drawn with the blur from blur.svg.
    GetProtocolHandler()->InsertResponse("http://www.example.com/", HttpOk(R"|(
<!DOCTYPE html>
<style>
body { margin: 0; }
img {
  -webkit-filter: url(blur.svg#blur);
  filter: url(blur.svg#blur);
}
</style>
<img src="circle.svg">
)|"));

    // Just a normal image.
    GetProtocolHandler()->InsertResponse("http://www.example.com/circle.svg",
                                         HttpOk(R"|(
<svg width="100" height="100" version="1.1" xmlns="http://www.w3.org/2000/svg"
     xmlns:xlink="http://www.w3.org/1999/xlink">
<circle cx="50" cy="50" r="50" fill="green" />
</svg>
)|",
                                                kImageSvgXml));

    // A blur filter stored inside an svg file.
    GetProtocolHandler()->InsertResponse("http://www.example.com/blur.svg#blur",
                                         HttpOk(R"|(
<svg version="1.1" xmlns="http://www.w3.org/2000/svg"
     xmlns:xlink="http://www.w3.org/1999/xlink">
  <filter id="blur">
    <feGaussianBlur in="SourceGraphic" stdDeviation="5"/>
  </filter>
</svg>
)|",
                                                kImageSvgXml));

    return GURL("http://www.example.com/");
  }

  base::Optional<ScreenshotOptions> GetScreenshotOptions() override {
    return ScreenshotOptions("css_url_filter.png", 0, 0, 100, 100, 1);
  }
};
HEADLESS_RENDER_BROWSERTEST(CssUrlFilter);

// Ensures that a number of SVGs features render correctly.
class SvgExamples : public HeadlessRenderTest {
 private:
  GURL GetPageUrl(HeadlessDevToolsClient* client) override {
    GetProtocolHandler()->InsertResponse(
        "http://www.example.com/",
        ResponseFromFile("svg_examples.svg", kImageSvgXml));
    GetProtocolHandler()->InsertResponse(
        "http://www.example.com/svg_example_image.png",
        ResponseFromFile("svg_example_image.png", kImagePng));

    return GURL("http://www.example.com/");
  }

  base::Optional<ScreenshotOptions> GetScreenshotOptions() override {
    return ScreenshotOptions("svg_examples.png", 0, 0, 400, 600, 1);
  }
};
HEADLESS_RENDER_BROWSERTEST(SvgExamples);

// Ensures that basic <canvas> painting is supported.
class Canvas : public HeadlessRenderTest {
 private:
  GURL GetPageUrl(HeadlessDevToolsClient* client) override {
    GetProtocolHandler()->InsertResponse("http://www.example.com/", HttpOk(R"|(
<html>
  <body>
    <canvas id="test_canvas" width="200" height="200"
            style="position:absolute;left:0px;top:0px">
      Oops!  Canvas not supported!
    </canvas>
    <script>
      var context = document.getElementById("test_canvas").
                    getContext("2d");
      context.fillStyle = "rgb(255,0,0)";
      context.fillRect(30, 30, 50, 50);
    </script>
  </body>
</html>
)|"));

    return GURL("http://www.example.com/");
  }

  base::Optional<ScreenshotOptions> GetScreenshotOptions() override {
    return ScreenshotOptions("canvas.png", 0, 0, 200, 200, 1);
  }
};
HEADLESS_RENDER_BROWSERTEST(Canvas);

}  // namespace headless
