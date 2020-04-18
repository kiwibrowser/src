// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <array>

#include "base/command_line.h"
#include "base/macros.h"
#include "base/path_service.h"
#include "base/strings/string_number_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/build_config.h"
#include "chrome/browser/extensions/api/desktop_capture/desktop_capture_api.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/browser/media/webrtc/fake_desktop_media_list.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test_utils.h"
#include "extensions/common/switches.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "third_party/webrtc/modules/desktop_capture/desktop_capture_types.h"

using content::DesktopMediaID;
using content::WebContentsMediaCaptureId;

namespace extensions {

namespace {

struct TestFlags {
  bool expect_screens;
  bool expect_windows;
  bool expect_tabs;
  bool expect_audio;
  DesktopMediaID selected_source;
  bool cancelled;

  // Following flags are set by FakeDesktopMediaPicker when it's created and
  // deleted.
  bool picker_created;
  bool picker_deleted;
};

// TODO(crbug.com/805145): Uncomment this when test is re-enabled.
#if 0
DesktopMediaID MakeFakeWebContentsMediaId(bool audio_share) {
  DesktopMediaID media_id(DesktopMediaID::TYPE_WEB_CONTENTS,
                          DesktopMediaID::kNullId,
                          WebContentsMediaCaptureId(DesktopMediaID::kFakeId,
                                                    DesktopMediaID::kFakeId));
  media_id.audio_share = audio_share;
  return media_id;
}
#endif

class FakeDesktopMediaPicker : public DesktopMediaPicker {
 public:
  explicit FakeDesktopMediaPicker(TestFlags* expectation)
      : expectation_(expectation),
        weak_factory_(this) {
    expectation_->picker_created = true;
  }
  ~FakeDesktopMediaPicker() override { expectation_->picker_deleted = true; }

  // DesktopMediaPicker interface.
  void Show(const DesktopMediaPicker::Params& params,
            std::vector<std::unique_ptr<DesktopMediaList>> source_lists,
            const DoneCallback& done_callback) override {
    bool show_screens = false;
    bool show_windows = false;
    bool show_tabs = false;

    for (auto& source_list : source_lists) {
      switch (source_list->GetMediaListType()) {
        case DesktopMediaID::TYPE_NONE:
          break;
        case DesktopMediaID::TYPE_SCREEN:
          show_screens = true;
          break;
        case DesktopMediaID::TYPE_WINDOW:
          show_windows = true;
          break;
        case DesktopMediaID::TYPE_WEB_CONTENTS:
          show_tabs = true;
          break;
      }
    }
    EXPECT_EQ(expectation_->expect_screens, show_screens);
    EXPECT_EQ(expectation_->expect_windows, show_windows);
    EXPECT_EQ(expectation_->expect_tabs, show_tabs);
    EXPECT_EQ(expectation_->expect_audio, params.request_audio);
    EXPECT_EQ(params.modality, ui::ModalType::MODAL_TYPE_CHILD);

    if (!expectation_->cancelled) {
      // Post a task to call the callback asynchronously.
      base::ThreadTaskRunnerHandle::Get()->PostTask(
          FROM_HERE, base::BindOnce(&FakeDesktopMediaPicker::CallCallback,
                                    weak_factory_.GetWeakPtr(), done_callback));
    } else {
      // If we expect the dialog to be cancelled then store the callback to
      // retain reference to the callback handler.
      done_callback_ = done_callback;
    }
  }

 private:
  void CallCallback(DoneCallback done_callback) {
    done_callback.Run(expectation_->selected_source);
  }

  TestFlags* expectation_;
  DoneCallback done_callback_;

  base::WeakPtrFactory<FakeDesktopMediaPicker> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(FakeDesktopMediaPicker);
};

class FakeDesktopMediaPickerFactory :
    public DesktopCaptureChooseDesktopMediaFunction::PickerFactory {
 public:
  FakeDesktopMediaPickerFactory() {}
  ~FakeDesktopMediaPickerFactory() override {}

  void SetTestFlags(TestFlags* test_flags, int tests_count) {
    test_flags_ = test_flags;
    tests_count_ = tests_count;
    current_test_ = 0;
  }

  std::unique_ptr<DesktopMediaPicker> CreatePicker() override {
    EXPECT_LE(current_test_, tests_count_);
    if (current_test_ >= tests_count_)
      return std::unique_ptr<DesktopMediaPicker>();
    ++current_test_;
    return std::unique_ptr<DesktopMediaPicker>(
        new FakeDesktopMediaPicker(test_flags_ + current_test_ - 1));
  }

  std::unique_ptr<DesktopMediaList> CreateMediaList(
      DesktopMediaID::Type type) override {
    EXPECT_LE(current_test_, tests_count_);
    return std::unique_ptr<DesktopMediaList>(new FakeDesktopMediaList(type));
  }

 private:
  TestFlags* test_flags_;
  int tests_count_;
  int current_test_;

  DISALLOW_COPY_AND_ASSIGN(FakeDesktopMediaPickerFactory);
};

class DesktopCaptureApiTest : public ExtensionApiTest {
 public:
  DesktopCaptureApiTest() {
    DesktopCaptureChooseDesktopMediaFunction::
        SetPickerFactoryForTests(&picker_factory_);
  }
  ~DesktopCaptureApiTest() override {
    DesktopCaptureChooseDesktopMediaFunction::
        SetPickerFactoryForTests(NULL);
  }

  void SetUpOnMainThread() override {
    ExtensionApiTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
  }

 protected:
  GURL GetURLForPath(const std::string& host, const std::string& path) {
    std::string port = base::UintToString(embedded_test_server()->port());
    GURL::Replacements replacements;
    replacements.SetHostStr(host);
    replacements.SetPortStr(port);
    return embedded_test_server()->GetURL(path).ReplaceComponents(replacements);
  }

  FakeDesktopMediaPickerFactory picker_factory_;
};

}  // namespace

// Flaky on Windows: http://crbug.com/301887
// Fails on Chrome OS: http://crbug.com/718512
// Flaky on macOS: http://crbug.com/804897
#if defined(OS_WIN) || defined(OS_CHROMEOS) || defined(OS_MACOSX)
#define MAYBE_ChooseDesktopMedia DISABLED_ChooseDesktopMedia
#else
#define MAYBE_ChooseDesktopMedia ChooseDesktopMedia
#endif
IN_PROC_BROWSER_TEST_F(DesktopCaptureApiTest, MAYBE_ChooseDesktopMedia) {
  // Each element in the following array corresponds to one test in
  // chrome/test/data/extensions/api_test/desktop_capture/test.js .
  TestFlags test_flags[] = {
    // pickerUiCanceled()
    {true, true, false, false, DesktopMediaID()},
    // chooseMedia()
    {true, true, false, false,
     DesktopMediaID(DesktopMediaID::TYPE_SCREEN, DesktopMediaID::kNullId)},
    // screensOnly()
    {true, false, false, false, DesktopMediaID()},
    // WindowsOnly()
    {false, true, false, false, DesktopMediaID()},
    // tabOnly()
    {false, false, true, false, DesktopMediaID()},
    // audioShareNoApproval()
    {true, true, true, true,
     DesktopMediaID(DesktopMediaID::TYPE_WEB_CONTENTS, 123, false)},
    // audioShareApproval()
    {true, true, true, true,
     DesktopMediaID(DesktopMediaID::TYPE_WEB_CONTENTS, 123, true)},
    // chooseMediaAndGetStream()
    {true, true, false, false,
     DesktopMediaID(DesktopMediaID::TYPE_SCREEN, webrtc::kFullDesktopScreenId)},
    // chooseMediaAndTryGetStreamWithInvalidId()
    {true, true, false, false,
     DesktopMediaID(DesktopMediaID::TYPE_SCREEN, webrtc::kFullDesktopScreenId)},
    // cancelDialog()
    {true, true, false, false, DesktopMediaID(), true},
// TODO(crbug.com/805145): Test fails; invalid device IDs being generated.
#if 0
      // tabShareWithAudioGetStream()
      {false, false, true, true, MakeFakeWebContentsMediaId(true)},
#endif
    // windowShareWithAudioGetStream()
    {false, true, false, true,
     DesktopMediaID(DesktopMediaID::TYPE_WINDOW, DesktopMediaID::kFakeId,
                    true)},
    // screenShareWithAudioGetStream()
    {true, false, false, true,
     DesktopMediaID(DesktopMediaID::TYPE_SCREEN, webrtc::kFullDesktopScreenId,
                    true)},
// TODO(crbug.com/805145): Test fails; invalid device IDs being generated.
#if 0
      // tabShareWithoutAudioGetStream()
      {false, false, true, true, MakeFakeWebContentsMediaId(false)},
#endif
    // windowShareWithoutAudioGetStream()
    {false, true, false, true,
     DesktopMediaID(DesktopMediaID::TYPE_WINDOW, DesktopMediaID::kFakeId)},
    // screenShareWithoutAudioGetStream()
    {true, false, false, true,
     DesktopMediaID(DesktopMediaID::TYPE_SCREEN, webrtc::kFullDesktopScreenId)},
  };
  picker_factory_.SetTestFlags(test_flags, arraysize(test_flags));
  ASSERT_TRUE(RunExtensionTest("desktop_capture")) << message_;
}

// Test is flaky http://crbug.com/301887.
IN_PROC_BROWSER_TEST_F(DesktopCaptureApiTest, DISABLED_Delegation) {
  // Initialize test server.
  base::FilePath test_data;
  EXPECT_TRUE(base::PathService::Get(chrome::DIR_TEST_DATA, &test_data));
  embedded_test_server()->ServeFilesFromDirectory(test_data.AppendASCII(
      "extensions/api_test/desktop_capture_delegate"));
  ASSERT_TRUE(embedded_test_server()->Start());

  // Load extension.
  base::FilePath extension_path =
      test_data_dir_.AppendASCII("desktop_capture_delegate");
  const Extension* extension = LoadExtensionWithFlags(
      extension_path, ExtensionBrowserTest::kFlagNone);
  ASSERT_TRUE(extension);

  ui_test_utils::NavigateToURL(
      browser(), GetURLForPath("example.com", "/example.com.html"));

  TestFlags test_flags[] = {
      {true, true, false, false,
       DesktopMediaID(DesktopMediaID::TYPE_SCREEN, DesktopMediaID::kNullId)},
      {true, true, false, false,
       DesktopMediaID(DesktopMediaID::TYPE_SCREEN, DesktopMediaID::kNullId)},
      {true, true, false, false,
       DesktopMediaID(DesktopMediaID::TYPE_SCREEN, DesktopMediaID::kNullId),
       true},
  };
  picker_factory_.SetTestFlags(test_flags, arraysize(test_flags));

  bool result;

  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();

  ASSERT_TRUE(content::ExecuteScriptAndExtractBool(
      web_contents, "getStream()", &result));
  EXPECT_TRUE(result);

  ASSERT_TRUE(content::ExecuteScriptAndExtractBool(
      web_contents, "getStreamWithInvalidId()", &result));
  EXPECT_TRUE(result);

  // Verify that the picker is closed once the tab is closed.
  content::WebContentsDestroyedWatcher destroyed_watcher(web_contents);
  ASSERT_TRUE(content::ExecuteScriptAndExtractBool(
      web_contents, "openPickerDialogAndReturn()", &result));
  EXPECT_TRUE(result);
  EXPECT_TRUE(test_flags[2].picker_created);
  EXPECT_FALSE(test_flags[2].picker_deleted);

  web_contents->Close();
  destroyed_watcher.Wait();
  EXPECT_TRUE(test_flags[2].picker_deleted);
}

}  // namespace extensions
