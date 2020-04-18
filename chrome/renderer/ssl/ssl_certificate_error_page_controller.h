// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_RENDERER_SSL_SSL_CERTIFICATE_ERROR_PAGE_CONTROLLER_H_
#define CHROME_RENDERER_SSL_SSL_CERTIFICATE_ERROR_PAGE_CONTROLLER_H_

#include "base/memory/weak_ptr.h"
#include "components/security_interstitials/core/controller_client.h"
#include "gin/wrappable.h"

namespace content {
class RenderFrame;
}

// This class makes various helper functions available to interstitials
// when committed interstitials are on. It is bound to the JavaScript
// window.certificateErrorPageController object.
class SSLCertificateErrorPageController
    : public gin::Wrappable<SSLCertificateErrorPageController> {
 public:
  static gin::WrapperInfo kWrapperInfo;

  class Delegate {
   public:
    // Called when the interstitial calls any of the installed JS methods.
    // |command| describes the command sent by the interstitial.
    virtual void SendCommand(
        security_interstitials::SecurityInterstitialCommand command) = 0;

   protected:
    virtual ~Delegate();
  };

  // Will invoke methods on |delegate| in response to user actions taken on the
  // interstitial. May call delegate methods even after the page has been
  // navigated away from, so it is recommended consumers make sure the weak
  // pointers are destroyed in response to navigations.
  static void Install(content::RenderFrame* render_frame,
                      base::WeakPtr<Delegate> delegate);

 private:
  explicit SSLCertificateErrorPageController(base::WeakPtr<Delegate> delegate);
  ~SSLCertificateErrorPageController() override;

  void DontProceed();
  void Proceed();
  void ShowMoreSection();
  void OpenHelpCenter();
  void OpenDiagnostic();
  void Reload();
  void OpenDateSettings();
  void OpenLogin();
  void DoReport();
  void DontReport();
  void OpenReportingPrivacy();
  void OpenWhitepaper();
  void ReportPhishingError();

  void SendCommand(security_interstitials::SecurityInterstitialCommand command);

  // gin::WrappableBase
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;

  base::WeakPtr<Delegate> const delegate_;

  DISALLOW_COPY_AND_ASSIGN(SSLCertificateErrorPageController);
};

#endif  // CHROME_RENDERER_SSL_SSL_CERTIFICATE_ERROR_PAGE_CONTROLLER_H_
