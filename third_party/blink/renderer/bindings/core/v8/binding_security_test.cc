// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/bindings/core/v8/binding_security.h"

#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/frame/use_counter.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/core/testing/sim/sim_request.h"
#include "third_party/blink/renderer/core/testing/sim/sim_test.h"
#include "third_party/blink/renderer/platform/testing/unit_test_helpers.h"

namespace blink {

namespace {
const char kMainFrame[] = "https://example.com/main.html";
const char kSameOriginTarget[] = "https://example.com/target.html";
const char kCrossOriginTarget[] = "https://not-example.com/target.html";
}

class BindingSecurityCounterTest
    : public SimTest,
      public testing::WithParamInterface<const char*> {
 public:
  enum class OriginDisposition { CrossOrigin, SameOrigin };

  BindingSecurityCounterTest() = default;

  void LoadWindowAndAccessProperty(OriginDisposition which_origin,
                                   const String& property) {
    SimRequest main(kMainFrame, "text/html");
    SimRequest target(which_origin == OriginDisposition::CrossOrigin
                          ? kCrossOriginTarget
                          : kSameOriginTarget,
                      "text/html");
    const String& document = String::Format(
        "<!DOCTYPE html>"
        "<script>"
        "  window.addEventListener('message', e => {"
        "    window.other = e.source.%s;"
        "    console.log('yay');"
        "  });"
        "  var w = window.open('%s');"
        "</script>",
        property.Utf8().data(),
        which_origin == OriginDisposition::CrossOrigin ? kCrossOriginTarget
                                                       : kSameOriginTarget);

    LoadURL(kMainFrame);
    main.Complete(document);
    target.Complete(
        "<!DOCTYPE html>"
        "<script>window.opener.postMessage('yay', '*');</script>");
    test::RunPendingTasks();
  }

  void LoadFrameAndAccessProperty(OriginDisposition which_origin,
                                  const String& property) {
    SimRequest main(kMainFrame, "text/html");
    SimRequest target(which_origin == OriginDisposition::CrossOrigin
                          ? kCrossOriginTarget
                          : kSameOriginTarget,
                      "text/html");
    const String& document = String::Format(
        "<!DOCTYPE html>"
        "<body>"
        "<script>"
        "  var i = document.createElement('iframe');"
        "  window.addEventListener('message', e => {"
        "    window.other = e.source.%s;"
        "    console.log('yay');"
        "  });"
        "  i.src = '%s';"
        "  document.body.appendChild(i);"
        "</script>",
        property.Utf8().data(),
        which_origin == OriginDisposition::CrossOrigin ? kCrossOriginTarget
                                                       : kSameOriginTarget);

    LoadURL(kMainFrame);
    main.Complete(document);
    target.Complete(
        "<!DOCTYPE html>"
        "<script>window.top.postMessage('yay', '*');</script>");
    test::RunPendingTasks();
  }
};

INSTANTIATE_TEST_CASE_P(WindowProperties,
                        BindingSecurityCounterTest,
                        testing::Values("window",
                                        "self",
                                        "location",
                                        "close",
                                        "closed",
                                        "focus",
                                        "blur",
                                        "frames",
                                        "length",
                                        "top",
                                        "opener",
                                        "parent",
                                        "postMessage"));

TEST_P(BindingSecurityCounterTest, CrossOriginWindow) {
  LoadWindowAndAccessProperty(OriginDisposition::CrossOrigin, GetParam());
  EXPECT_TRUE(GetDocument().GetPage()->GetUseCounter().HasRecordedMeasurement(
      WebFeature::kCrossOriginPropertyAccess));
  EXPECT_TRUE(GetDocument().GetPage()->GetUseCounter().HasRecordedMeasurement(
      WebFeature::kCrossOriginPropertyAccessFromOpener));
}

TEST_P(BindingSecurityCounterTest, SameOriginWindow) {
  LoadWindowAndAccessProperty(OriginDisposition::SameOrigin, GetParam());
  EXPECT_FALSE(GetDocument().GetPage()->GetUseCounter().HasRecordedMeasurement(
      WebFeature::kCrossOriginPropertyAccess));
  EXPECT_FALSE(GetDocument().GetPage()->GetUseCounter().HasRecordedMeasurement(
      WebFeature::kCrossOriginPropertyAccessFromOpener));
}

TEST_P(BindingSecurityCounterTest, CrossOriginFrame) {
  LoadFrameAndAccessProperty(OriginDisposition::CrossOrigin, GetParam());
  EXPECT_TRUE(GetDocument().GetPage()->GetUseCounter().HasRecordedMeasurement(
      WebFeature::kCrossOriginPropertyAccess));
  EXPECT_FALSE(GetDocument().GetPage()->GetUseCounter().HasRecordedMeasurement(
      WebFeature::kCrossOriginPropertyAccessFromOpener));
}

TEST_P(BindingSecurityCounterTest, SameOriginFrame) {
  LoadFrameAndAccessProperty(OriginDisposition::SameOrigin, GetParam());
  EXPECT_FALSE(GetDocument().GetPage()->GetUseCounter().HasRecordedMeasurement(
      WebFeature::kCrossOriginPropertyAccess));
  EXPECT_FALSE(GetDocument().GetPage()->GetUseCounter().HasRecordedMeasurement(
      WebFeature::kCrossOriginPropertyAccessFromOpener));
}

}  // namespace
