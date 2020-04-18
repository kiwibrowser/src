// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "build/build_config.h"
#include "chrome/browser/certificate_viewer.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/webui/certificate_viewer_webui.h"
#include "chrome/browser/ui/webui/constrained_web_dialog_ui.h"
#include "chrome/common/url_constants.h"
#include "chrome/test/base/ui_test_utils.h"
#include "chrome/test/base/web_ui_browser_test.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/test/browser_test_utils.h"
#include "net/cert/x509_util_nss.h"
#include "net/test/test_certificate_data.h"

// Test framework for chrome/test/data/webui/certificate_viewer_dialog_test.js.
class CertificateViewerUITest : public WebUIBrowserTest {
 public:
  CertificateViewerUITest();
  ~CertificateViewerUITest() override;

 protected:
  void CreateCertViewerDialog();
  void ShowCertificateViewer();
#if defined(OS_CHROMEOS)
  void ShowModalCertificateViewer();
#endif
};


void CertificateViewerUITest::CreateCertViewerDialog() {

}

void CertificateViewerUITest::ShowCertificateViewer() {
  net::ScopedCERTCertificate google_cert(
      net::x509_util::CreateCERTCertificateFromBytes(google_der,
                                                     sizeof(google_der)));
  ASSERT_TRUE(google_cert);
  net::ScopedCERTCertificateList certs;
  certs.push_back(net::x509_util::DupCERTCertificate(google_cert.get()));

  ASSERT_TRUE(browser());
  ASSERT_TRUE(browser()->window());

  CertificateViewerDialog* dialog =
      new CertificateViewerDialog(std::move(certs));
  dialog->Show(browser()->tab_strip_model()->GetActiveWebContents(),
               browser()->window()->GetNativeWindow());
  content::WebContents* webui_webcontents =
      dialog->GetWebUI()->GetWebContents();
  content::WaitForLoadStop(webui_webcontents);
  content::WebUI* webui = webui_webcontents->GetWebUI();
  webui_webcontents->GetRenderViewHost()->SetWebUIProperty(
      "expectedUrl", chrome::kChromeUICertificateViewerURL);
  SetWebUIInstance(webui);
}

#if defined(OS_CHROMEOS)
void CertificateViewerUITest::ShowModalCertificateViewer() {
  net::ScopedCERTCertificate google_cert(
      net::x509_util::CreateCERTCertificateFromBytes(google_der,
                                                     sizeof(google_der)));
  ASSERT_TRUE(google_cert);
  net::ScopedCERTCertificateList certs;
  certs.push_back(net::x509_util::DupCERTCertificate(google_cert.get()));

  ASSERT_TRUE(browser());
  ASSERT_TRUE(browser()->window());

  CertificateViewerModalDialog* dialog =
      new CertificateViewerModalDialog(std::move(certs));
  dialog->Show(browser()->tab_strip_model()->GetActiveWebContents(),
               browser()->window()->GetNativeWindow());
  content::WebContents* webui_webcontents =
      dialog->GetWebUI()->GetWebContents();
  content::WaitForLoadStop(webui_webcontents);
  content::WebUI* webui = webui_webcontents->GetWebUI();
  webui_webcontents->GetRenderViewHost()->SetWebUIProperty(
      "expectedUrl", chrome::kChromeUICertificateViewerDialogURL);
  SetWebUIInstance(webui);
}
#endif
