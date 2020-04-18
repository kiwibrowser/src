// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_CERTIFICATE_DIALOGS_H_
#define CHROME_BROWSER_UI_CERTIFICATE_DIALOGS_H_

#include "net/cert/scoped_nss_types.h"
#include "net/cert/x509_certificate.h"
#include "ui/shell_dialogs/select_file_dialog.h"

namespace content {
class WebContents;
}

void ShowCertSelectFileDialog(ui::SelectFileDialog* select_file_dialog,
                              ui::SelectFileDialog::Type type,
                              const base::FilePath& suggested_path,
                              gfx::NativeWindow parent,
                              void* params);

// Show a dialog to save |cert| alone or the cert + its chain.
void ShowCertExportDialog(content::WebContents* web_contents,
                          gfx::NativeWindow parent,
                          const scoped_refptr<net::X509Certificate>& cert);

// Show a dialog to save the first certificate or the whole chain encompassed by
// the iterators.
void ShowCertExportDialog(content::WebContents* web_contents,
                          gfx::NativeWindow parent,
                          net::ScopedCERTCertificateList::iterator certs_begin,
                          net::ScopedCERTCertificateList::iterator certs_end);

#endif  // CHROME_BROWSER_UI_CERTIFICATE_DIALOGS_H_
