// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/network/onc/onc_certificate_importer_impl.h"

#include <cert.h>
#include <keyhi.h>
#include <pk11pub.h>
#include <stddef.h>

#include "base/base64.h"
#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/sequenced_task_runner.h"
#include "base/single_thread_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/values.h"
#include "chromeos/network/network_event_log.h"
#include "chromeos/network/onc/onc_utils.h"
#include "crypto/scoped_nss_types.h"
#include "net/base/net_errors.h"
#include "net/cert/nss_cert_database.h"
#include "net/cert/x509_util_nss.h"

namespace chromeos {
namespace onc {

namespace {

void CallBackOnOriginLoop(
    const scoped_refptr<base::SingleThreadTaskRunner>& origin_loop,
    CertificateImporter::DoneCallback callback,
    bool success,
    net::ScopedCERTCertificateList onc_trusted_certificates) {
  origin_loop->PostTask(FROM_HERE,
                        base::BindOnce(std::move(callback), success,
                                       std::move(onc_trusted_certificates)));
}

}  // namespace

CertificateImporterImpl::CertificateImporterImpl(
    const scoped_refptr<base::SequencedTaskRunner>& io_task_runner,
    net::NSSCertDatabase* target_nssdb)
    : io_task_runner_(io_task_runner),
      target_nssdb_(target_nssdb),
      weak_factory_(this) {
  CHECK(target_nssdb);
}

CertificateImporterImpl::~CertificateImporterImpl() = default;

void CertificateImporterImpl::ImportCertificates(
    std::unique_ptr<OncParsedCertificates> certificates,
    ::onc::ONCSource source,
    DoneCallback done_callback) {
  VLOG(2) << "ONC file has "
          << certificates->server_or_authority_certificates().size()
          << " server/authority certificates, "
          << certificates->client_certificates().size()
          << " client certificates";

  // |done_callback| must only be called as long as |this| still exists.
  // Thereforce, call back to |this|. This check of |this| must happen last and
  // on the origin thread.
  DoneCallback callback_to_this =
      base::BindOnce(&CertificateImporterImpl::RunDoneCallback,
                     weak_factory_.GetWeakPtr(), std::move(done_callback));

  // |done_callback| must be called on the origin thread.
  DoneCallback callback_on_origin_loop =
      base::BindOnce(&CallBackOnOriginLoop, base::ThreadTaskRunnerHandle::Get(),
                     std::move(callback_to_this));

  // This is the actual function that imports the certificates.
  base::OnceClosure import_certs_callback = base::BindOnce(
      &StoreCertificates, source, std::move(callback_on_origin_loop),
      std::move(certificates), target_nssdb_);

  // The NSSCertDatabase must be accessed on |io_task_runner_|
  io_task_runner_->PostTask(FROM_HERE, std::move(import_certs_callback));
}

// static
void CertificateImporterImpl::StoreCertificates(
    ::onc::ONCSource source,
    DoneCallback done_callback,
    std::unique_ptr<OncParsedCertificates> certificates,
    net::NSSCertDatabase* nssdb) {
  // Web trust is only granted to certificates imported by the user.
  bool allow_trust_imports = source == ::onc::ONC_SOURCE_USER_IMPORT;
  net::ScopedCERTCertificateList onc_trusted_certificates;

  // Even if the certificate parsing had an error at some point, we try to
  // import the certificates which could be parsed.
  bool success = !certificates->has_error();

  for (const OncParsedCertificates::ServerOrAuthorityCertificate&
           server_or_authority_cert :
       certificates->server_or_authority_certificates()) {
    if (!StoreServerOrCaCertificate(source, allow_trust_imports,
                                    server_or_authority_cert, nssdb,
                                    &onc_trusted_certificates)) {
      success = false;
    } else {
      VLOG(2) << "Successfully imported certificate with GUID "
              << server_or_authority_cert.guid();
    }
  }

  for (const OncParsedCertificates::ClientCertificate& client_cert :
       certificates->client_certificates()) {
    if (!StoreClientCertificate(client_cert, nssdb)) {
      success = false;
    } else {
      VLOG(2) << "Successfully imported certificate with GUID "
              << client_cert.guid();
    }
  }

  std::move(done_callback).Run(success, std::move(onc_trusted_certificates));
}

void CertificateImporterImpl::RunDoneCallback(
    DoneCallback callback,
    bool success,
    net::ScopedCERTCertificateList onc_trusted_certificates) {
  if (!success)
    NET_LOG_ERROR("ONC Certificate Import Error", "");
  std::move(callback).Run(success, std::move(onc_trusted_certificates));
}

bool CertificateImporterImpl::StoreServerOrCaCertificate(
    ::onc::ONCSource source,
    bool allow_trust_imports,
    const OncParsedCertificates::ServerOrAuthorityCertificate& certificate,
    net::NSSCertDatabase* nssdb,
    net::ScopedCERTCertificateList* onc_trusted_certificates) {
  // Device policy can't import certificates.
  if (source == ::onc::ONC_SOURCE_DEVICE_POLICY) {
    // This isn't a parsing error.
    LOG(WARNING) << "Refusing to import certificate from device policy: "
                 << certificate.guid();
    return true;
  }

  bool import_with_ssl_trust = false;
  if (certificate.web_trust_requested()) {
    if (!allow_trust_imports) {
      LOG(WARNING) << "Web trust not granted for certificate: "
                   << certificate.guid();
    } else {
      import_with_ssl_trust = true;
    }
  }

  net::ScopedCERTCertificate x509_cert =
      net::x509_util::CreateCERTCertificateFromX509Certificate(
          certificate.certificate().get());
  if (!x509_cert.get()) {
    LOG(ERROR) << "Unable to create certificate: " << certificate.guid();
    return false;
  }

  net::NSSCertDatabase::TrustBits trust = (import_with_ssl_trust ?
                                           net::NSSCertDatabase::TRUSTED_SSL :
                                           net::NSSCertDatabase::TRUST_DEFAULT);

  if (x509_cert.get()->isperm) {
    net::CertType net_cert_type =
        certificate.type() == OncParsedCertificates::
                                  ServerOrAuthorityCertificate::Type::kServer
            ? net::SERVER_CERT
            : net::CA_CERT;
    VLOG(1) << "Certificate is already installed.";
    net::NSSCertDatabase::TrustBits missing_trust_bits =
        trust & ~nssdb->GetCertTrust(x509_cert.get(), net_cert_type);
    if (missing_trust_bits) {
      std::string error_reason;
      bool success = false;
      if (nssdb->IsReadOnly(x509_cert.get())) {
        error_reason = " Certificate is stored read-only.";
      } else {
        success = nssdb->SetCertTrust(x509_cert.get(), net_cert_type, trust);
      }
      if (!success) {
        LOG(ERROR) << "Certificate " << certificate.guid()
                   << " was already present, but trust couldn't be set."
                   << error_reason;
      }
    }
  } else {
    net::ScopedCERTCertificateList cert_list;
    cert_list.push_back(net::x509_util::DupCERTCertificate(x509_cert.get()));
    net::NSSCertDatabase::ImportCertFailureList failures;
    bool success = false;
    if (certificate.type() ==
        OncParsedCertificates::ServerOrAuthorityCertificate::Type::kServer)
      success = nssdb->ImportServerCert(cert_list, trust, &failures);
    else  // Authority cert
      success = nssdb->ImportCACerts(cert_list, trust, &failures);

    if (!failures.empty()) {
      std::string error_string = net::ErrorToString(failures[0].net_error);
      LOG(ERROR) << "Error ( " << error_string << " ) importing certificate "
                 << certificate.guid();
      return false;
    }

    if (!success) {
      LOG(ERROR) << "Unknown error importing certificate "
                 << certificate.guid();
      return false;
    }
  }

  if (certificate.web_trust_requested() && onc_trusted_certificates)
    onc_trusted_certificates->push_back(std::move(x509_cert));

  return true;
}

bool CertificateImporterImpl::StoreClientCertificate(
    const OncParsedCertificates::ClientCertificate& certificate,
    net::NSSCertDatabase* nssdb) {
  // Since this has a private key, always use the private module.
  crypto::ScopedPK11Slot private_slot(nssdb->GetPrivateSlot());
  if (!private_slot)
    return false;

  net::ScopedCERTCertificateList imported_certs;

  int import_result =
      nssdb->ImportFromPKCS12(private_slot.get(), certificate.pkcs12_data(),
                              base::string16(), false, &imported_certs);
  if (import_result != net::OK) {
    std::string error_string = net::ErrorToString(import_result);
    LOG(ERROR) << "Unable to import client certificate with guid "
               << certificate.guid() << ", error: " << error_string;
    return false;
  }

  if (imported_certs.size() == 0) {
    LOG(WARNING) << "PKCS12 data contains no importable certificates for guid "
                 << certificate.guid();
    return true;
  }

  if (imported_certs.size() != 1) {
    LOG(WARNING) << "PKCS12 data for guid " << certificate.guid()
                 << " contains more than one certificate. "
                    "Only the first one will be imported.";
  }

  CERTCertificate* cert_result = imported_certs[0].get();

  // Find the private key associated with this certificate, and set the
  // nickname on it.
  SECKEYPrivateKey* private_key = PK11_FindPrivateKeyFromCert(
      cert_result->slot, cert_result, nullptr /* wincx */);
  if (private_key) {
    PK11_SetPrivateKeyNickname(private_key,
                               const_cast<char*>(certificate.guid().c_str()));
    SECKEY_DestroyPrivateKey(private_key);
  } else {
    LOG(WARNING) << "Unable to find private key for certificate "
                 << certificate.guid();
  }
  return true;
}

}  // namespace onc
}  // namespace chromeos
