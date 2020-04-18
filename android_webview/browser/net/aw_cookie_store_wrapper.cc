// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/browser/net/aw_cookie_store_wrapper.h"

#include <memory>
#include <string>

#include "android_webview/browser/net/init_native_callback.h"
#include "base/threading/thread_task_runner_handle.h"
#include "net/cookies/canonical_cookie.h"
#include "url/gurl.h"

namespace android_webview {

namespace {

void SetCookieWithOptionsAsyncOnCookieThread(
    const GURL& url,
    const std::string& cookie_line,
    const net::CookieOptions& options,
    net::CookieStore::SetCookiesCallback callback) {
  GetCookieStore()->SetCookieWithOptionsAsync(url, cookie_line, options,
                                              std::move(callback));
}

void SetCanonicalCookieAsyncOnCookieThread(
    std::unique_ptr<net::CanonicalCookie> cookie,
    bool secure_source,
    bool modify_http_only,
    net::CookieStore::SetCookiesCallback callback) {
  GetCookieStore()->SetCanonicalCookieAsync(
      std::move(cookie), secure_source, modify_http_only, std::move(callback));
}

void GetCookieListWithOptionsAsyncOnCookieThread(
    const GURL& url,
    const net::CookieOptions& options,
    net::CookieStore::GetCookieListCallback callback) {
  GetCookieStore()->GetCookieListWithOptionsAsync(url, options,
                                                  std::move(callback));
}

void GetAllCookiesAsyncOnCookieThread(
    net::CookieStore::GetCookieListCallback callback) {
  GetCookieStore()->GetAllCookiesAsync(std::move(callback));
}

void DeleteCookieAsyncOnCookieThread(const GURL& url,
                                     const std::string& cookie_name,
                                     base::OnceClosure callback) {
  GetCookieStore()->DeleteCookieAsync(url, cookie_name, std::move(callback));
}

void DeleteCanonicalCookieAsyncOnCookieThread(
    const net::CanonicalCookie& cookie,
    net::CookieStore::DeleteCallback callback) {
  GetCookieStore()->DeleteCanonicalCookieAsync(cookie, std::move(callback));
}

void DeleteAllCreatedWithinRangeAsyncOnCookieThread(
    const net::CookieDeletionInfo::TimeRange& creation_range,
    net::CookieStore::DeleteCallback callback) {
  GetCookieStore()->DeleteAllCreatedInTimeRangeAsync(creation_range,
                                                     std::move(callback));
}

void DeleteAllMatchingInfoAsyncOnCookieThread(
    net::CookieDeletionInfo delete_info,
    net::CookieStore::DeleteCallback callback) {
  GetCookieStore()->DeleteAllMatchingInfoAsync(std::move(delete_info),
                                               std::move(callback));
}

void DeleteSessionCookiesAsyncOnCookieThread(
    net::CookieStore::DeleteCallback callback) {
  GetCookieStore()->DeleteSessionCookiesAsync(std::move(callback));
}

void FlushStoreOnCookieThread(base::OnceClosure callback) {
  GetCookieStore()->FlushStore(std::move(callback));
}

void SetForceKeepSessionStateOnCookieThread() {
  GetCookieStore()->SetForceKeepSessionState();
}

}  // namespace

AwCookieStoreWrapper::AwCookieStoreWrapper()
    : client_task_runner_(base::ThreadTaskRunnerHandle::Get()),
      weak_factory_(this) {}

AwCookieStoreWrapper::~AwCookieStoreWrapper() {}

void AwCookieStoreWrapper::SetCookieWithOptionsAsync(
    const GURL& url,
    const std::string& cookie_line,
    const net::CookieOptions& options,
    net::CookieStore::SetCookiesCallback callback) {
  DCHECK(client_task_runner_->RunsTasksInCurrentSequence());
  PostTaskToCookieStoreTaskRunner(base::BindOnce(
      &SetCookieWithOptionsAsyncOnCookieThread, url, cookie_line, options,
      CreateWrappedCallback<bool>(std::move(callback))));
}

void AwCookieStoreWrapper::SetCanonicalCookieAsync(
    std::unique_ptr<net::CanonicalCookie> cookie,
    bool secure_source,
    bool modify_http_only,
    SetCookiesCallback callback) {
  DCHECK(client_task_runner_->RunsTasksInCurrentSequence());
  PostTaskToCookieStoreTaskRunner(base::BindOnce(
      &SetCanonicalCookieAsyncOnCookieThread, std::move(cookie), secure_source,
      modify_http_only, CreateWrappedCallback<bool>(std::move(callback))));
}

void AwCookieStoreWrapper::GetCookieListWithOptionsAsync(
    const GURL& url,
    const net::CookieOptions& options,
    GetCookieListCallback callback) {
  DCHECK(client_task_runner_->RunsTasksInCurrentSequence());
  PostTaskToCookieStoreTaskRunner(base::BindOnce(
      &GetCookieListWithOptionsAsyncOnCookieThread, url, options,
      CreateWrappedCallback<const net::CookieList&>(std::move(callback))));
}

void AwCookieStoreWrapper::GetAllCookiesAsync(GetCookieListCallback callback) {
  DCHECK(client_task_runner_->RunsTasksInCurrentSequence());
  PostTaskToCookieStoreTaskRunner(base::BindOnce(
      &GetAllCookiesAsyncOnCookieThread,
      CreateWrappedCallback<const net::CookieList&>(std::move(callback))));
}

void AwCookieStoreWrapper::DeleteCookieAsync(const GURL& url,
                                             const std::string& cookie_name,
                                             base::OnceClosure callback) {
  DCHECK(client_task_runner_->RunsTasksInCurrentSequence());
  PostTaskToCookieStoreTaskRunner(
      base::BindOnce(&DeleteCookieAsyncOnCookieThread, url, cookie_name,
                     CreateWrappedClosureCallback(std::move(callback))));
}

void AwCookieStoreWrapper::DeleteCanonicalCookieAsync(
    const net::CanonicalCookie& cookie,
    DeleteCallback callback) {
  DCHECK(client_task_runner_->RunsTasksInCurrentSequence());
  PostTaskToCookieStoreTaskRunner(
      base::BindOnce(&DeleteCanonicalCookieAsyncOnCookieThread, cookie,
                     CreateWrappedCallback<uint32_t>(std::move(callback))));
}

void AwCookieStoreWrapper::DeleteAllCreatedInTimeRangeAsync(
    const net::CookieDeletionInfo::TimeRange& creation_range,
    DeleteCallback callback) {
  DCHECK(client_task_runner_->RunsTasksInCurrentSequence());
  PostTaskToCookieStoreTaskRunner(base::BindOnce(
      &DeleteAllCreatedWithinRangeAsyncOnCookieThread, creation_range,
      CreateWrappedCallback<uint32_t>(std::move(callback))));
}

void AwCookieStoreWrapper::DeleteAllMatchingInfoAsync(
    net::CookieDeletionInfo delete_info,
    DeleteCallback callback) {
  DCHECK(client_task_runner_->RunsTasksInCurrentSequence());
  PostTaskToCookieStoreTaskRunner(base::BindOnce(
      &DeleteAllMatchingInfoAsyncOnCookieThread, std::move(delete_info),
      CreateWrappedCallback<uint32_t>(std::move(callback))));
}

void AwCookieStoreWrapper::DeleteSessionCookiesAsync(DeleteCallback callback) {
  DCHECK(client_task_runner_->RunsTasksInCurrentSequence());
  PostTaskToCookieStoreTaskRunner(
      base::BindOnce(&DeleteSessionCookiesAsyncOnCookieThread,
                     CreateWrappedCallback<uint32_t>(std::move(callback))));
}

void AwCookieStoreWrapper::FlushStore(base::OnceClosure callback) {
  DCHECK(client_task_runner_->RunsTasksInCurrentSequence());
  PostTaskToCookieStoreTaskRunner(
      base::BindOnce(&FlushStoreOnCookieThread,
                     CreateWrappedClosureCallback(std::move(callback))));
}

void AwCookieStoreWrapper::SetForceKeepSessionState() {
  DCHECK(client_task_runner_->RunsTasksInCurrentSequence());
  PostTaskToCookieStoreTaskRunner(
      base::BindOnce(&SetForceKeepSessionStateOnCookieThread));
}

net::CookieChangeDispatcher& AwCookieStoreWrapper::GetChangeDispatcher() {
  return change_dispatcher_;
}

bool AwCookieStoreWrapper::IsEphemeral() {
  return GetCookieStore()->IsEphemeral();
}

base::OnceClosure AwCookieStoreWrapper::CreateWrappedClosureCallback(
    base::OnceClosure callback) {
  if (callback.is_null())
    return callback;
  return base::BindOnce(
      base::IgnoreResult(&base::TaskRunner::PostTask), client_task_runner_,
      FROM_HERE,
      base::BindOnce(&AwCookieStoreWrapper::RunClosureCallback,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void AwCookieStoreWrapper::RunClosureCallback(base::OnceClosure callback) {
  DCHECK(client_task_runner_->RunsTasksInCurrentSequence());
  std::move(callback).Run();
}

}  // namespace android_webview
