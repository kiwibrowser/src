// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chromeos/cert_loader.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/task_scheduler/post_task.h"
#include "crypto/nss_util.h"
#include "crypto/scoped_nss_types.h"
#include "net/cert/cert_database.h"
#include "net/cert/nss_cert_database.h"
#include "net/cert/nss_cert_database_chromeos.h"
#include "net/cert/x509_util_nss.h"

namespace chromeos {

// Caches certificates from a NSSCertDatabase. Handles reloading of certificates
// on update notifications and provides status flags (loading / loaded).
// CertLoader can use multiple CertCaches to combine certificates from multiple
// sources.
class CertLoader::CertCache : public net::CertDatabase::Observer {
 public:
  explicit CertCache(base::RepeatingClosure certificates_updated_callback)
      : certificates_updated_callback_(certificates_updated_callback),
        weak_factory_(this) {}

  ~CertCache() override {
    net::CertDatabase::GetInstance()->RemoveObserver(this);
  }

  void SetNSSDB(net::NSSCertDatabase* nss_database) {
    CHECK(!nss_database_);
    nss_database_ = nss_database;

    // Start observing cert database for changes.
    // Observing net::CertDatabase is preferred over observing |nss_database_|
    // directly, as |nss_database_| observers receive only events generated
    // directly by |nss_database_|, so they may miss a few relevant ones.
    // TODO(tbarzic): Once singleton NSSCertDatabase is removed, investigate if
    // it would be OK to observe |nss_database_| directly; or change
    // NSSCertDatabase to send notification on all relevant changes.
    net::CertDatabase::GetInstance()->AddObserver(this);

    LoadCertificates();
  }

  net::NSSCertDatabase* nss_database() { return nss_database_; }

  // net::CertDatabase::Observer
  void OnCertDBChanged() override {
    VLOG(1) << "OnCertDBChanged";
    LoadCertificates();
  }

  const net::ScopedCERTCertificateList& cert_list() const { return cert_list_; }

  bool initial_load_running() const {
    return nss_database_ && !initial_load_finished_;
  }

  bool initial_load_finished() const { return initial_load_finished_; }

  // Returns true if the underlying NSSCertDatabase has access to the system
  // slot.
  bool has_system_certificates() const { return has_system_certificates_; }

 private:
  // Trigger a certificate load. If a certificate loading task is already in
  // progress, will start a reload once the current task is finished.
  void LoadCertificates() {
    CHECK(thread_checker_.CalledOnValidThread());
    VLOG(1) << "LoadCertificates: " << certificates_update_running_;

    if (certificates_update_running_) {
      certificates_update_required_ = true;
      return;
    }

    certificates_update_running_ = true;
    certificates_update_required_ = false;

    if (nss_database_) {
      has_system_certificates_ =
          static_cast<bool>(nss_database_->GetSystemSlot());
      nss_database_->ListCerts(base::Bind(&CertCache::UpdateCertificates,
                                          weak_factory_.GetWeakPtr()));
    }
  }

  // Called if a certificate load task is finished.
  void UpdateCertificates(net::ScopedCERTCertificateList cert_list) {
    CHECK(thread_checker_.CalledOnValidThread());
    DCHECK(certificates_update_running_);
    VLOG(1) << "UpdateCertificates: " << cert_list.size();

    // Ignore any existing certificates.
    cert_list_ = std::move(cert_list);

    initial_load_finished_ = true;
    certificates_updated_callback_.Run();

    certificates_update_running_ = false;
    if (certificates_update_required_)
      LoadCertificates();
  }

  // To be called when certificates have been updated.
  base::RepeatingClosure certificates_updated_callback_;

  bool has_system_certificates_ = false;

  // This is true after certificates have been loaded initially.
  bool initial_load_finished_ = false;
  // This is true if a notification about certificate DB changes arrived while
  // loading certificates and means that we will have to trigger another
  // certificates load after that.
  bool certificates_update_required_ = false;
  // This is true while certificates are being loaded.
  bool certificates_update_running_ = false;

  // The NSS certificate database from which the certificates should be loaded.
  net::NSSCertDatabase* nss_database_ = nullptr;

  // Cached Certificates loaded from the database.
  net::ScopedCERTCertificateList cert_list_;

  base::ThreadChecker thread_checker_;

  base::WeakPtrFactory<CertCache> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(CertCache);
};

namespace {

// Checks if |certificate| is on the given |slot|.
bool IsCertificateOnSlot(CERTCertificate* certificate, PK11SlotInfo* slot) {
  crypto::ScopedPK11SlotList slots_for_cert(
      PK11_GetAllSlotsForCert(certificate, nullptr));
  if (!slots_for_cert)
    return false;

  for (PK11SlotListElement* slot_element =
           PK11_GetFirstSafe(slots_for_cert.get());
       slot_element; slot_element = PK11_GetNextSafe(slots_for_cert.get(),
                                                     slot_element, PR_FALSE)) {
    if (slot_element->slot == slot) {
      // All previously visited elements have been freed by PK11_GetNextSafe,
      // but we're not calling that for the last one, so free it explicitly.
      // The slots_for_cert list itself will be freed because ScopedPK11SlotList
      // is a unique_ptr.
      PK11_FreeSlotListElement(slots_for_cert.get(), slot_element);
      return true;
    }
  }
  return false;
}

// Goes through all certificates in |all_certs| and copies those certificates
// which are on |system_slot| to a new list.
net::ScopedCERTCertificateList FilterSystemTokenCertificates(
    net::ScopedCERTCertificateList certs,
    crypto::ScopedPK11Slot system_slot) {
  VLOG(1) << "FilterSystemTokenCertificates";
  if (!system_slot)
    return net::ScopedCERTCertificateList();

  PK11SlotInfo* system_slot_ptr = system_slot.get();
  // Only keep certificates which are on the |system_slot|.
  certs.erase(
      std::remove_if(certs.begin(), certs.end(),
                     [system_slot_ptr](const net::ScopedCERTCertificate& cert) {
                       return !IsCertificateOnSlot(cert.get(), system_slot_ptr);
                     }),
      certs.end());
  return certs;
}

}  // namespace

static CertLoader* g_cert_loader = nullptr;
static bool g_force_hardware_backed_for_test = false;

// static
void CertLoader::Initialize() {
  CHECK(!g_cert_loader);
  g_cert_loader = new CertLoader();
}

// static
void CertLoader::Shutdown() {
  CHECK(g_cert_loader);
  delete g_cert_loader;
  g_cert_loader = nullptr;
}

// static
CertLoader* CertLoader::Get() {
  CHECK(g_cert_loader) << "CertLoader::Get() called before Initialize()";
  return g_cert_loader;
}

// static
bool CertLoader::IsInitialized() {
  return g_cert_loader;
}

CertLoader::CertLoader() : weak_factory_(this) {
  system_cert_cache_ = std::make_unique<CertCache>(
      base::BindRepeating(&CertLoader::CacheUpdated, base::Unretained(this)));
  user_cert_cache_ = std::make_unique<CertCache>(
      base::BindRepeating(&CertLoader::CacheUpdated, base::Unretained(this)));
}

CertLoader::~CertLoader() = default;

void CertLoader::SetSystemNSSDB(net::NSSCertDatabase* system_slot_database) {
  system_cert_cache_->SetNSSDB(system_slot_database);
}

void CertLoader::SetUserNSSDB(net::NSSCertDatabase* user_database) {
  user_cert_cache_->SetNSSDB(user_database);
}

void CertLoader::AddObserver(CertLoader::Observer* observer) {
  observers_.AddObserver(observer);
}

void CertLoader::RemoveObserver(CertLoader::Observer* observer) {
  observers_.RemoveObserver(observer);
}

// static
bool CertLoader::IsCertificateHardwareBacked(CERTCertificate* cert) {
  if (g_force_hardware_backed_for_test)
    return true;
  PK11SlotInfo* slot = cert->slot;
  return slot && PK11_IsHW(slot);
}

bool CertLoader::initial_load_of_any_database_running() const {
  return system_cert_cache_->initial_load_running() ||
         user_cert_cache_->initial_load_running();
}

bool CertLoader::initial_load_finished() const {
  return system_cert_cache_->initial_load_finished() ||
         user_cert_cache_->initial_load_finished();
}

bool CertLoader::user_cert_database_load_finished() const {
  return user_cert_cache_->initial_load_finished();
}

// static
void CertLoader::ForceHardwareBackedForTesting() {
  g_force_hardware_backed_for_test = true;
}

// static
//
// For background see this discussion on dev-tech-crypto.lists.mozilla.org:
// http://web.archiveorange.com/archive/v/6JJW7E40sypfZGtbkzxX
//
// NOTE: This function relies on the convention that the same PKCS#11 ID
// is shared between a certificate and its associated private and public
// keys.  I tried to implement this with PK11_GetLowLevelKeyIDForCert(),
// but that always returns NULL on Chrome OS for me.
std::string CertLoader::GetPkcs11IdAndSlotForCert(CERTCertificate* cert,
                                                  int* slot_id) {
  DCHECK(slot_id);

  SECKEYPrivateKey* priv_key = PK11_FindKeyByAnyCert(cert, nullptr /* wincx */);
  if (!priv_key)
    return std::string();

  *slot_id = static_cast<int>(PK11_GetSlotID(priv_key->pkcs11Slot));

  // Get the CKA_ID attribute for a key.
  SECItem* sec_item = PK11_GetLowLevelKeyIDForPrivateKey(priv_key);
  std::string pkcs11_id;
  if (sec_item) {
    pkcs11_id = base::HexEncode(sec_item->data, sec_item->len);
    SECITEM_FreeItem(sec_item, PR_TRUE);
  }
  SECKEY_DestroyPrivateKey(priv_key);

  return pkcs11_id;
}

void CertLoader::CacheUpdated() {
  DCHECK(thread_checker_.CalledOnValidThread());
  VLOG(1) << "CacheUpdated";

  // If user_cert_cache_ has access to system certificates and it has already
  // finished its initial load, it will contain system certificates which we can
  // filter.
  if (user_cert_cache_->initial_load_finished() &&
      user_cert_cache_->has_system_certificates()) {
    crypto::ScopedPK11Slot system_slot =
        user_cert_cache_->nss_database()->GetSystemSlot();
    DCHECK(system_slot);
    base::PostTaskWithTraitsAndReplyWithResult(
        FROM_HERE,
        {base::MayBlock(), base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN},
        base::BindOnce(&FilterSystemTokenCertificates,
                       net::x509_util::DupCERTCertificateList(
                           user_cert_cache_->cert_list()),
                       std::move(system_slot)),
        base::BindOnce(&CertLoader::UpdateCertificates,
                       weak_factory_.GetWeakPtr(),
                       net::x509_util::DupCERTCertificateList(
                           user_cert_cache_->cert_list())));
  } else {
    // The user's cert cache does not contain system certificates.
    net::ScopedCERTCertificateList system_certs =
        net::x509_util::DupCERTCertificateList(system_cert_cache_->cert_list());
    net::ScopedCERTCertificateList all_certs =
        net::x509_util::DupCERTCertificateList(user_cert_cache_->cert_list());
    all_certs.reserve(all_certs.size() + system_certs.size());
    for (const net::ScopedCERTCertificate& cert : system_certs)
      all_certs.push_back(net::x509_util::DupCERTCertificate(cert.get()));
    UpdateCertificates(std::move(all_certs), std::move(system_certs));
  }
}

void CertLoader::UpdateCertificates(
    net::ScopedCERTCertificateList all_certs,
    net::ScopedCERTCertificateList system_certs) {
  CHECK(thread_checker_.CalledOnValidThread());

  VLOG(1) << "UpdateCertificates: " << all_certs.size() << " ("
          << system_certs.size() << " on system slot)";

  // Ignore any existing certificates.
  all_certs_ = std::move(all_certs);
  system_certs_ = std::move(system_certs);

  NotifyCertificatesLoaded();
}

void CertLoader::NotifyCertificatesLoaded() {
  for (auto& observer : observers_)
    observer.OnCertificatesLoaded(all_certs_);
}

}  // namespace chromeos
