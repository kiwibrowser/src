/*
 * Copyright (C) 2006, 2007, 2009, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2008, 2010 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2012, Samsung Electronics. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "third_party/blink/renderer/core/page/chrome_client.h"

#include <algorithm>
#include "third_party/blink/public/platform/web_screen_info.h"
#include "third_party/blink/renderer/core/core_initializer.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/core/frame/frame_console.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/inspector/console_message.h"
#include "third_party/blink/renderer/core/layout/hit_test_result.h"
#include "third_party/blink/renderer/core/page/frame_tree.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/core/page/scoped_page_pauser.h"
#include "third_party/blink/renderer/core/probe/core_probes.h"
#include "third_party/blink/renderer/platform/geometry/int_rect.h"
#include "third_party/blink/renderer/platform/network/network_hints.h"

namespace blink {

void ChromeClient::Trace(blink::Visitor* visitor) {
  visitor->Trace(last_mouse_over_node_);
  PlatformChromeClient::Trace(visitor);
}

void ChromeClient::InstallSupplements(LocalFrame& frame) {
  CoreInitializer::GetInstance().InstallSupplements(frame);
}

void ChromeClient::SetWindowRectWithAdjustment(const IntRect& pending_rect,
                                               LocalFrame& frame) {
  IntRect screen = GetScreenInfo().available_rect;
  IntRect window = pending_rect;

  IntSize minimum_size = MinimumWindowSize();
  IntSize size_for_constraining_move = minimum_size;
  // Let size 0 pass through, since that indicates default size, not minimum
  // size.
  if (window.Width()) {
    window.SetWidth(std::min(std::max(minimum_size.Width(), window.Width()),
                             screen.Width()));
    size_for_constraining_move.SetWidth(window.Width());
  }
  if (window.Height()) {
    window.SetHeight(std::min(std::max(minimum_size.Height(), window.Height()),
                              screen.Height()));
    size_for_constraining_move.SetHeight(window.Height());
  }

  // Constrain the window position within the valid screen area.
  window.SetX(
      std::max(screen.X(),
               std::min(window.X(),
                        screen.MaxX() - size_for_constraining_move.Width())));
  window.SetY(
      std::max(screen.Y(),
               std::min(window.Y(),
                        screen.MaxY() - size_for_constraining_move.Height())));
  SetWindowRect(window, frame);
}

bool ChromeClient::CanOpenModalIfDuringPageDismissal(
    Frame& main_frame,
    ChromeClient::DialogType dialog,
    const String& message) {
  for (Frame* frame = &main_frame; frame;
       frame = frame->Tree().TraverseNext()) {
    if (!frame->IsLocalFrame())
      continue;
    LocalFrame& local_frame = ToLocalFrame(*frame);
    Document::PageDismissalType dismissal =
        local_frame.GetDocument()->PageDismissalEventBeingDispatched();
    if (dismissal != Document::kNoDismissal) {
      return ShouldOpenModalDialogDuringPageDismissal(local_frame, dialog,
                                                      message, dismissal);
    }
  }
  return true;
}

template <typename Delegate>
static bool OpenJavaScriptDialog(LocalFrame* frame,
                                 const String& message,
                                 ChromeClient::DialogType dialog_type,
                                 const Delegate& delegate) {
  // Suspend pages in case the client method runs a new event loop that would
  // otherwise cause the load to continue while we're in the middle of
  // executing JavaScript.
  ScopedPagePauser pauser;
  probe::willRunJavaScriptDialog(frame);
  bool result = delegate();
  probe::didRunJavaScriptDialog(frame);
  return result;
}

bool ChromeClient::OpenBeforeUnloadConfirmPanel(const String& message,
                                                LocalFrame* frame,
                                                bool is_reload) {
  DCHECK(frame);
  return OpenJavaScriptDialog(
      frame, message, ChromeClient::kHTMLDialog, [this, frame, is_reload]() {
        return OpenBeforeUnloadConfirmPanelDelegate(frame, is_reload);
      });
}

bool ChromeClient::OpenJavaScriptAlert(LocalFrame* frame,
                                       const String& message) {
  DCHECK(frame);
  if (!CanOpenModalIfDuringPageDismissal(frame->Tree().Top(),
                                         ChromeClient::kAlertDialog, message))
    return false;
  return OpenJavaScriptDialog(
      frame, message, ChromeClient::kAlertDialog, [this, frame, &message]() {
        return OpenJavaScriptAlertDelegate(frame, message);
      });
}

bool ChromeClient::OpenJavaScriptConfirm(LocalFrame* frame,
                                         const String& message) {
  DCHECK(frame);
  if (!CanOpenModalIfDuringPageDismissal(frame->Tree().Top(),
                                         ChromeClient::kConfirmDialog, message))
    return false;
  return OpenJavaScriptDialog(
      frame, message, ChromeClient::kConfirmDialog, [this, frame, &message]() {
        return OpenJavaScriptConfirmDelegate(frame, message);
      });
}

bool ChromeClient::OpenJavaScriptPrompt(LocalFrame* frame,
                                        const String& prompt,
                                        const String& default_value,
                                        String& result) {
  DCHECK(frame);
  if (!CanOpenModalIfDuringPageDismissal(frame->Tree().Top(),
                                         ChromeClient::kPromptDialog, prompt))
    return false;
  return OpenJavaScriptDialog(
      frame, prompt, ChromeClient::kPromptDialog,
      [this, frame, &prompt, &default_value, &result]() {
        return OpenJavaScriptPromptDelegate(frame, prompt, default_value,
                                            result);
      });
}

void ChromeClient::MouseDidMoveOverElement(LocalFrame& frame,
                                           const HitTestResult& result) {
  if (!result.GetScrollbar() && result.InnerNode() &&
      result.InnerNode()->GetDocument().IsDNSPrefetchEnabled())
    PrefetchDNS(result.AbsoluteLinkURL().Host());

  ShowMouseOverURL(result);

  if (result.GetScrollbar())
    ClearToolTip(frame);
  else
    SetToolTip(frame, result);
}

void ChromeClient::SetToolTip(LocalFrame& frame, const HitTestResult& result) {
  // First priority is a tooltip for element with "title" attribute.
  TextDirection tool_tip_direction;
  String tool_tip = result.Title(tool_tip_direction);

  // Lastly, some elements provide default tooltip strings.  e.g. <input
  // type="file" multiple> shows a tooltip for the selected filenames.
  if (tool_tip.IsEmpty()) {
    if (Node* node = result.InnerNode()) {
      if (node->IsElementNode()) {
        tool_tip = ToElement(node)->DefaultToolTip();

        // FIXME: We should obtain text direction of tooltip from
        // ChromeClient or platform. As of October 2011, all client
        // implementations don't use text direction information for
        // ChromeClient::setToolTip. We'll work on tooltip text
        // direction during bidi cleanup in form inputs.
        tool_tip_direction = TextDirection::kLtr;
      }
    }
  }

  if (last_tool_tip_point_ == result.GetHitTestLocation().Point() &&
      last_tool_tip_text_ == tool_tip)
    return;

  // If a tooltip was displayed earlier, and mouse cursor moves over
  // a different node with the same tooltip text, make sure the previous
  // tooltip is unset, so that it does not get stuck positioned relative
  // to the previous node).
  // The ::setToolTip overload, which is be called down the road,
  // ensures a new tooltip to be displayed with the new context.
  if (result.InnerNodeOrImageMapImage() != last_mouse_over_node_ &&
      !last_tool_tip_text_.IsEmpty() && tool_tip == last_tool_tip_text_)
    ClearToolTip(frame);

  last_tool_tip_point_ = result.GetHitTestLocation().Point();
  last_tool_tip_text_ = tool_tip;
  last_mouse_over_node_ = result.InnerNodeOrImageMapImage();
  SetToolTip(frame, tool_tip, tool_tip_direction);
}

void ChromeClient::ClearToolTip(LocalFrame& frame) {
  // Do not check m_lastToolTip* and do not update them intentionally.
  // We don't want to show tooltips with same content after clearToolTip().
  SetToolTip(frame, String(), TextDirection::kLtr);
}

bool ChromeClient::Print(LocalFrame* frame) {
  if (!CanOpenModalIfDuringPageDismissal(*frame->GetPage()->MainFrame(),
                                         ChromeClient::kPrintDialog, "")) {
    return false;
  }

  if (frame->GetDocument()->IsSandboxed(kSandboxModals)) {
    UseCounter::Count(frame, WebFeature::kDialogInSandboxedContext);
    frame->Console().AddMessage(ConsoleMessage::Create(
        kSecurityMessageSource, kErrorMessageLevel,
        "Ignored call to 'print()'. The document is sandboxed, and the "
        "'allow-modals' keyword is not set."));
    return false;
  }

  // Suspend pages in case the client method runs a new event loop that would
  // otherwise cause the load to continue while we're in the middle of
  // executing JavaScript.
  ScopedPagePauser pauser;

  PrintDelegate(frame);
  return true;
}

}  // namespace blink
