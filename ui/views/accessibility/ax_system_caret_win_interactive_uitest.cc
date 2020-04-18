// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <windows.h>
#include <oleacc.h>
#include <wrl/client.h>

#include "base/macros.h"
#include "base/path_service.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/scoped_variant.h"
#include "mojo/edk/embedder/embedder.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/aura/window.h"
#include "ui/aura/window_tree_host.h"
#include "ui/base/ime/input_method.h"
#include "ui/base/ime/text_edit_commands.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/ui_base_paths.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/gl/test/gl_surface_test_support.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/controls/textfield/textfield_test_api.h"
#include "ui/views/test/widget_test.h"
#include "ui/views/widget/widget.h"

namespace views {

namespace {

class AXSystemCaretWinTest : public test::WidgetTest {
 public:
  AXSystemCaretWinTest() : self_(CHILDID_SELF) {}
  ~AXSystemCaretWinTest() override {}

  void SetUp() override {
    mojo::edk::Init();
    gl::GLSurfaceTestSupport::InitializeOneOff();
    ui::RegisterPathProvider();
    base::FilePath ui_test_pak_path;
    ASSERT_TRUE(base::PathService::Get(ui::UI_TEST_PAK, &ui_test_pak_path));
    ui::ResourceBundle::InitSharedInstanceWithPakPath(ui_test_pak_path);
    test::WidgetTest::SetUp();

    widget_ = CreateNativeDesktopWidget();
    widget_->SetBounds(gfx::Rect(0, 0, 200, 200));
    textfield_ = new Textfield();
    textfield_->SetBounds(0, 0, 200, 20);
    textfield_->SetText(base::ASCIIToUTF16("Some text."));
    widget_->GetRootView()->AddChildView(textfield_);
    test::WidgetActivationWaiter waiter(widget_, true);
    widget_->Show();
    waiter.Wait();
    textfield_->RequestFocus();
    ASSERT_TRUE(widget_->IsActive());
    ASSERT_TRUE(textfield_->HasFocus());
    ASSERT_EQ(ui::TEXT_INPUT_TYPE_TEXT,
              widget_->GetInputMethod()->GetTextInputType());
  }

  void TearDown() override {
    widget_->CloseNow();
    test::WidgetTest::TearDown();
    ui::ResourceBundle::CleanupSharedInstance();
  }

 protected:
  Widget* widget_;
  Textfield* textfield_;
  base::win::ScopedVariant self_;

  DISALLOW_COPY_AND_ASSIGN(AXSystemCaretWinTest);
};

}  // namespace

TEST_F(AXSystemCaretWinTest, DISABLED_TestOnCaretBoundsChangeInTextField) {
  TextfieldTestApi textfield_test_api(textfield_);
  Microsoft::WRL::ComPtr<IAccessible> caret_accessible;
  gfx::NativeWindow native_window = widget_->GetNativeWindow();
  ASSERT_NE(nullptr, native_window);
  HWND hwnd = native_window->GetHost()->GetAcceleratedWidget();
  EXPECT_HRESULT_SUCCEEDED(AccessibleObjectFromWindow(
      hwnd, static_cast<DWORD>(OBJID_CARET), IID_IAccessible,
      reinterpret_cast<void**>(caret_accessible.GetAddressOf())));

  textfield_test_api.ExecuteTextEditCommand(
      ui::TextEditCommand::MOVE_TO_BEGINNING_OF_DOCUMENT);
  gfx::Point caret_position = textfield_test_api.GetCursorViewRect().origin();
  LONG x, y, width, height;
  EXPECT_EQ(S_OK,
            caret_accessible->accLocation(&x, &y, &width, &height, self_));
  EXPECT_EQ(caret_position.x(), x);
  EXPECT_EQ(caret_position.y(), y);
  EXPECT_EQ(1, width);

  textfield_test_api.ExecuteTextEditCommand(
      ui::TextEditCommand::MOVE_TO_END_OF_DOCUMENT);
  gfx::Point caret_position2 = textfield_test_api.GetCursorViewRect().origin();
  EXPECT_NE(caret_position, caret_position2);
  EXPECT_EQ(S_OK,
            caret_accessible->accLocation(&x, &y, &width, &height, self_));
  EXPECT_EQ(caret_position2.x(), x);
  EXPECT_EQ(caret_position2.y(), y);
  EXPECT_EQ(1, width);
}

TEST_F(AXSystemCaretWinTest, DISABLED_TestOnInputTypeChangeInTextField) {
  Microsoft::WRL::ComPtr<IAccessible> caret_accessible;
  gfx::NativeWindow native_window = widget_->GetNativeWindow();
  ASSERT_NE(nullptr, native_window);
  HWND hwnd = native_window->GetHost()->GetAcceleratedWidget();
  EXPECT_HRESULT_SUCCEEDED(AccessibleObjectFromWindow(
      hwnd, static_cast<DWORD>(OBJID_CARET), IID_IAccessible,
      reinterpret_cast<void**>(caret_accessible.GetAddressOf())));
  LONG x, y, width, height;
  EXPECT_EQ(S_OK,
            caret_accessible->accLocation(&x, &y, &width, &height, self_));

  textfield_->SetTextInputType(ui::TEXT_INPUT_TYPE_PASSWORD);
  // Caret object should still be valid.
  EXPECT_EQ(S_OK,
            caret_accessible->accLocation(&x, &y, &width, &height, self_));

  // Retrieving the caret again should also work.
  caret_accessible.Reset();
  EXPECT_HRESULT_SUCCEEDED(AccessibleObjectFromWindow(
      hwnd, static_cast<DWORD>(OBJID_CARET), IID_IAccessible,
      reinterpret_cast<void**>(caret_accessible.GetAddressOf())));
  LONG x2, y2, width2, height2;
  EXPECT_EQ(S_OK,
            caret_accessible->accLocation(&x2, &y2, &width2, &height2, self_));
  EXPECT_EQ(x, x2);
  EXPECT_EQ(y, y2);
  EXPECT_EQ(width, width2);
  EXPECT_EQ(height, height2);
}

TEST_F(AXSystemCaretWinTest, DISABLED_TestMovingWindow) {
  Microsoft::WRL::ComPtr<IAccessible> caret_accessible;
  gfx::NativeWindow native_window = widget_->GetNativeWindow();
  ASSERT_NE(nullptr, native_window);
  HWND hwnd = native_window->GetHost()->GetAcceleratedWidget();
  EXPECT_HRESULT_SUCCEEDED(AccessibleObjectFromWindow(
      hwnd, static_cast<DWORD>(OBJID_CARET), IID_IAccessible,
      reinterpret_cast<void**>(caret_accessible.GetAddressOf())));
  LONG x, y, width, height;
  EXPECT_EQ(S_OK,
            caret_accessible->accLocation(&x, &y, &width, &height, self_));

  widget_->SetBounds(gfx::Rect(100, 100, 500, 500));
  LONG x2, y2, width2, height2;
  caret_accessible.Reset();
  EXPECT_HRESULT_SUCCEEDED(AccessibleObjectFromWindow(
      hwnd, static_cast<DWORD>(OBJID_CARET), IID_IAccessible,
      reinterpret_cast<void**>(caret_accessible.GetAddressOf())));
  EXPECT_EQ(S_OK,
            caret_accessible->accLocation(&x2, &y2, &width2, &height2, self_));
  EXPECT_NE(x, x2);
  EXPECT_NE(y, y2);
  // The width and height of the caret shouldn't change.
  EXPECT_EQ(width, width2);
  EXPECT_EQ(height, height2);

  // Try maximizing the window.
  SendMessage(hwnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0);
  LONG x3, y3, width3, height3;
  EXPECT_HRESULT_FAILED(
      caret_accessible->accLocation(&x3, &y3, &width3, &height3, self_));
  caret_accessible.Reset();

  EXPECT_HRESULT_SUCCEEDED(AccessibleObjectFromWindow(
      hwnd, static_cast<DWORD>(OBJID_CARET), IID_IAccessible,
      reinterpret_cast<void**>(caret_accessible.GetAddressOf())));
  EXPECT_EQ(S_OK,
            caret_accessible->accLocation(&x3, &y3, &width3, &height3, self_));
  EXPECT_NE(x2, x3);
  EXPECT_NE(y2, y3);
  // The width and height of the caret shouldn't change.
  EXPECT_EQ(width, width3);
  EXPECT_EQ(height, height3);
}

}  // namespace views
