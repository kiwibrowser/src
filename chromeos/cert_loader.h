// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_CERT_LOADER_H_
#define CHROMEOS_CERT_LOADER_H_

#include <string>
#include <vector>

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/threading/thread_checker.h"
#include "chromeos/chromeos_export.h"
#include "net/cert/scoped_nss_types.h"

namespace net {
class NSSCertDatabase;
}

namespace chromeos {

// This class is responsible for loading certificates once the TPM is
// initialized. It is expected to be constructed on the UI thread and public
// methods should all be called from the UI thread.
// When certificates have been loaded (after login completes and tpm token is
// initialized), or the cert database changes, observers are called with
// OnCertificatesLoaded().
// This class supports using one or two cert databases. The expected usage is
// that CertLoader is used with a NSSCertDatabase backed by the system token
// before user sign-in, and additionally with a user-specific NSSCertDatabase
// after user sign-in. When both NSSCertDatabase are used, CertLoader combines
// certificates from both into |all_certs()|.
class CHROMEOS_EXPORT CertLoader {
 public:
  class Observer {
   public:
    // Called when the certificates, passed for convenience as |all_certs|,
    // have completed loading.
    virtual void OnCertificatesLoaded(
        const net::ScopedCERTCertificateList& all_certs) = 0;

   protected:
    virtual ~Observer() {}
  };

  // Sets the global instance. Must be called before any calls to Get().
  static void Initialize();

  // Destroys the global instance.
  static void Shutdown();

  // Gets the global instance. Initialize() must be called first.
  static CertLoader* Get();

  // Returns true if the global instance has been initialized.
  static bool IsInitialized();

  // Returns the PKCS#11 attribute CKA_ID for a certificate as an upper-case
  // hex string and sets |slot_id| to the id of the containing slot, or returns
  // an empty string and doesn't modify |slot_id| if the PKCS#11 id could not be
  // determined.
  static std::string GetPkcs11IdAndSlotForCert(CERTCertificate* cert,
                                               int* slot_id);

  // Sets the NSS cert database which CertLoader should use to access system
  // slot certificates. The CertLoader will _not_ take ownership of the database
  // - see comment on SetUserNSSDB. CertLoader supports working with only one
  // database or with both (system and user) databases.
  void SetSystemNSSDB(net::NSSCertDatabase* system_slot_database);

  // Sets the NSS cert database which CertLoader should use to access user slot
  // certificates. CertLoader understands the edge case that this database could
  // also give access to system slot certificates (e.g. for affiliated users).
  // The CertLoader will _not_ take the ownership of the database, but it
  // expects it to stay alive at least until the shutdown starts on the main
  // thread. This assumes that SetUserNSSDB and other methods directly using
  // |database_| are not called during shutdown. CertLoader supports working
  // with only one database or with both (system and user) databases.
  void SetUserNSSDB(net::NSSCertDatabase* user_database);

  void AddObserver(CertLoader::Observer* observer);
  void RemoveObserver(CertLoader::Observer* observer);

  // Returns true if |cert| is hardware backed. See also
  // ForceHardwareBackedForTesting().
  static bool IsCertificateHardwareBacked(CERTCertificate* cert);

  // Returns true when the certificate list has been requested but not loaded.
  // When two databases are in use (SetSystemNSSDB and SetUserNSSDB have both
  // been called), this returns true when at least one of them is currently
  // loading certificates.
  // Note that this method poses an exception in the CertLoader interface:
  // While most of CertLoader's interface treats the initial load of a second
  // database the same way as an update in the first database, this method does
  // not. The reason is that it's targeted at displaying a message in the GUI,
  // so the user knows that (more) certificates will be available soon.
  bool initial_load_of_any_database_running() const;

  // Returns true if any certificates have been loaded. If CertLoader uses a
  // system and a user NSS database, this returns true after the certificates
  // from the first (usually system) database have been loaded.
  bool initial_load_finished() const;

  // Returns true if certificates from a user NSS database have been loaded.
  bool user_cert_database_load_finished() const;

  // Returns all certificates. This will be empty until certificates_loaded() is
  // true.
  const net::ScopedCERTCertificateList& all_certs() const {
    DCHECK(thread_checker_.CalledOnValidThread());
    return all_certs_;
  }

  // Returns certificates from the system token. This will be empty until
  // certificates_loaded() is true.
  const net::ScopedCERTCertificateList& system_certs() const {
    DCHECK(thread_checker_.CalledOnValidThread());
    return system_certs_;
  }

  // Called in tests if |IsCertificateHardwareBacked()| should always return
  // true.
  static void ForceHardwareBackedForTesting();

 private:
  class CertCache;

  CertLoader();
  ~CertLoader();

  // Called by |system_cert_cache_| or |user_cert_cache| when these had an
  // update.
  void CacheUpdated();

  // Called if a certificate load task is finished.
  void UpdateCertificates(net::ScopedCERTCertificateList all_certs,
                          net::ScopedCERTCertificateList system_certs);

  void NotifyCertificatesLoaded();

  base::ObserverList<Observer> observers_;

  // Cache for certificates from the system-token NSSCertDatabase.
  std::unique_ptr<CertCache> system_cert_cache_;
  // Cache for certificates from the user-specific NSSCertDatabase.
  std::unique_ptr<CertCache> user_cert_cache_;

  // Cached certificates loaded from the database(s).
  net::ScopedCERTCertificateList all_certs_;

  // Cached certificates from system token.
  net::ScopedCERTCertificateList system_certs_;

  base::ThreadChecker thread_checker_;

  base::WeakPtrFactory<CertLoader> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(CertLoader);
};

}  // namespace chromeos

#endif  // CHROMEOS_CERT_LOADER_H_
