// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include "content/browser/accessibility/browser_accessibility.h"
#include "content/browser/accessibility/browser_accessibility_manager.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/public/test/test_utils.h"
#include "content/shell/browser/shell.h"
#include "content/test/accessibility_browser_test_utils.h"
#include "net/base/data_url.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/codec/png_codec.h"
#include "url/gurl.h"

namespace content {

namespace {

class AccessibilityActionBrowserTest : public ContentBrowserTest {
 public:
  AccessibilityActionBrowserTest() {}
  ~AccessibilityActionBrowserTest() override {}

 protected:
  BrowserAccessibility* FindNode(ax::mojom::Role role,
                                 const std::string& name) {
    BrowserAccessibility* root = GetManager()->GetRoot();
    CHECK(root);
    return FindNodeInSubtree(*root, role, name);
  }

  BrowserAccessibilityManager* GetManager() {
    WebContentsImpl* web_contents =
        static_cast<WebContentsImpl*>(shell()->web_contents());
    return web_contents->GetRootBrowserAccessibilityManager();
  }

  void GetBitmapFromImageDataURL(BrowserAccessibility* target,
                                 SkBitmap* bitmap) {
    std::string image_data_url =
        target->GetStringAttribute(ax::mojom::StringAttribute::kImageDataUrl);
    std::string mimetype;
    std::string charset;
    std::string png_data;
    ASSERT_TRUE(net::DataURL::Parse(GURL(image_data_url),
                                    &mimetype, &charset, &png_data));
    ASSERT_EQ("image/png", mimetype);
    ASSERT_TRUE(gfx::PNGCodec::Decode(
        reinterpret_cast<const unsigned char*>(png_data.data()),
        png_data.size(), bitmap));
  }

 private:
  BrowserAccessibility* FindNodeInSubtree(BrowserAccessibility& node,
                                          ax::mojom::Role role,
                                          const std::string& name) {
    if (node.GetRole() == role &&
        node.GetStringAttribute(ax::mojom::StringAttribute::kName) == name)
      return &node;
    for (unsigned int i = 0; i < node.PlatformChildCount(); ++i) {
      BrowserAccessibility* result = FindNodeInSubtree(
          *node.PlatformGetChild(i), role, name);
      if (result)
        return result;
    }
    return nullptr;
  }
};

}  // namespace

IN_PROC_BROWSER_TEST_F(AccessibilityActionBrowserTest, FocusAction) {
  NavigateToURL(shell(), GURL(url::kAboutBlankURL));

  AccessibilityNotificationWaiter waiter(shell()->web_contents(),
                                         ui::kAXModeComplete,
                                         ax::mojom::Event::kLoadComplete);
  GURL url("data:text/html,"
           "<button>One</button>"
           "<button>Two</button>"
           "<button>Three</button>");
  NavigateToURL(shell(), url);
  waiter.WaitForNotification();

  BrowserAccessibility* target = FindNode(ax::mojom::Role::kButton, "One");
  ASSERT_NE(nullptr, target);

  AccessibilityNotificationWaiter waiter2(
      shell()->web_contents(), ui::kAXModeComplete, ax::mojom::Event::kFocus);
  GetManager()->SetFocus(*target);
  waiter2.WaitForNotification();

  BrowserAccessibility* focus = GetManager()->GetFocus();
  EXPECT_EQ(focus->GetId(), target->GetId());
}

IN_PROC_BROWSER_TEST_F(AccessibilityActionBrowserTest,
                       IncrementDecrementActions) {
  NavigateToURL(shell(), GURL(url::kAboutBlankURL));

  AccessibilityNotificationWaiter waiter(shell()->web_contents(),
                                         ui::kAXModeComplete,
                                         ax::mojom::Event::kLoadComplete);
  GURL url("data:text/html,"
           "<input type=range min=2 value=8 max=10 step=2>");
  NavigateToURL(shell(), url);
  waiter.WaitForNotification();

  BrowserAccessibility* target = FindNode(ax::mojom::Role::kSlider, "");
  ASSERT_NE(nullptr, target);
  EXPECT_EQ(8.0, target->GetFloatAttribute(
                     ax::mojom::FloatAttribute::kValueForRange));

  // Increment, should result in value changing from 8 to 10.
  {
    AccessibilityNotificationWaiter waiter2(shell()->web_contents(),
                                            ui::kAXModeComplete,
                                            ax::mojom::Event::kValueChanged);
    GetManager()->Increment(*target);
    waiter2.WaitForNotification();
  }
  EXPECT_EQ(10.0, target->GetFloatAttribute(
                      ax::mojom::FloatAttribute::kValueForRange));

  // Increment, should result in value staying the same (max).
  {
    AccessibilityNotificationWaiter waiter2(shell()->web_contents(),
                                            ui::kAXModeComplete,
                                            ax::mojom::Event::kValueChanged);
    GetManager()->Increment(*target);
    waiter2.WaitForNotification();
  }
  EXPECT_EQ(10.0, target->GetFloatAttribute(
                      ax::mojom::FloatAttribute::kValueForRange));

  // Decrement, should result in value changing from 10 to 8.
  {
    AccessibilityNotificationWaiter waiter2(shell()->web_contents(),
                                            ui::kAXModeComplete,
                                            ax::mojom::Event::kValueChanged);
    GetManager()->Decrement(*target);
    waiter2.WaitForNotification();
  }
  EXPECT_EQ(8.0, target->GetFloatAttribute(
                     ax::mojom::FloatAttribute::kValueForRange));
}

IN_PROC_BROWSER_TEST_F(AccessibilityActionBrowserTest, CanvasGetImage) {
  NavigateToURL(shell(), GURL(url::kAboutBlankURL));

  AccessibilityNotificationWaiter waiter(shell()->web_contents(),
                                         ui::kAXModeComplete,
                                         ax::mojom::Event::kLoadComplete);
  GURL url("data:text/html,"
           "<body>"
           "<canvas aria-label='canvas' id='c' width='4' height='2'></canvas>"
           "<script>\n"
           "  var c = document.getElementById('c').getContext('2d');\n"
           "  c.beginPath();\n"
           "  c.moveTo(0, 0.5);\n"
           "  c.lineTo(4, 0.5);\n"
           "  c.strokeStyle = '%23ff0000';\n"
           "  c.stroke();\n"
           "  c.beginPath();\n"
           "  c.moveTo(0, 1.5);\n"
           "  c.lineTo(4, 1.5);\n"
           "  c.strokeStyle = '%230000ff';\n"
           "  c.stroke();\n"
           "</script>"
           "</body>");

  NavigateToURL(shell(), url);
  waiter.WaitForNotification();

  BrowserAccessibility* target = FindNode(ax::mojom::Role::kCanvas, "canvas");
  ASSERT_NE(nullptr, target);

  AccessibilityNotificationWaiter waiter2(shell()->web_contents(),
                                          ui::kAXModeComplete,
                                          ax::mojom::Event::kImageFrameUpdated);
  GetManager()->GetImageData(*target, gfx::Size());
  waiter2.WaitForNotification();

  SkBitmap bitmap;
  GetBitmapFromImageDataURL(target, &bitmap);
  ASSERT_EQ(4, bitmap.width());
  ASSERT_EQ(2, bitmap.height());
  EXPECT_EQ(SK_ColorRED, bitmap.getColor(0, 0));
  EXPECT_EQ(SK_ColorRED, bitmap.getColor(1, 0));
  EXPECT_EQ(SK_ColorRED, bitmap.getColor(2, 0));
  EXPECT_EQ(SK_ColorRED, bitmap.getColor(3, 0));
  EXPECT_EQ(SK_ColorBLUE, bitmap.getColor(0, 1));
  EXPECT_EQ(SK_ColorBLUE, bitmap.getColor(1, 1));
  EXPECT_EQ(SK_ColorBLUE, bitmap.getColor(2, 1));
  EXPECT_EQ(SK_ColorBLUE, bitmap.getColor(3, 1));
}

IN_PROC_BROWSER_TEST_F(AccessibilityActionBrowserTest, CanvasGetImageScale) {
  NavigateToURL(shell(), GURL(url::kAboutBlankURL));

  AccessibilityNotificationWaiter waiter(shell()->web_contents(),
                                         ui::kAXModeComplete,
                                         ax::mojom::Event::kLoadComplete);
  GURL url("data:text/html,"
           "<body>"
           "<canvas aria-label='canvas' id='c' width='40' height='20'></canvas>"
           "<script>\n"
           "  var c = document.getElementById('c').getContext('2d');\n"
           "  c.fillStyle = '%2300ff00';\n"
           "  c.fillRect(0, 0, 40, 10);\n"
           "  c.fillStyle = '%23ff00ff';\n"
           "  c.fillRect(0, 10, 40, 10);\n"
           "</script>"
           "</body>");

  NavigateToURL(shell(), url);
  waiter.WaitForNotification();

  BrowserAccessibility* target = FindNode(ax::mojom::Role::kCanvas, "canvas");
  ASSERT_NE(nullptr, target);

  AccessibilityNotificationWaiter waiter2(shell()->web_contents(),
                                          ui::kAXModeComplete,
                                          ax::mojom::Event::kImageFrameUpdated);
  GetManager()->GetImageData(*target, gfx::Size(4, 4));
  waiter2.WaitForNotification();

  SkBitmap bitmap;
  GetBitmapFromImageDataURL(target, &bitmap);
  ASSERT_EQ(4, bitmap.width());
  ASSERT_EQ(2, bitmap.height());
  EXPECT_EQ(SK_ColorGREEN, bitmap.getColor(0, 0));
  EXPECT_EQ(SK_ColorGREEN, bitmap.getColor(1, 0));
  EXPECT_EQ(SK_ColorGREEN, bitmap.getColor(2, 0));
  EXPECT_EQ(SK_ColorGREEN, bitmap.getColor(3, 0));
  EXPECT_EQ(SK_ColorMAGENTA, bitmap.getColor(0, 1));
  EXPECT_EQ(SK_ColorMAGENTA, bitmap.getColor(1, 1));
  EXPECT_EQ(SK_ColorMAGENTA, bitmap.getColor(2, 1));
  EXPECT_EQ(SK_ColorMAGENTA, bitmap.getColor(3, 1));
}

IN_PROC_BROWSER_TEST_F(AccessibilityActionBrowserTest, ImgElementGetImage) {
  NavigateToURL(shell(), GURL(url::kAboutBlankURL));

  AccessibilityNotificationWaiter waiter(shell()->web_contents(),
                                         ui::kAXModeComplete,
                                         ax::mojom::Event::kLoadComplete);
  GURL url("data:text/html,"
           "<body>"
           "<img src='data:image/gif;base64,R0lGODdhAgADAKEDAAAA//"
           "8AAAD/AP///ywAAAAAAgADAAACBEwkAAUAOw=='>"
           "</body>");

  NavigateToURL(shell(), url);
  waiter.WaitForNotification();

  BrowserAccessibility* target = FindNode(ax::mojom::Role::kImage, "");
  ASSERT_NE(nullptr, target);

  AccessibilityNotificationWaiter waiter2(shell()->web_contents(),
                                          ui::kAXModeComplete,
                                          ax::mojom::Event::kImageFrameUpdated);
  GetManager()->GetImageData(*target, gfx::Size());
  waiter2.WaitForNotification();

  SkBitmap bitmap;
  GetBitmapFromImageDataURL(target, &bitmap);
  ASSERT_EQ(2, bitmap.width());
  ASSERT_EQ(3, bitmap.height());
  EXPECT_EQ(SK_ColorRED, bitmap.getColor(0, 0));
  EXPECT_EQ(SK_ColorRED, bitmap.getColor(1, 0));
  EXPECT_EQ(SK_ColorGREEN, bitmap.getColor(0, 1));
  EXPECT_EQ(SK_ColorGREEN, bitmap.getColor(1, 1));
  EXPECT_EQ(SK_ColorBLUE, bitmap.getColor(0, 2));
  EXPECT_EQ(SK_ColorBLUE, bitmap.getColor(1, 2));
}

}  // namespace content
