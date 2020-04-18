// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/frame/intervention.h"

#include "services/service_manager/public/cpp/connector.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/reporting.mojom-blink.h"
#include "third_party/blink/renderer/core/frame/frame_console.h"
#include "third_party/blink/renderer/core/frame/intervention_report.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/local_frame_client.h"
#include "third_party/blink/renderer/core/frame/report.h"
#include "third_party/blink/renderer/core/frame/reporting_context.h"
#include "third_party/blink/renderer/core/inspector/console_message.h"

namespace blink {

// static
void Intervention::GenerateReport(const LocalFrame* frame,
                                  const String& message) {
  if (!frame)
    return;

  // Send the message to the console.
  frame->Console().AddMessage(ConsoleMessage::Create(
      kInterventionMessageSource, kErrorMessageLevel, message));

  if (!frame->Client())
    return;

  Document* document = frame->GetDocument();

  // Construct the intervention report.
  InterventionReport* body =
      new InterventionReport(message, SourceLocation::Capture());
  Report* report =
      new Report("intervention", document->Url().GetString(), body);

  // Send the intervention report to any ReportingObservers.
  ReportingContext* reporting_context = ReportingContext::From(document);
  if (reporting_context->ObserverExists())
    reporting_context->QueueReport(report);

  // Send the intervention report to the Reporting API.
  mojom::blink::ReportingServiceProxyPtr service;
  Platform* platform = Platform::Current();
  platform->GetConnector()->BindInterface(platform->GetBrowserServiceName(),
                                          &service);
  service->QueueInterventionReport(document->Url(), message, body->sourceFile(),
                                   body->lineNumber(), body->columnNumber());
}

}  // namespace blink
