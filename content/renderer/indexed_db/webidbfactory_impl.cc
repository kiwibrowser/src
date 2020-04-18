// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/indexed_db/webidbfactory_impl.h"

#include "base/memory/ptr_util.h"
#include "content/renderer/indexed_db/indexed_db_callbacks_impl.h"
#include "content/renderer/indexed_db/indexed_db_database_callbacks_impl.h"
#include "content/renderer/storage_util.h"
#include "ipc/ipc_sync_channel.h"
#include "mojo/public/cpp/bindings/strong_associated_binding.h"
#include "third_party/blink/public/platform/web_security_origin.h"
#include "third_party/blink/public/platform/web_string.h"

using blink::WebIDBCallbacks;
using blink::WebIDBDatabase;
using blink::WebIDBDatabaseCallbacks;
using blink::WebSecurityOrigin;
using blink::WebString;
using indexed_db::mojom::CallbacksAssociatedPtrInfo;
using indexed_db::mojom::DatabaseCallbacksAssociatedPtrInfo;
using indexed_db::mojom::FactoryAssociatedPtr;

namespace content {

class WebIDBFactoryImpl::IOThreadHelper {
 public:
  IOThreadHelper(scoped_refptr<IPC::SyncMessageFilter> sync_message_filter);
  ~IOThreadHelper();

  FactoryAssociatedPtr& GetService();
  CallbacksAssociatedPtrInfo GetCallbacksProxy(
      std::unique_ptr<IndexedDBCallbacksImpl> callbacks);
  DatabaseCallbacksAssociatedPtrInfo GetDatabaseCallbacksProxy(
      std::unique_ptr<IndexedDBDatabaseCallbacksImpl> callbacks);

  void GetDatabaseNames(std::unique_ptr<IndexedDBCallbacksImpl> callbacks,
                        const url::Origin& origin);
  void Open(const base::string16& name,
            int64_t version,
            int64_t transaction_id,
            std::unique_ptr<IndexedDBCallbacksImpl> callbacks,
            std::unique_ptr<IndexedDBDatabaseCallbacksImpl> database_callbacks,
            const url::Origin& origin);
  void DeleteDatabase(const base::string16& name,
                      std::unique_ptr<IndexedDBCallbacksImpl> callbacks,
                      const url::Origin& origin,
                      bool force_close);

 private:
  scoped_refptr<IPC::SyncMessageFilter> sync_message_filter_;
  FactoryAssociatedPtr service_;

  DISALLOW_COPY_AND_ASSIGN(IOThreadHelper);
};

WebIDBFactoryImpl::WebIDBFactoryImpl(
    scoped_refptr<IPC::SyncMessageFilter> sync_message_filter,
    scoped_refptr<base::SingleThreadTaskRunner> io_runner)
    : io_helper_(new IOThreadHelper(std::move(sync_message_filter))),
      io_runner_(std::move(io_runner)) {}

WebIDBFactoryImpl::~WebIDBFactoryImpl() {
  io_runner_->DeleteSoon(FROM_HERE, io_helper_);
}

void WebIDBFactoryImpl::GetDatabaseNames(
    WebIDBCallbacks* callbacks,
    const WebSecurityOrigin& origin,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner) {
  auto callbacks_impl = std::make_unique<IndexedDBCallbacksImpl>(
      base::WrapUnique(callbacks), IndexedDBCallbacksImpl::kNoTransaction,
      nullptr, io_runner_, std::move(task_runner));
  io_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&IOThreadHelper::GetDatabaseNames,
                     base::Unretained(io_helper_), std::move(callbacks_impl),
                     url::Origin(origin)));
}

void WebIDBFactoryImpl::Open(
    const WebString& name,
    long long version,
    long long transaction_id,
    WebIDBCallbacks* callbacks,
    WebIDBDatabaseCallbacks* database_callbacks,
    const WebSecurityOrigin& origin,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner) {
  auto callbacks_impl = std::make_unique<IndexedDBCallbacksImpl>(
      base::WrapUnique(callbacks), transaction_id, nullptr, io_runner_,
      task_runner);
  auto database_callbacks_impl =
      std::make_unique<IndexedDBDatabaseCallbacksImpl>(
          base::WrapUnique(database_callbacks), std::move(task_runner));
  io_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&IOThreadHelper::Open, base::Unretained(io_helper_),
                     name.Utf16(), version, transaction_id,
                     std::move(callbacks_impl),
                     std::move(database_callbacks_impl), url::Origin(origin)));
}

void WebIDBFactoryImpl::DeleteDatabase(
    const WebString& name,
    WebIDBCallbacks* callbacks,
    const WebSecurityOrigin& origin,
    bool force_close,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner) {
  auto callbacks_impl = std::make_unique<IndexedDBCallbacksImpl>(
      base::WrapUnique(callbacks), IndexedDBCallbacksImpl::kNoTransaction,
      nullptr, io_runner_, std::move(task_runner));
  io_runner_->PostTask(
      FROM_HERE, base::BindOnce(&IOThreadHelper::DeleteDatabase,
                                base::Unretained(io_helper_), name.Utf16(),
                                std::move(callbacks_impl), url::Origin(origin),
                                force_close));
}

WebIDBFactoryImpl::IOThreadHelper::IOThreadHelper(
    scoped_refptr<IPC::SyncMessageFilter> sync_message_filter)
    : sync_message_filter_(std::move(sync_message_filter)) {}

WebIDBFactoryImpl::IOThreadHelper::~IOThreadHelper() {}

FactoryAssociatedPtr& WebIDBFactoryImpl::IOThreadHelper::GetService() {
  if (!service_)
    sync_message_filter_->GetRemoteAssociatedInterface(&service_);
  return service_;
}

CallbacksAssociatedPtrInfo WebIDBFactoryImpl::IOThreadHelper::GetCallbacksProxy(
    std::unique_ptr<IndexedDBCallbacksImpl> callbacks) {
  CallbacksAssociatedPtrInfo ptr_info;
  auto request = mojo::MakeRequest(&ptr_info);
  mojo::MakeStrongAssociatedBinding(std::move(callbacks), std::move(request));
  return ptr_info;
}

DatabaseCallbacksAssociatedPtrInfo
WebIDBFactoryImpl::IOThreadHelper::GetDatabaseCallbacksProxy(
    std::unique_ptr<IndexedDBDatabaseCallbacksImpl> callbacks) {
  DatabaseCallbacksAssociatedPtrInfo ptr_info;
  auto request = mojo::MakeRequest(&ptr_info);
  mojo::MakeStrongAssociatedBinding(std::move(callbacks), std::move(request));
  return ptr_info;
}

void WebIDBFactoryImpl::IOThreadHelper::GetDatabaseNames(
    std::unique_ptr<IndexedDBCallbacksImpl> callbacks,
    const url::Origin& origin) {
  GetService()->GetDatabaseNames(GetCallbacksProxy(std::move(callbacks)),
                                 origin);
}

void WebIDBFactoryImpl::IOThreadHelper::Open(
    const base::string16& name,
    int64_t version,
    int64_t transaction_id,
    std::unique_ptr<IndexedDBCallbacksImpl> callbacks,
    std::unique_ptr<IndexedDBDatabaseCallbacksImpl> database_callbacks,
    const url::Origin& origin) {
  GetService()->Open(GetCallbacksProxy(std::move(callbacks)),
                     GetDatabaseCallbacksProxy(std::move(database_callbacks)),
                     origin, name, version, transaction_id);
}

void WebIDBFactoryImpl::IOThreadHelper::DeleteDatabase(
    const base::string16& name,
    std::unique_ptr<IndexedDBCallbacksImpl> callbacks,
    const url::Origin& origin,
    bool force_close) {
  GetService()->DeleteDatabase(GetCallbacksProxy(std::move(callbacks)), origin,
                               name, force_close);
}

}  // namespace content
