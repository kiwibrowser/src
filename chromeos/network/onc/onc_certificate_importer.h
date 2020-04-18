// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_NETWORK_ONC_ONC_CERTIFICATE_IMPORTER_H_
#define CHROMEOS_NETWORK_ONC_ONC_CERTIFICATE_IMPORTER_H_

#include "base/callback_forward.h"
#include "base/macros.h"
#include "chromeos/chromeos_export.h"
#include "chromeos/network/onc/onc_parsed_certificates.h"
#include "components/onc/onc_constants.h"
#include "net/cert/scoped_nss_types.h"

namespace chromeos {
namespace onc {

class CHROMEOS_EXPORT CertificateImporter {
 public:
  typedef base::OnceCallback<void(
      bool success,
      net::ScopedCERTCertificateList onc_trusted_certificates)>
      DoneCallback;

  CertificateImporter() {}
  virtual ~CertificateImporter() {}

  // Import |certificates|.
  // Certificates are only imported with web trust for user imports. If the
  // "Remove" field of a certificate is enabled, then removes the certificate
  // from the store instead of importing.
  // When the import is completed, |done_callback| will be called with |success|
  // equal to true if all certificates were imported successfully.
  // |onc_trusted_certificates| will contain the list of certificates that
  // were imported and requested the TrustBit "Web".
  // Never calls |done_callback| after this importer is destructed.
  virtual void ImportCertificates(
      std::unique_ptr<OncParsedCertificates> certificates,
      ::onc::ONCSource source,
      DoneCallback done_callback) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(CertificateImporter);
};

}  // namespace onc
}  // namespace chromeos

#endif  // CHROMEOS_NETWORK_ONC_ONC_CERTIFICATE_IMPORTER_H_
