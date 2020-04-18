// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CERTIFICATE_MANAGER_MODEL_H_
#define CHROME_BROWSER_CERTIFICATE_MANAGER_MODEL_H_

#include <map>
#include <memory>
#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/strings/string16.h"
#include "net/cert/nss_cert_database.h"
#include "net/cert/scoped_nss_types.h"
#include "net/ssl/client_cert_identity.h"

namespace chromeos {
class CertificateProvider;
}  // namespace chromeos

namespace content {
class BrowserContext;
class ResourceContext;
}  // namespace content

// CertificateManagerModel provides the data to be displayed in the certificate
// manager dialog, and processes changes from the view.
class CertificateManagerModel {
 public:
  // Map from the subject organization name to the list of certs from that
  // organization.  If a cert does not have an organization name, the
  // subject's CertPrincipal::GetDisplayName() value is used instead.
  typedef std::map<std::string, net::ScopedCERTCertificateList> OrgGroupingMap;

  typedef base::Callback<void(std::unique_ptr<CertificateManagerModel>)>
      CreationCallback;

  // Enumeration of the possible columns in the certificate manager tree view.
  enum Column {
    COL_SUBJECT_NAME,
    COL_CERTIFICATE_STORE,
    COL_SERIAL_NUMBER,
    COL_EXPIRES_ON,
  };

  class Observer {
   public:
    // Called to notify the view that the certificate list has been refreshed.
    // TODO(mattm): do a more granular updating strategy?  Maybe retrieve new
    // list of certs, diff against past list, and then notify of the changes?
    virtual void CertificatesRefreshed() = 0;
  };

  // Creates a CertificateManagerModel. The model will be passed to the callback
  // when it is ready. The caller must ensure the model does not outlive the
  // |browser_context|.
  static void Create(content::BrowserContext* browser_context,
                     Observer* observer,
                     const CreationCallback& callback);

  ~CertificateManagerModel();

  bool is_user_db_available() const { return is_user_db_available_; }
  bool is_tpm_available() const { return is_tpm_available_; }

  // Accessor for read-only access to the underlying NSSCertDatabase.
  const net::NSSCertDatabase* cert_db() const { return cert_db_; }

  // Trigger a refresh of the list of certs, unlock any slots if necessary.
  // Following this call, the observer CertificatesRefreshed method will be
  // called so the view can call FilterAndBuildOrgGroupingMap as necessary to
  // refresh its tree views.
  void Refresh();

  // Fill |map| with the certificates matching |filter_type|.
  void FilterAndBuildOrgGroupingMap(net::CertType filter_type,
                                    OrgGroupingMap* map) const;

  // Get the data to be displayed in |column| for the given |cert|.
  base::string16 GetColumnText(CERTCertificate* cert, Column column) const;

  // Import private keys and certificates from PKCS #12 encoded
  // |data|, using the given |password|. If |is_extractable| is false,
  // mark the private key as unextractable from the slot.
  // Returns a net error code on failure.
  int ImportFromPKCS12(PK11SlotInfo* slot_info, const std::string& data,
                       const base::string16& password, bool is_extractable);

  // Import user certificate from DER encoded |data|.
  // Returns a net error code on failure.
  int ImportUserCert(const std::string& data);

  // Import CA certificates.
  // Tries to import all the certificates given.  The root will be trusted
  // according to |trust_bits|.  Any certificates that could not be imported
  // will be listed in |not_imported|.
  // |trust_bits| should be a bit field of TRUST* values from NSSCertDatabase.
  // Returns false if there is an internal error, otherwise true is returned and
  // |not_imported| should be checked for any certificates that were not
  // imported.
  bool ImportCACerts(const net::ScopedCERTCertificateList& certificates,
                     net::NSSCertDatabase::TrustBits trust_bits,
                     net::NSSCertDatabase::ImportCertFailureList* not_imported);

  // Import server certificate.  The first cert should be the server cert.  Any
  // additional certs should be intermediate/CA certs and will be imported but
  // not given any trust.
  // Any certificates that could not be imported will be listed in
  // |not_imported|.
  // |trust_bits| can be set to explicitly trust or distrust the certificate, or
  // use TRUST_DEFAULT to inherit trust as normal.
  // Returns false if there is an internal error, otherwise true is returned and
  // |not_imported| should be checked for any certificates that were not
  // imported.
  bool ImportServerCert(
      const net::ScopedCERTCertificateList& certificates,
      net::NSSCertDatabase::TrustBits trust_bits,
      net::NSSCertDatabase::ImportCertFailureList* not_imported);

  // Set trust values for certificate.
  // |trust_bits| should be a bit field of TRUST* values from NSSCertDatabase.
  // Returns true on success or false on failure.
  bool SetCertTrust(CERTCertificate* cert,
                    net::CertType type,
                    net::NSSCertDatabase::TrustBits trust_bits);

  // Delete the cert.  Returns true on success.  |cert| is still valid when this
  // function returns.
  bool Delete(CERTCertificate* cert);

  // IsHardwareBacked returns true if |cert| is hardware backed.
  bool IsHardwareBacked(CERTCertificate* cert) const;

 private:
  CertificateManagerModel(
      net::NSSCertDatabase* nss_cert_database,
      bool is_user_db_available,
      bool is_tpm_available,
      Observer* observer,
      std::unique_ptr<chromeos::CertificateProvider>
          extension_certificate_provider);

  // Methods used during initialization, see the comment at the top of the .cc
  // file for details.
  static void DidGetCertDBOnUIThread(
      net::NSSCertDatabase* cert_db,
      bool is_user_db_available,
      bool is_tpm_available,
      CertificateManagerModel::Observer* observer,
      std::unique_ptr<chromeos::CertificateProvider>
          extension_certificate_provider,
      const CreationCallback& callback);
  static void DidGetCertDBOnIOThread(
      CertificateManagerModel::Observer* observer,
      std::unique_ptr<chromeos::CertificateProvider>
          extension_certificate_provider,
      const CreationCallback& callback,
      net::NSSCertDatabase* cert_db);
  static void GetCertDBOnIOThread(
      content::ResourceContext* context,
      CertificateManagerModel::Observer* observer,
      std::unique_ptr<chromeos::CertificateProvider>
          extension_certificate_provider,
      const CreationCallback& callback);

  // Callback used by Refresh() for when the cert slots have been unlocked.
  // This method does the actual refreshing.
  void RefreshSlotsUnlocked();

  // Callback used to refresh extension provided certificates. Refreshes UI.
  void RefreshExtensionCertificates(
      net::ClientCertIdentityList new_cert_identities);

  net::NSSCertDatabase* cert_db_;
  net::ScopedCERTCertificateList cert_list_;
  net::ScopedCERTCertificateList extension_cert_list_;
  // Whether the certificate database has a public slot associated with the
  // profile. If not set, importing certificates is not allowed with this model.
  bool is_user_db_available_;
  bool is_tpm_available_;

  // The observer to notify when certificate list is refreshed.
  Observer* observer_;

  // Certificate provider used to fetch extension provided certificates.
  std::unique_ptr<chromeos::CertificateProvider>
      extension_certificate_provider_;

  base::WeakPtrFactory<CertificateManagerModel> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(CertificateManagerModel);
};

#endif  // CHROME_BROWSER_CERTIFICATE_MANAGER_MODEL_H_
