// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/web_database_observer_impl.h"

#include "base/bind.h"
#include "base/metrics/histogram_macros.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string16.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/renderer/storage_util.h"
#include "storage/common/database/database_identifier.h"
#include "third_party/blink/public/platform/web_security_origin.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/sqlite/sqlite3.h"

using blink::WebSecurityOrigin;
using blink::WebString;

namespace content {

namespace {

const int kResultHistogramSize = 50;
const int kCallsiteHistogramSize = 10;
const int kWebSQLSuccess = -1;

int DetermineHistogramResult(int websql_error, int sqlite_error) {
  // If we have a sqlite error, log it after trimming the extended bits.
  // There are 26 possible values, but we leave room for some new ones.
  if (sqlite_error)
    return std::min(sqlite_error & 0xff, 30);

  // Otherwise, websql_error may be an SQLExceptionCode, SQLErrorCode
  // or a DOMExceptionCode, or -1 for success.
  if (websql_error == kWebSQLSuccess)
    return 0;  // no error

  // SQLExceptionCode starts at 1000
  if (websql_error >= 1000)
    websql_error -= 1000;

  return std::min(websql_error + 30, kResultHistogramSize - 1);
}

#define UMA_HISTOGRAM_WEBSQL_RESULT(name, callsite, websql_error, sqlite_error) \
  do { \
    DCHECK(callsite < kCallsiteHistogramSize); \
    int result = DetermineHistogramResult(websql_error, sqlite_error); \
    UMA_HISTOGRAM_ENUMERATION("websql.Async." name, \
                              result, kResultHistogramSize); \
    if (result) { \
      UMA_HISTOGRAM_ENUMERATION("websql.Async." name ".ErrorSite", \
                                callsite, kCallsiteHistogramSize); \
    } \
  } while (0)


}  // namespace

WebDatabaseObserverImpl::WebDatabaseObserverImpl(
    scoped_refptr<blink::mojom::ThreadSafeWebDatabaseHostPtr> web_database_host)
    : web_database_host_(std::move(web_database_host)),
      open_connections_(new storage::DatabaseConnectionsWrapper),
      main_thread_task_runner_(base::ThreadTaskRunnerHandle::Get()) {
  DCHECK(main_thread_task_runner_);
}

WebDatabaseObserverImpl::~WebDatabaseObserverImpl() = default;

void WebDatabaseObserverImpl::DatabaseOpened(
    const WebSecurityOrigin& origin,
    const WebString& database_name,
    const WebString& database_display_name,
    unsigned long estimated_size) {
  open_connections_->AddOpenConnection(storage::GetIdentifierFromOrigin(origin),
                                       database_name.Utf16());
  GetWebDatabaseHost().Opened(origin, database_name.Utf16(),
                              database_display_name.Utf16(), estimated_size);
}

void WebDatabaseObserverImpl::DatabaseModified(const WebSecurityOrigin& origin,
                                               const WebString& database_name) {
  GetWebDatabaseHost().Modified(origin, database_name.Utf16());
}

void WebDatabaseObserverImpl::DatabaseClosed(const WebSecurityOrigin& origin,
                                             const WebString& database_name) {
  DCHECK(!main_thread_task_runner_->RunsTasksInCurrentSequence());
  GetWebDatabaseHost().Closed(origin, database_name.Utf16());
  open_connections_->RemoveOpenConnection(
      storage::GetIdentifierFromOrigin(origin), database_name.Utf16());
}

void WebDatabaseObserverImpl::ReportOpenDatabaseResult(
    const WebSecurityOrigin& origin,
    const WebString& database_name,
    int callsite,
    int websql_error,
    int sqlite_error,
    double call_time) {
  UMA_HISTOGRAM_WEBSQL_RESULT("OpenResult", callsite,
                              websql_error, sqlite_error);
  HandleSqliteError(origin, database_name, sqlite_error);

  if (websql_error == kWebSQLSuccess && sqlite_error == SQLITE_OK) {
    UMA_HISTOGRAM_TIMES("websql.Async.OpenTime.Success",
                        base::TimeDelta::FromSecondsD(call_time));
  } else {
    UMA_HISTOGRAM_TIMES("websql.Async.OpenTime.Error",
                        base::TimeDelta::FromSecondsD(call_time));
  }
}

void WebDatabaseObserverImpl::ReportChangeVersionResult(
    const WebSecurityOrigin& origin,
    const WebString& database_name,
    int callsite,
    int websql_error,
    int sqlite_error) {
  UMA_HISTOGRAM_WEBSQL_RESULT("ChangeVersionResult", callsite,
                              websql_error, sqlite_error);
  HandleSqliteError(origin, database_name, sqlite_error);
}

void WebDatabaseObserverImpl::ReportStartTransactionResult(
    const WebSecurityOrigin& origin,
    const WebString& database_name,
    int callsite,
    int websql_error,
    int sqlite_error) {
  UMA_HISTOGRAM_WEBSQL_RESULT("BeginResult", callsite,
                              websql_error, sqlite_error);
  HandleSqliteError(origin, database_name, sqlite_error);
}

void WebDatabaseObserverImpl::ReportCommitTransactionResult(
    const WebSecurityOrigin& origin,
    const WebString& database_name,
    int callsite,
    int websql_error,
    int sqlite_error) {
  UMA_HISTOGRAM_WEBSQL_RESULT("CommitResult", callsite,
                              websql_error, sqlite_error);
  HandleSqliteError(origin, database_name, sqlite_error);
}

void WebDatabaseObserverImpl::ReportExecuteStatementResult(
    const WebSecurityOrigin& origin,
    const WebString& database_name,
    int callsite,
    int websql_error,
    int sqlite_error) {
  UMA_HISTOGRAM_WEBSQL_RESULT("StatementResult", callsite,
                              websql_error, sqlite_error);
  HandleSqliteError(origin, database_name, sqlite_error);
}

void WebDatabaseObserverImpl::ReportVacuumDatabaseResult(
    const WebSecurityOrigin& origin,
    const WebString& database_name,
    int sqlite_error) {
  int result = DetermineHistogramResult(-1, sqlite_error);
  UMA_HISTOGRAM_ENUMERATION("websql.Async.VacuumResult",
                              result, kResultHistogramSize);
  HandleSqliteError(origin, database_name, sqlite_error);
}

bool WebDatabaseObserverImpl::WaitForAllDatabasesToClose(
    base::TimeDelta timeout) {
  DCHECK(main_thread_task_runner_->RunsTasksInCurrentSequence());
  return open_connections_->WaitForAllDatabasesToClose(timeout);
}

void WebDatabaseObserverImpl::HandleSqliteError(const WebSecurityOrigin& origin,
                                                const WebString& database_name,
                                                int error) {
  // We filter out errors which the backend doesn't act on to avoid
  // a unnecessary ipc traffic, this method can get called at a fairly
  // high frequency (per-sqlstatement).
  if (error == SQLITE_CORRUPT || error == SQLITE_NOTADB) {
    GetWebDatabaseHost().HandleSqliteError(origin, database_name.Utf16(),
                                           error);
  }
}

blink::mojom::WebDatabaseHost& WebDatabaseObserverImpl::GetWebDatabaseHost() {
  return **web_database_host_;
}

}  // namespace content
