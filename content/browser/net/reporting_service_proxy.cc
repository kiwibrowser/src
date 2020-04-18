// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/net/reporting_service_proxy.h"

#include <memory>
#include <string>
#include <utility>

#include "base/memory/ref_counted.h"
#include "base/values.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/site_instance.h"
#include "content/public/browser/storage_partition.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "net/reporting/reporting_report.h"
#include "net/reporting/reporting_service.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"
#include "third_party/blink/public/platform/reporting.mojom.h"
#include "url/gurl.h"

namespace content {

namespace {

class ReportingServiceProxyImpl : public blink::mojom::ReportingServiceProxy {
 public:
  ReportingServiceProxyImpl(
      scoped_refptr<net::URLRequestContextGetter> request_context_getter)
      : request_context_getter_(std::move(request_context_getter)) {}

  // blink::mojom::ReportingServiceProxy:

  void QueueInterventionReport(const GURL& url,
                               const std::string& message,
                               const std::string& source_file,
                               int line_number,
                               int column_number) override {
    auto body = std::make_unique<base::DictionaryValue>();
    body->SetString("message", message);
    body->SetString("sourceFile", source_file);
    body->SetInteger("lineNumber", line_number);
    body->SetInteger("columnNumber", column_number);
    QueueReport(url, "default", "intervention", std::move(body));
  }

  void QueueDeprecationReport(const GURL& url,
                              const std::string& id,
                              base::Time anticipatedRemoval,
                              const std::string& message,
                              const std::string& source_file,
                              int line_number,
                              int column_number) override {
    auto body = std::make_unique<base::DictionaryValue>();
    body->SetString("id", id);
    if (anticipatedRemoval.is_null())
      body->SetDouble("anticipatedRemoval", anticipatedRemoval.ToDoubleT());
    body->SetString("message", message);
    body->SetString("sourceFile", source_file);
    body->SetInteger("lineNumber", line_number);
    body->SetInteger("columnNumber", column_number);
    QueueReport(url, "default", "deprecation", std::move(body));
  }

  void QueueCspViolationReport(const GURL& url,
                               const std::string& group,
                               const std::string& document_uri,
                               const std::string& referrer,
                               const std::string& violated_directive,
                               const std::string& effective_directive,
                               const std::string& original_policy,
                               const std::string& disposition,
                               const std::string& blocked_uri,
                               int line_number,
                               int column_number,
                               const std::string& source_file,
                               int status_code,
                               const std::string& script_sample) override {
    auto body = std::make_unique<base::DictionaryValue>();
    body->SetString("document-uri", document_uri);
    body->SetString("referrer", referrer);
    body->SetString("violated-directive", violated_directive);
    body->SetString("effective-directive", effective_directive);
    body->SetString("original-policy", original_policy);
    body->SetString("disposition", disposition);
    body->SetString("blocked-uri", blocked_uri);
    if (line_number)
      body->SetInteger("line-number", line_number);
    if (column_number)
      body->SetInteger("column-number", column_number);
    body->SetString("source-file", source_file);
    if (status_code)
      body->SetInteger("status-code", status_code);
    body->SetString("script-sample", script_sample);
    QueueReport(url, group, "csp", std::move(body));
  }

 private:
  void QueueReport(const GURL& url,
                   const std::string& group,
                   const std::string& type,
                   std::unique_ptr<base::Value> body) {
    net::URLRequestContext* request_context =
        request_context_getter_->GetURLRequestContext();
    if (!request_context) {
      net::ReportingReport::RecordReportDiscardedForNoURLRequestContext();
      return;
    }

    net::ReportingService* reporting_service =
        request_context->reporting_service();
    if (!reporting_service) {
      net::ReportingReport::RecordReportDiscardedForNoReportingService();
      return;
    }

    // Depth is only non-zero for NEL reports, and those can't come from the
    // renderer.
    reporting_service->QueueReport(url, group, type, std::move(body),
                                   /* depth= */ 0);
  }

  scoped_refptr<net::URLRequestContextGetter> request_context_getter_;
};

void CreateReportingServiceProxyOnNetworkTaskRunner(
    blink::mojom::ReportingServiceProxyRequest request,
    scoped_refptr<net::URLRequestContextGetter> request_context_getter) {
  mojo::MakeStrongBinding(std::make_unique<ReportingServiceProxyImpl>(
                              std::move(request_context_getter)),
                          std::move(request));
}

}  // namespace

// static
void CreateReportingServiceProxy(
    StoragePartition* storage_partition,
    blink::mojom::ReportingServiceProxyRequest request) {
  scoped_refptr<net::URLRequestContextGetter> request_context_getter(
      storage_partition->GetURLRequestContext());
  scoped_refptr<base::SingleThreadTaskRunner> network_task_runner(
      request_context_getter->GetNetworkTaskRunner());
  network_task_runner->PostTask(
      FROM_HERE,
      base::BindOnce(&CreateReportingServiceProxyOnNetworkTaskRunner,
                     std::move(request), std::move(request_context_getter)));
}

}  // namespace content
