// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_FILEAPI_WEBBLOB_MESSAGES_H_
#define CONTENT_COMMON_FILEAPI_WEBBLOB_MESSAGES_H_

// IPC messages for HTML5 Blob URLs

#include "content/common/content_export.h"
#include "ipc/ipc_message_macros.h"
#include "ipc/ipc_message_utils.h"
#include "ipc/ipc_param_traits.h"
#include "url/ipc/url_param_traits.h"

#undef IPC_MESSAGE_EXPORT
#define IPC_MESSAGE_EXPORT CONTENT_EXPORT
#define IPC_MESSAGE_START BlobMsgStart


// NOTE: This message is synchronous to ensure that the browser is aware of the
// UUID before the UUID is passed to another process. This protects against a
// race condition in which the browser could be asked about a UUID that doesn't
// yet exist from its perspective. See also https://goo.gl/bfdE64.
IPC_SYNC_MESSAGE_CONTROL2_0(BlobHostMsg_RegisterPublicURL,
                            GURL,
                            std::string /* uuid */)
IPC_MESSAGE_CONTROL1(BlobHostMsg_RevokePublicURL,
                     GURL)

#endif  // CONTENT_COMMON_FILEAPI_WEBBLOB_MESSAGES_H_
