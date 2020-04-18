// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/dom_storage/dom_storage_message_filter.h"

#include "base/auto_reset.h"
#include "base/bind.h"
#include "base/strings/nullable_string16.h"
#include "base/strings/utf_string_conversions.h"
#include "content/browser/bad_message.h"
#include "content/browser/dom_storage/dom_storage_area.h"
#include "content/browser/dom_storage/dom_storage_context_wrapper.h"
#include "content/browser/dom_storage/dom_storage_host.h"
#include "content/browser/dom_storage/dom_storage_namespace.h"
#include "content/browser/dom_storage/dom_storage_task_runner.h"
#include "content/common/dom_storage/dom_storage_messages.h"
#include "content/public/browser/browser_thread.h"
#include "url/gurl.h"

namespace content {

DOMStorageMessageFilter::DOMStorageMessageFilter(
    DOMStorageContextWrapper* context)
    : BrowserMessageFilter(DOMStorageMsgStart),
      context_(context->context()),
      connection_dispatching_message_for_(0) {
}

DOMStorageMessageFilter::~DOMStorageMessageFilter() {
  DCHECK(!host_.get());
}

void DOMStorageMessageFilter::InitializeInSequence() {
  DCHECK(!BrowserThread::CurrentlyOn(BrowserThread::IO));
  host_.reset(new DOMStorageHost(context_.get()));
  context_->AddEventObserver(this);
}

void DOMStorageMessageFilter::UninitializeInSequence() {
  // TODO(michaeln): Restore this DCHECK once crbug/166470 and crbug/164403
  // are resolved.
  // DCHECK(!BrowserThread::CurrentlyOn(BrowserThread::IO));
  context_->RemoveEventObserver(this);
  host_.reset();
}

void DOMStorageMessageFilter::OnFilterAdded(IPC::Channel* channel) {
  context_->task_runner()->PostShutdownBlockingTask(
      FROM_HERE, DOMStorageTaskRunner::PRIMARY_SEQUENCE,
      base::BindOnce(&DOMStorageMessageFilter::InitializeInSequence, this));
}

void DOMStorageMessageFilter::OnFilterRemoved() {
  context_->task_runner()->PostShutdownBlockingTask(
      FROM_HERE, DOMStorageTaskRunner::PRIMARY_SEQUENCE,
      base::BindOnce(&DOMStorageMessageFilter::UninitializeInSequence, this));
}

base::TaskRunner* DOMStorageMessageFilter::OverrideTaskRunnerForMessage(
    const IPC::Message& message) {
  if (IPC_MESSAGE_CLASS(message) == DOMStorageMsgStart)
    return context_->task_runner();
  return nullptr;
}

bool DOMStorageMessageFilter::OnMessageReceived(const IPC::Message& message) {
  if (IPC_MESSAGE_CLASS(message) != DOMStorageMsgStart)
    return false;
  DCHECK(!BrowserThread::CurrentlyOn(BrowserThread::IO));
  DCHECK(host_.get());

  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(DOMStorageMessageFilter, message)
    IPC_MESSAGE_HANDLER(DOMStorageHostMsg_OpenStorageArea, OnOpenStorageArea)
    IPC_MESSAGE_HANDLER(DOMStorageHostMsg_CloseStorageArea, OnCloseStorageArea)
    IPC_MESSAGE_HANDLER(DOMStorageHostMsg_LoadStorageArea, OnLoadStorageArea)
    IPC_MESSAGE_HANDLER(DOMStorageHostMsg_SetItem, OnSetItem)
    IPC_MESSAGE_HANDLER(DOMStorageHostMsg_RemoveItem, OnRemoveItem)
    IPC_MESSAGE_HANDLER(DOMStorageHostMsg_Clear, OnClear)
    IPC_MESSAGE_HANDLER(DOMStorageHostMsg_FlushMessages, OnFlushMessages)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void DOMStorageMessageFilter::OnOpenStorageArea(int connection_id,
                                                const std::string& namespace_id,
                                                const GURL& origin) {
  DCHECK(!BrowserThread::CurrentlyOn(BrowserThread::IO));
  base::Optional<bad_message::BadMessageReason> error = host_->OpenStorageArea(
      connection_id, namespace_id, url::Origin::Create(origin));
  if (error)
    bad_message::ReceivedBadMessage(this, error.value());
}

void DOMStorageMessageFilter::OnCloseStorageArea(int connection_id) {
  DCHECK(!BrowserThread::CurrentlyOn(BrowserThread::IO));
  host_->CloseStorageArea(connection_id);
}

void DOMStorageMessageFilter::OnLoadStorageArea(int connection_id,
                                                DOMStorageValuesMap* map) {
  DCHECK(!BrowserThread::CurrentlyOn(BrowserThread::IO));
  if (!host_->ExtractAreaValues(connection_id, map)) {
    bad_message::ReceivedBadMessage(this, bad_message::DSMF_LOAD_STORAGE);
    return;
  }
  Send(new DOMStorageMsg_AsyncOperationComplete(true));
}

void DOMStorageMessageFilter::OnSetItem(
    int connection_id,
    const base::string16& key,
    const base::string16& value,
    const base::NullableString16& client_old_value,
    const GURL& page_url) {
  DCHECK(!BrowserThread::CurrentlyOn(BrowserThread::IO));
  DCHECK_EQ(0, connection_dispatching_message_for_);
  base::AutoReset<int> auto_reset(&connection_dispatching_message_for_,
                            connection_id);
  bool success =
      host_->SetAreaItem(connection_id, key, value, client_old_value, page_url);
  Send(new DOMStorageMsg_AsyncOperationComplete(success));
}

void DOMStorageMessageFilter::OnRemoveItem(
    int connection_id,
    const base::string16& key,
    const base::NullableString16& client_old_value,
    const GURL& page_url) {
  DCHECK(!BrowserThread::CurrentlyOn(BrowserThread::IO));
  DCHECK_EQ(0, connection_dispatching_message_for_);
  base::AutoReset<int> auto_reset(&connection_dispatching_message_for_,
                            connection_id);
  host_->RemoveAreaItem(connection_id, key, client_old_value, page_url);
  Send(new DOMStorageMsg_AsyncOperationComplete(true));
}

void DOMStorageMessageFilter::OnClear(
    int connection_id, const GURL& page_url) {
  DCHECK(!BrowserThread::CurrentlyOn(BrowserThread::IO));
  DCHECK_EQ(0, connection_dispatching_message_for_);
  base::AutoReset<int> auto_reset(&connection_dispatching_message_for_,
                            connection_id);
  host_->ClearArea(connection_id, page_url);
  Send(new DOMStorageMsg_AsyncOperationComplete(true));
}

void DOMStorageMessageFilter::OnFlushMessages() {
  // Intentionally empty method body.
}

void DOMStorageMessageFilter::OnDOMStorageItemSet(
    const DOMStorageArea* area,
    const base::string16& key,
    const base::string16& new_value,
    const base::NullableString16& old_value,
    const GURL& page_url) {
  SendDOMStorageEvent(area, page_url,
                      base::NullableString16(key, false),
                      base::NullableString16(new_value, false),
                      old_value);
}

void DOMStorageMessageFilter::OnDOMStorageItemRemoved(
    const DOMStorageArea* area,
    const base::string16& key,
    const base::string16& old_value,
    const GURL& page_url) {
  SendDOMStorageEvent(area, page_url,
                      base::NullableString16(key, false),
                      base::NullableString16(),
                      base::NullableString16(old_value, false));
}

void DOMStorageMessageFilter::OnDOMStorageAreaCleared(
    const DOMStorageArea* area,
    const GURL& page_url) {
  SendDOMStorageEvent(area, page_url,
                      base::NullableString16(),
                      base::NullableString16(),
                      base::NullableString16());
}

void DOMStorageMessageFilter::SendDOMStorageEvent(
    const DOMStorageArea* area,
    const GURL& page_url,
    const base::NullableString16& key,
    const base::NullableString16& new_value,
    const base::NullableString16& old_value) {
  DCHECK(!BrowserThread::CurrentlyOn(BrowserThread::IO));
  // Only send mutation events to processes which have the area open.
  bool originated_in_process = connection_dispatching_message_for_ != 0;
  if (originated_in_process ||
      host_->HasAreaOpen(area->namespace_id(), area->origin())) {
    DOMStorageMsg_Event_Params params;
    params.origin = area->origin().GetURL();
    params.page_url = page_url;
    params.connection_id = connection_dispatching_message_for_;
    params.key = key;
    params.new_value = new_value;
    params.old_value = old_value;
    params.namespace_id = area->namespace_id();
    Send(new DOMStorageMsg_Event(params));
  }
}

}  // namespace content
