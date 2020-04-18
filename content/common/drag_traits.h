// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/drag_event_source_info.h"
#include "content/public/common/common_param_traits.h"
#include "content/public/common/drop_data.h"
#include "ipc/ipc_message_macros.h"
#include "third_party/blink/public/platform/web_drag_operation.h"
#include "ui/gfx/geometry/point.h"

#define IPC_MESSAGE_START DragMsgStart

IPC_ENUM_TRAITS(blink::WebDragOperation)  // Bitmask.
IPC_ENUM_TRAITS_MAX_VALUE(ui::DragDropTypes::DragEventSource,
                          ui::DragDropTypes::DRAG_EVENT_SOURCE_LAST)
IPC_ENUM_TRAITS_MAX_VALUE(content::DropData::Kind,
                          content::DropData::Kind::LAST)

IPC_STRUCT_TRAITS_BEGIN(ui::FileInfo)
  IPC_STRUCT_TRAITS_MEMBER(path)
  IPC_STRUCT_TRAITS_MEMBER(display_name)
IPC_STRUCT_TRAITS_END()

IPC_STRUCT_TRAITS_BEGIN(content::DropData)
  IPC_STRUCT_TRAITS_MEMBER(key_modifiers)
  IPC_STRUCT_TRAITS_MEMBER(url)
  IPC_STRUCT_TRAITS_MEMBER(url_title)
  IPC_STRUCT_TRAITS_MEMBER(download_metadata)
  IPC_STRUCT_TRAITS_MEMBER(referrer_policy)
  IPC_STRUCT_TRAITS_MEMBER(filenames)
  IPC_STRUCT_TRAITS_MEMBER(filesystem_id)
  IPC_STRUCT_TRAITS_MEMBER(file_system_files)
  IPC_STRUCT_TRAITS_MEMBER(text)
  IPC_STRUCT_TRAITS_MEMBER(html)
  IPC_STRUCT_TRAITS_MEMBER(html_base_url)
  IPC_STRUCT_TRAITS_MEMBER(file_contents)
  IPC_STRUCT_TRAITS_MEMBER(file_contents_source_url)
  IPC_STRUCT_TRAITS_MEMBER(file_contents_filename_extension)
  IPC_STRUCT_TRAITS_MEMBER(file_contents_content_disposition)
  IPC_STRUCT_TRAITS_MEMBER(custom_data)
IPC_STRUCT_TRAITS_END()

IPC_STRUCT_TRAITS_BEGIN(content::DropData::FileSystemFileInfo)
  IPC_STRUCT_TRAITS_MEMBER(url)
  IPC_STRUCT_TRAITS_MEMBER(size)
  IPC_STRUCT_TRAITS_MEMBER(filesystem_id)
IPC_STRUCT_TRAITS_END()

IPC_STRUCT_TRAITS_BEGIN(content::DropData::Metadata)
  IPC_STRUCT_TRAITS_MEMBER(kind)
  IPC_STRUCT_TRAITS_MEMBER(mime_type)
  IPC_STRUCT_TRAITS_MEMBER(filename)
  IPC_STRUCT_TRAITS_MEMBER(file_system_url)
IPC_STRUCT_TRAITS_END()

IPC_STRUCT_TRAITS_BEGIN(content::DragEventSourceInfo)
  IPC_STRUCT_TRAITS_MEMBER(event_location)
  IPC_STRUCT_TRAITS_MEMBER(event_source)
IPC_STRUCT_TRAITS_END()
