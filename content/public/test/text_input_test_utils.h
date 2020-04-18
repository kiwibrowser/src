// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_TEST_TEXT_INPUT_TEST_UTILS_H_
#define CONTENT_PUBLIC_TEST_TEXT_INPUT_TEST_UTILS_H_

#include <string>
#include <vector>

#include "base/callback.h"
#include "base/strings/string16.h"
#include "ui/base/ime/text_input_mode.h"
#include "ui/base/ime/text_input_type.h"

#ifdef OS_MACOSX
#include "content/public/browser/browser_message_filter.h"
#endif

namespace ipc {
class Message;
}

namespace gfx {
class Range;
}

namespace ui {
struct ImeTextSpan;
}

namespace content {

class MessageLoopRunner;
class RenderFrameHost;
class RenderProcessHost;
class RenderWidgetHost;
class RenderWidgetHostView;
class RenderWidgetHostViewBase;
class WebContents;
struct TextInputState;

// Returns the |TextInputState.type| from the TextInputManager owned by
// |web_contents|.
ui::TextInputType GetTextInputTypeFromWebContents(WebContents* web_contents);

// This method returns true if |view| is registered in the TextInputManager that
// is owned by |web_contents|. If that is the case, the value of |type| will be
// the |TextInputState.type| corresponding to the |view|. Returns false if
// |view| is not registered.
bool GetTextInputTypeForView(WebContents* web_contents,
                             RenderWidgetHostView* view,
                             ui::TextInputType* type);

// This method returns the number of RenderWidgetHostViews which are currently
// registered with the TextInputManager that is owned by |web_contents|.
size_t GetRegisteredViewsCountFromTextInputManager(WebContents* web_contents);

// Returns the RWHV corresponding to the frame with a focused <input> within the
// given WebContents.
RenderWidgetHostView* GetActiveViewFromWebContents(WebContents* web_contents);

// This method will send a request for an immediate update on composition range
// from TextInputManager's active widget corresponding to the |web_contents|.
// This function will return false if the request is not successfully sent;
// either due to missing TextInputManager or lack of an active widget.
bool RequestCompositionInfoFromActiveWidget(WebContents* web_contents);

// Returns true if |frame| has a focused editable element.
bool DoesFrameHaveFocusedEditableElement(RenderFrameHost* frame);

// Sends a request to the RenderWidget corresponding to |rwh| to commit the
// given |text|.
void SendImeCommitTextToWidget(
    RenderWidgetHost* rwh,
    const base::string16& text,
    const std::vector<ui::ImeTextSpan>& ime_text_spans,
    const gfx::Range& replacement_range,
    int relative_cursor_pos);

// Sends a request to RenderWidget corresponding to |rwh| to set the given
// composition text and update the corresponding IME params.
void SendImeSetCompositionTextToWidget(
    RenderWidgetHost* rwh,
    const base::string16& text,
    const std::vector<ui::ImeTextSpan>& ime_text_spans,
    const gfx::Range& replacement_range,
    int selection_start,
    int selection_end);

// Immediately destroys the RenderWidgetHost corresponding to the local root
// which is identified by the given process ID and RenderFrameHost routing ID.
bool DestroyRenderWidgetHost(int32_t process_id, int32_t local_root_routing_id);

// This class provides the necessary API for accessing the state of and also
// observing the TextInputManager for WebContents.
class TextInputManagerTester {
 public:
  TextInputManagerTester(WebContents* web_contents);
  virtual ~TextInputManagerTester();

  // Sets a callback which is invoked when a RWHV calls UpdateTextInputState
  // on the TextInputManager which is being observed.
  void SetUpdateTextInputStateCalledCallback(const base::Closure& callback);

  // Sets a callback which is invoked when a RWHV calls SelectionBoundsChanged
  // on the TextInputManager which is being observed.
  void SetOnSelectionBoundsChangedCallback(const base::Closure& callback);

  // Sets a callback which is invoked when a RWHV calls
  // ImeCompositionRangeChanged on the TextInputManager that is being observed.
  void SetOnImeCompositionRangeChangedCallback(const base::Closure& callback);

  // Sets a callback which is invoked when a RWHV calls SelectionChanged on the
  // TextInputManager which is being observed.
  void SetOnTextSelectionChangedCallback(const base::Closure& callback);

  // Returns true if there is a focused <input> and populates |type| with
  // |TextInputState.type| of the TextInputManager.
  bool GetTextInputType(ui::TextInputType* type);

  // Returns true if there is a focused <input> and populates |value| with
  // |TextInputState.value| of the TextInputManager.
  bool GetTextInputValue(std::string* value);

  // Returns true if there is a focused <input> and populates |length| with the
  // length of the selected text range in the focused view.
  bool GetCurrentTextSelectionLength(size_t* length);

  // This method sets |output| to the last value of composition range length
  // reported by a renderer corresponding to WebContents. If no such update have
  // been received, the method will leave |output| untouched and returns false.
  // Returning true means an update has been received and the value of |output|
  // has been updated accordingly.
  bool GetLastCompositionRangeLength(uint32_t* output);

  // Returns the RenderWidgetHostView with a focused <input> element or nullptr
  // if none exists.
  const RenderWidgetHostView* GetActiveView();

  // Returns the RenderWidgetHostView which has most recently updated any of its
  // state (e.g., TextInputState or otherwise).
  RenderWidgetHostView* GetUpdatedView();

  // Returns true if a call to TextInputManager::UpdateTextInputState has led
  // to a change in TextInputState (since the time the observer has been
  // created).
  bool IsTextInputStateChanged();

 private:
  // The actual internal observer of the TextInputManager.
  class InternalObserver;

  std::unique_ptr<InternalObserver> observer_;

  DISALLOW_COPY_AND_ASSIGN(TextInputManagerTester);
};

// This class observes the lifetime of a RenderWidgetHostView.
class TestRenderWidgetHostViewDestructionObserver {
 public:
  TestRenderWidgetHostViewDestructionObserver(RenderWidgetHostView* view);
  virtual ~TestRenderWidgetHostViewDestructionObserver();

  // Waits for the RWHV which is being observed to get destroyed.
  void Wait();

 private:
  // The actual internal observer of RenderWidgetHostViewBase.
  class InternalObserver;

  std::unique_ptr<InternalObserver> observer_;

  DISALLOW_COPY_AND_ASSIGN(TestRenderWidgetHostViewDestructionObserver);
};

// Helper class to create TextInputState structs on the browser side and send it
// to the given RenderWidgetHostView. This class can be used for faking changes
// in TextInputState for testing on the browser side.
class TextInputStateSender {
 public:
  explicit TextInputStateSender(RenderWidgetHostView* view);
  virtual ~TextInputStateSender();

  void Send();

  void SetFromCurrentState();

  // The required setter methods. These setter methods can be used to call
  // RenderWidgetHostViewBase::TextInputStateChanged with fake, customized
  // TextInputState. Used for unit-testing on the browser side.
  void SetType(ui::TextInputType type);
  void SetMode(ui::TextInputMode mode);
  void SetFlags(int flags);
  void SetCanComposeInline(bool can_compose_inline);
  void SetShowImeIfNeeded(bool show_ime_if_needed);

 private:
  std::unique_ptr<TextInputState> text_input_state_;
  RenderWidgetHostViewBase* const view_;

  DISALLOW_COPY_AND_ASSIGN(TextInputStateSender);
};

// This class is intended to observe the InputMethod.
class TestInputMethodObserver {
 public:
  // static
  // Creates and returns a platform specific implementation of an
  // InputMethodObserver.
  static std::unique_ptr<TestInputMethodObserver> Create(
      WebContents* web_contents);

  virtual ~TestInputMethodObserver();

  virtual ui::TextInputType GetTextInputTypeFromClient() = 0;

  virtual void SetOnShowImeIfNeededCallback(const base::Closure& callback) = 0;

 protected:
  TestInputMethodObserver();
};

#ifdef OS_MACOSX
// The test message filter for TextInputClientMac incoming messages from the
// renderer.
// NOTE: This filter should be added to the intended RenderProcessHost before
// the actual TextInputClientMessageFilter, otherwise, the messages
// will be handled and will never receive this filter.
class TestTextInputClientMessageFilter : public BrowserMessageFilter {
 public:
  // Creates the filter and adds itself to |host|.
  TestTextInputClientMessageFilter(RenderProcessHost* host);

  // Wait until the IPC TextInputClientReplyMsg_GotStringForRange arrives.
  void WaitForStringFromRange();

  // BrowserMessageFilter overrides.
  bool OnMessageReceived(const IPC::Message& message) override;

  // Sets a callback for the string for range IPC arriving from the renderer.
  // The callback is invoked before that of TextInputClientMac and is handled on
  // UI thread.
  void SetStringForRangeCallback(const base::Closure& callback);

  RenderProcessHost* process() const { return host_; }
  std::string string_from_range() { return string_from_range_; }

 private:
  ~TestTextInputClientMessageFilter() override;
  RenderProcessHost* const host_;
  std::string string_from_range_;
  bool received_string_from_range_;
  base::Closure string_for_range_callback_;
  scoped_refptr<MessageLoopRunner> message_loop_runner_;

  DISALLOW_COPY_AND_ASSIGN(TestTextInputClientMessageFilter);
};

// Requests the |tab_view| for the definition of the word identified by the
// given selection range. |range| identifies a word in the focused
// RenderWidgetHost underneath |tab_view| which may be different than the
// RenderWidgetHost corresponding to |tab_view|.
void AskForLookUpDictionaryForRange(RenderWidgetHostView* tab_view,
                                    const gfx::Range& range);

// Returns the total count of NSWindows instances which belong to the currently
// running NSApplication.
size_t GetOpenNSWindowsCount();
#endif

}  // namespace content

#endif  // CONTENT_PUBLIC_TEST_TEXT_INPUT_TEST_UTILS_H_
