// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_COMMON_RENDER_MESSAGES_H_
#define CHROME_COMMON_RENDER_MESSAGES_H_

#include <stdint.h>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/strings/string16.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "chrome/common/browser_controls_state_param_traits.h"
#include "chrome/common/buildflags.h"
#include "chrome/common/web_application_info_provider_param_traits.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/offline_pages/buildflags/buildflags.h"
#include "content/public/common/webplugininfo.h"
#include "ipc/ipc_channel_handle.h"
#include "ipc/ipc_message_macros.h"
#include "ipc/ipc_platform_file.h"
#include "media/media_buildflags.h"
#include "ppapi/buildflags/buildflags.h"
#include "url/gurl.h"
#include "url/ipc/url_param_traits.h"
#include "url/origin.h"

// Singly-included section for enums and custom IPC traits.
#ifndef INTERNAL_CHROME_COMMON_RENDER_MESSAGES_H_
#define INTERNAL_CHROME_COMMON_RENDER_MESSAGES_H_


#endif  // INTERNAL_CHROME_COMMON_RENDER_MESSAGES_H_

#define IPC_MESSAGE_START ChromeMsgStart

//-----------------------------------------------------------------------------
// RenderView messages
// These are messages sent from the browser to the renderer process.

// Tells the render frame to load all blocked plugins with the given identifier.
IPC_MESSAGE_ROUTED1(ChromeViewMsg_LoadBlockedPlugins,
                    std::string /* identifier */)

// Tells the renderer whether or not a file system access has been allowed.
IPC_MESSAGE_ROUTED2(ChromeViewMsg_RequestFileSystemAccessAsyncResponse,
                    int  /* request_id */,
                    bool /* allowed */)

// JavaScript related messages -----------------------------------------------

#if BUILDFLAG(ENABLE_OFFLINE_PAGES)
// Message sent from the renderer to the browser to schedule to download the
// page at a later time.
IPC_MESSAGE_ROUTED0(ChromeViewHostMsg_DownloadPageLater)

// Message sent from the renderer to the browser to indicate if download button
// is being shown in error page.
IPC_MESSAGE_ROUTED1(ChromeViewHostMsg_SetIsShowingDownloadButtonInErrorPage,
                    bool /* showing download button */)
#endif

//-----------------------------------------------------------------------------
// Misc messages
// These are messages sent from the renderer to the browser process.

// Tells the browser that content in the current page was blocked due to the
// user's content settings.
IPC_MESSAGE_ROUTED2(ChromeViewHostMsg_ContentBlocked,
                    ContentSettingsType /* type of blocked content */,
                    base::string16 /* details on blocked content */)

// Sent by the renderer process to check whether access to web databases is
// granted by content settings.
IPC_SYNC_MESSAGE_CONTROL5_1(ChromeViewHostMsg_AllowDatabase,
                            int /* render_frame_id */,
                            GURL /* origin_url */,
                            GURL /* top origin url */,
                            base::string16 /* database name */,
                            base::string16 /* database display name */,
                            bool /* allowed */)

// Sent by the renderer process to check whether access to DOM Storage is
// granted by content settings.
IPC_SYNC_MESSAGE_CONTROL4_1(ChromeViewHostMsg_AllowDOMStorage,
                            int /* render_frame_id */,
                            GURL /* origin_url */,
                            GURL /* top origin url */,
                            bool /* if true local storage, otherwise session */,
                            bool /* allowed */)

// Sent by the renderer process to check whether access to FileSystem is
// granted by content settings.
IPC_SYNC_MESSAGE_CONTROL3_1(ChromeViewHostMsg_RequestFileSystemAccessSync,
                            int /* render_frame_id */,
                            GURL /* origin_url */,
                            GURL /* top origin url */,
                            bool /* allowed */)

// Sent by the renderer process to check whether access to FileSystem is
// granted by content settings.
IPC_MESSAGE_CONTROL4(ChromeViewHostMsg_RequestFileSystemAccessAsync,
                    int /* render_frame_id */,
                    int /* request_id */,
                    GURL /* origin_url */,
                    GURL /* top origin url */)

// Sent by the renderer process to check whether access to Indexed DBis
// granted by content settings.
IPC_SYNC_MESSAGE_CONTROL4_1(ChromeViewHostMsg_AllowIndexedDB,
                            int /* render_frame_id */,
                            GURL /* origin_url */,
                            GURL /* top origin url */,
                            base::string16 /* database name */,
                            bool /* allowed */)

#if BUILDFLAG(ENABLE_PLUGINS)
// Sent by the renderer to check if crash reporting is enabled.
IPC_SYNC_MESSAGE_CONTROL0_1(ChromeViewHostMsg_IsCrashReportingEnabled,
                            bool /* enabled */)
#endif

// Tells the browser to open a PDF file in a new tab. Used when no PDF Viewer is
// available, and user clicks to view PDF.
IPC_MESSAGE_ROUTED1(ChromeViewHostMsg_OpenPDF, GURL /* url */)

#endif  // CHROME_COMMON_RENDER_MESSAGES_H_
