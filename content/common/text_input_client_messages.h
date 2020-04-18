// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_TEXT_INPUT_CLIENT_MESSAGES_H_
#define CONTENT_COMMON_TEXT_INPUT_CLIENT_MESSAGES_H_

#include <stddef.h>

#include "build/build_config.h"
#include "ipc/ipc_message_macros.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/range/range.h"

#if defined(OS_MACOSX)
#include "content/common/mac/attributed_string_coder.h"
#endif

#define IPC_MESSAGE_START TextInputClientMsgStart
#undef IPC_MESSAGE_EXPORT
#define IPC_MESSAGE_EXPORT CONTENT_EXPORT

// Browser -> Renderer Messages ////////////////////////////////////////////////
// These messages are sent from the browser to the renderer. Each one has a
// corresponding reply message.
////////////////////////////////////////////////////////////////////////////////

// Tells the renderer to send back the character index for a point.
IPC_MESSAGE_ROUTED1(TextInputClientMsg_CharacterIndexForPoint,
                    gfx::Point)

// Tells the renderer to send back the rectangle for a given character range.
IPC_MESSAGE_ROUTED1(TextInputClientMsg_FirstRectForCharacterRange,
                    gfx::Range)

// Tells the renderer to send back the text fragment in a given range.
IPC_MESSAGE_ROUTED1(TextInputClientMsg_StringForRange,
                    gfx::Range)

// Tells the renderer to send back the word under the given point and its
// baseline point.
IPC_MESSAGE_ROUTED1(TextInputClientMsg_StringAtPoint, gfx::Point)

////////////////////////////////////////////////////////////////////////////////

// Renderer -> Browser Replies /////////////////////////////////////////////////
// These messages are sent in reply to the above messages.
////////////////////////////////////////////////////////////////////////////////

// Reply message for TextInputClientMsg_CharacterIndexForPoint.
IPC_MESSAGE_ROUTED1(TextInputClientReplyMsg_GotCharacterIndexForPoint,
                    uint32_t /* character index */)

// Reply message for TextInputClientMsg_FirstRectForCharacterRange.
IPC_MESSAGE_ROUTED1(TextInputClientReplyMsg_GotFirstRectForRange,
                    gfx::Rect /* frame rectangle */)

#if defined(OS_MACOSX)
// Reply message for TextInputClientMsg_StringForRange.
IPC_MESSAGE_ROUTED2(TextInputClientReplyMsg_GotStringForRange,
                    mac::AttributedStringCoder::EncodedString,
                    gfx::Point)

// Reply message for TextInputClientMsg_StringAtPoint
IPC_MESSAGE_ROUTED2(TextInputClientReplyMsg_GotStringAtPoint,
                    mac::AttributedStringCoder::EncodedString,
                    gfx::Point)
#endif  // defined(OS_MACOSX)

#endif  // CONTENT_COMMON_TEXT_INPUT_CLIENT_MESSAGES_H_
