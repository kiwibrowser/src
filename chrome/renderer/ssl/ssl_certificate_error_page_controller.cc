// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/ssl/ssl_certificate_error_page_controller.h"

#include "components/security_interstitials/core/controller_client.h"
#include "content/public/renderer/render_frame.h"
#include "gin/handle.h"
#include "gin/object_template_builder.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_local_frame.h"

gin::WrapperInfo SSLCertificateErrorPageController::kWrapperInfo = {
    gin::kEmbedderNativeGin};

SSLCertificateErrorPageController::Delegate::~Delegate() {}

void SSLCertificateErrorPageController::Install(
    content::RenderFrame* render_frame,
    base::WeakPtr<Delegate> delegate) {
  v8::Isolate* isolate = blink::MainThreadIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context =
      render_frame->GetWebFrame()->MainWorldScriptContext();
  if (context.IsEmpty())
    return;

  v8::Context::Scope context_scope(context);

  gin::Handle<SSLCertificateErrorPageController> controller = gin::CreateHandle(
      isolate, new SSLCertificateErrorPageController(delegate));
  if (controller.IsEmpty())
    return;

  v8::Local<v8::Object> global = context->Global();
  global->Set(gin::StringToV8(isolate, "certificateErrorPageController"),
              controller.ToV8());
}

SSLCertificateErrorPageController::SSLCertificateErrorPageController(
    base::WeakPtr<Delegate> delegate)
    : delegate_(delegate) {}

SSLCertificateErrorPageController::~SSLCertificateErrorPageController() {}

void SSLCertificateErrorPageController::DontProceed() {
  SendCommand(
      security_interstitials::SecurityInterstitialCommand::CMD_DONT_PROCEED);
}

void SSLCertificateErrorPageController::Proceed() {
  SendCommand(security_interstitials::SecurityInterstitialCommand::CMD_PROCEED);
}

void SSLCertificateErrorPageController::ShowMoreSection() {
  SendCommand(security_interstitials::SecurityInterstitialCommand::
                  CMD_SHOW_MORE_SECTION);
}

void SSLCertificateErrorPageController::OpenHelpCenter() {
  SendCommand(security_interstitials::SecurityInterstitialCommand::
                  CMD_OPEN_HELP_CENTER);
}

void SSLCertificateErrorPageController::OpenDiagnostic() {
  SendCommand(
      security_interstitials::SecurityInterstitialCommand::CMD_OPEN_DIAGNOSTIC);
}

void SSLCertificateErrorPageController::Reload() {
  SendCommand(security_interstitials::SecurityInterstitialCommand::CMD_RELOAD);
}

void SSLCertificateErrorPageController::OpenDateSettings() {
  SendCommand(security_interstitials::SecurityInterstitialCommand::
                  CMD_OPEN_DATE_SETTINGS);
}

void SSLCertificateErrorPageController::OpenLogin() {
  SendCommand(
      security_interstitials::SecurityInterstitialCommand::CMD_OPEN_LOGIN);
}

void SSLCertificateErrorPageController::DoReport() {
  SendCommand(
      security_interstitials::SecurityInterstitialCommand::CMD_DO_REPORT);
}

void SSLCertificateErrorPageController::DontReport() {
  SendCommand(
      security_interstitials::SecurityInterstitialCommand::CMD_DONT_REPORT);
}

void SSLCertificateErrorPageController::OpenReportingPrivacy() {
  SendCommand(security_interstitials::SecurityInterstitialCommand::
                  CMD_OPEN_REPORTING_PRIVACY);
}

void SSLCertificateErrorPageController::OpenWhitepaper() {
  SendCommand(
      security_interstitials::SecurityInterstitialCommand::CMD_OPEN_WHITEPAPER);
}

void SSLCertificateErrorPageController::ReportPhishingError() {
  SendCommand(security_interstitials::SecurityInterstitialCommand::
                  CMD_REPORT_PHISHING_ERROR);
}

void SSLCertificateErrorPageController::SendCommand(
    security_interstitials::SecurityInterstitialCommand command) {
  if (delegate_) {
    delegate_->SendCommand(command);
  }
}

gin::ObjectTemplateBuilder
SSLCertificateErrorPageController::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin::Wrappable<SSLCertificateErrorPageController>::
      GetObjectTemplateBuilder(isolate)
          .SetMethod("dontProceed",
                     &SSLCertificateErrorPageController::DontProceed)
          .SetMethod("proceed", &SSLCertificateErrorPageController::Proceed)
          .SetMethod("showMoreSection",
                     &SSLCertificateErrorPageController::ShowMoreSection)
          .SetMethod("openHelpCenter",
                     &SSLCertificateErrorPageController::OpenHelpCenter)
          .SetMethod("openDiagnostic",
                     &SSLCertificateErrorPageController::OpenDiagnostic)
          .SetMethod("reload", &SSLCertificateErrorPageController::Reload)
          .SetMethod("openDateSettings",
                     &SSLCertificateErrorPageController::OpenDateSettings)
          .SetMethod("openLogin", &SSLCertificateErrorPageController::OpenLogin)
          .SetMethod("doReport", &SSLCertificateErrorPageController::DoReport)
          .SetMethod("dontReport",
                     &SSLCertificateErrorPageController::DontReport)
          .SetMethod("openReportingPrivacy",
                     &SSLCertificateErrorPageController::OpenReportingPrivacy)
          .SetMethod("openWhitepaper",
                     &SSLCertificateErrorPageController::OpenWhitepaper)
          .SetMethod("reportPhishingError",
                     &SSLCertificateErrorPageController::ReportPhishingError);
}
