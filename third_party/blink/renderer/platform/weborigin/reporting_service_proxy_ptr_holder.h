// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_WEBORIGIN_REPORTING_SERVICE_PROXY_PTR_HOLDER_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_WEBORIGIN_REPORTING_SERVICE_PROXY_PTR_HOLDER_H_

#include "third_party/blink/public/platform/interface_provider.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/reporting.mojom-blink.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"

namespace blink {

class ReportingServiceProxyPtrHolder {
 public:
  ReportingServiceProxyPtrHolder() {
    Platform::Current()->GetInterfaceProvider()->GetInterface(
        mojo::MakeRequest(&reporting_service_proxy));
  }
  ~ReportingServiceProxyPtrHolder() = default;

  void QueueInterventionReport(const KURL& url,
                               const String& message,
                               const String& source_file,
                               int line_number,
                               int column_number) {
    if (reporting_service_proxy) {
      reporting_service_proxy->QueueInterventionReport(
          url, message ? message : "", source_file ? source_file : "",
          line_number, column_number);
    }
  }

  void QueueDeprecationReport(const KURL& url,
                              const String& id,
                              WTF::Time anticipatedRemoval,
                              const String& message,
                              const String& source_file,
                              int line_number,
                              int column_number) {
    if (reporting_service_proxy) {
      reporting_service_proxy->QueueDeprecationReport(
          url, id, anticipatedRemoval, message ? message : "",
          source_file ? source_file : "", line_number, column_number);
    }
  }

  void QueueCspViolationReport(
      const KURL& url,
      const String& group,
      const SecurityPolicyViolationEventInit& violation_data) {
    if (reporting_service_proxy) {
      reporting_service_proxy->QueueCspViolationReport(
          url, group ? group : "default",
          violation_data.documentURI() ? violation_data.documentURI() : "",
          violation_data.referrer() ? violation_data.referrer() : "",
          violation_data.violatedDirective()
              ? violation_data.violatedDirective()
              : "",
          violation_data.effectiveDirective()
              ? violation_data.effectiveDirective()
              : "",
          violation_data.originalPolicy() ? violation_data.originalPolicy()
                                          : "",
          violation_data.disposition() ? violation_data.disposition() : "",
          violation_data.blockedURI() ? violation_data.blockedURI() : "",
          violation_data.lineNumber(), violation_data.columnNumber(),
          violation_data.sourceFile() ? violation_data.sourceFile() : "",
          violation_data.statusCode(),
          violation_data.sample() ? violation_data.sample() : "");
    }
  }

 private:
  mojom::blink::ReportingServiceProxyPtr reporting_service_proxy;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_WEBORIGIN_REPORTING_SERVICE_PROXY_PTR_HOLDER_H_
