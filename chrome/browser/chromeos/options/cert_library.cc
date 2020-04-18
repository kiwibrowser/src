// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/options/cert_library.h"

#include <algorithm>

#include "base/i18n/string_compare.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list_threadsafe.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/browser_process.h"  // g_browser_process
#include "chrome/grit/generated_resources.h"
#include "chromeos/dbus/cryptohome_client.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "chromeos/login/login_state.h"
#include "chromeos/network/certificate_helper.h"
#include "chromeos/network/onc/onc_utils.h"
#include "crypto/nss_util.h"
#include "net/cert/cert_database.h"
#include "net/cert/nss_cert_database.h"
#include "net/cert/x509_util_nss.h"
#include "third_party/icu/source/i18n/unicode/coll.h"  // icu::Collator
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/l10n/l10n_util_collator.h"

namespace chromeos {

namespace {

// Root CA certificates that are built into Chrome use this token name.
const char kRootCertificateTokenName[] = "Builtin Object Token";

base::string16 GetDisplayString(CERTCertificate* cert, bool hardware_backed) {
  base::string16 issued_by =
      base::UTF8ToUTF16(certificate::GetIssuerDisplayName(cert));

  base::string16 issued_to =
      base::UTF8ToUTF16(certificate::GetCertNameOrNickname(cert));
  base::string16 issued_to_ascii =
      base::UTF8ToUTF16(certificate::GetCertAsciiNameOrNickname(cert));
  if (issued_to_ascii != issued_to) {
    // Input contained encoded data, show original and decoded forms.
    issued_to = l10n_util::GetStringFUTF16(IDS_CERT_INFO_IDN_VALUE_FORMAT,
                                           issued_to_ascii, issued_to);
  }

  if (hardware_backed) {
    return l10n_util::GetStringFUTF16(
        IDS_CERT_MANAGER_HARDWARE_BACKED_KEY_FORMAT_LONG,
        issued_by,
        issued_to,
        l10n_util::GetStringUTF16(IDS_CERT_MANAGER_HARDWARE_BACKED));
  } else {
    return l10n_util::GetStringFUTF16(
        IDS_CERT_MANAGER_KEY_FORMAT_LONG,
        issued_by,
        issued_to);
  }
}

std::string CertToPEM(CERTCertificate* cert) {
  std::string pem_encoded_cert;
  if (!net::x509_util::GetPEMEncoded(cert, &pem_encoded_cert)) {
    LOG(ERROR) << "Couldn't PEM-encode certificate";
    return std::string();
  }
  return pem_encoded_cert;
}

}  // namespace

class CertNameComparator {
 public:
  explicit CertNameComparator(icu::Collator* collator)
      : collator_(collator) {
  }

  bool operator()(const net::ScopedCERTCertificate& lhs,
                  const net::ScopedCERTCertificate& rhs) const {
    base::string16 lhs_name = GetDisplayString(lhs.get(), false);
    base::string16 rhs_name = GetDisplayString(rhs.get(), false);
    if (collator_ == NULL)
      return lhs_name < rhs_name;
    return base::i18n::CompareString16WithCollator(*collator_, lhs_name,
                                                   rhs_name) == UCOL_LESS;
  }

 private:
  icu::Collator* collator_;
};

static CertLibrary* g_cert_library = NULL;

// static
void CertLibrary::Initialize() {
  CHECK(!g_cert_library);
  g_cert_library = new CertLibrary();
}

// static
void CertLibrary::Shutdown() {
  CHECK(g_cert_library);
  delete g_cert_library;
  g_cert_library = NULL;
}

// static
CertLibrary* CertLibrary::Get() {
  CHECK(g_cert_library) << "CertLibrary::Get() called before Initialize()";
  return g_cert_library;
}

// static
bool CertLibrary::IsInitialized() {
  return g_cert_library;
}

CertLibrary::CertLibrary() {
  CertLoader::Get()->AddObserver(this);
}

CertLibrary::~CertLibrary() {
  CertLoader::Get()->RemoveObserver(this);
}

void CertLibrary::AddObserver(CertLibrary::Observer* observer) {
  observer_list_.AddObserver(observer);
}

void CertLibrary::RemoveObserver(CertLibrary::Observer* observer) {
  observer_list_.RemoveObserver(observer);
}

bool CertLibrary::CertificatesLoading() const {
  return CertLoader::Get()->initial_load_of_any_database_running();
}

bool CertLibrary::CertificatesLoaded() const {
  return CertLoader::Get()->initial_load_finished();
}

int CertLibrary::NumCertificates(CertType type) const {
  const net::ScopedCERTCertificateList& cert_list =
      GetCertificateListForType(type);
  return static_cast<int>(cert_list.size());
}

base::string16 CertLibrary::GetCertDisplayStringAt(CertType type,
                                                   int index) const {
  CERTCertificate* cert = GetCertificateAt(type, index);
  bool hardware_backed = IsCertHardwareBackedAt(type, index);
  return GetDisplayString(cert, hardware_backed);
}

std::string CertLibrary::GetServerCACertPEMAt(int index) const {
  return CertToPEM(GetCertificateAt(CERT_TYPE_SERVER_CA, index));
}

std::string CertLibrary::GetUserCertPkcs11IdAt(int index, int* slot_id) const {
  CERTCertificate* cert = GetCertificateAt(CERT_TYPE_USER, index);
  return CertLoader::GetPkcs11IdAndSlotForCert(cert, slot_id);
}

bool CertLibrary::IsCertHardwareBackedAt(CertType type, int index) const {
  CERTCertificate* cert = GetCertificateAt(type, index);
  return CertLoader::IsCertificateHardwareBacked(cert);
}

int CertLibrary::GetServerCACertIndexByPEM(
    const std::string& pem_encoded) const {
  int num_certs = NumCertificates(CERT_TYPE_SERVER_CA);
  for (int index = 0; index < num_certs; ++index) {
    CERTCertificate* cert = GetCertificateAt(CERT_TYPE_SERVER_CA, index);
    if (CertToPEM(cert) != pem_encoded)
      continue;
    return index;
  }
  return -1;
}

int CertLibrary::GetUserCertIndexByPkcs11Id(
    const std::string& pkcs11_id) const {
  int num_certs = NumCertificates(CERT_TYPE_USER);
  for (int index = 0; index < num_certs; ++index) {
    CERTCertificate* cert = GetCertificateAt(CERT_TYPE_USER, index);
    int slot_id = -1;
    std::string id = CertLoader::GetPkcs11IdAndSlotForCert(cert, &slot_id);
    if (id == pkcs11_id)
      return index;
  }
  return -1;  // Not found.
}

void CertLibrary::OnCertificatesLoaded(
    const net::ScopedCERTCertificateList& cert_list) {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
  VLOG(1) << "CertLibrary::OnCertificatesLoaded: " << cert_list.size();
  certs_.clear();
  user_certs_.clear();
  server_certs_.clear();
  server_ca_certs_.clear();

  // Add certificates to the appropriate list.
  certs_.reserve(cert_list.size());
  for (const net::ScopedCERTCertificate& cert : cert_list) {
    certs_.push_back(net::x509_util::DupCERTCertificate(cert.get()));
    net::CertType type = certificate::GetCertType(cert.get());
    switch (type) {
      case net::USER_CERT:
        user_certs_.push_back(net::x509_util::DupCERTCertificate(cert.get()));
        break;
      case net::SERVER_CERT:
        server_certs_.push_back(net::x509_util::DupCERTCertificate(cert.get()));
        break;
      case net::CA_CERT: {
        // Exclude root CA certificates that are built into Chrome.
        std::string token_name = certificate::GetCertTokenName(cert.get());
        if (token_name != kRootCertificateTokenName)
          server_ca_certs_.push_back(
              net::x509_util::DupCERTCertificate(cert.get()));
        break;
      }
      default:
        break;
    }
  }

  // Perform locale-sensitive sorting by certificate name.
  UErrorCode error = U_ZERO_ERROR;
  std::unique_ptr<icu::Collator> collator(icu::Collator::createInstance(
      icu::Locale(g_browser_process->GetApplicationLocale().c_str()), error));
  if (U_FAILURE(error))
    collator.reset();
  CertNameComparator cert_name_comparator(collator.get());
  std::sort(certs_.begin(), certs_.end(), cert_name_comparator);
  std::sort(user_certs_.begin(), user_certs_.end(), cert_name_comparator);
  std::sort(server_certs_.begin(), server_certs_.end(), cert_name_comparator);
  std::sort(server_ca_certs_.begin(), server_ca_certs_.end(),
            cert_name_comparator);

  VLOG(1) << "certs_: " << certs_.size();
  VLOG(1) << "user_certs_: " << user_certs_.size();
  VLOG(1) << "server_certs_: " << server_certs_.size();
  VLOG(1) << "server_ca_certs_: " << server_ca_certs_.size();

  for (auto& observer : observer_list_)
    observer.OnCertificatesLoaded();
}

CERTCertificate* CertLibrary::GetCertificateAt(CertType type, int index) const {
  const net::ScopedCERTCertificateList& cert_list =
      GetCertificateListForType(type);
  DCHECK_GE(index, 0);
  DCHECK_LT(index, static_cast<int>(cert_list.size()));
  return cert_list[index].get();
}

const net::ScopedCERTCertificateList& CertLibrary::GetCertificateListForType(
    CertType type) const {
  if (type == CERT_TYPE_USER)
    return user_certs_;
  if (type == CERT_TYPE_SERVER)
    return server_certs_;
  if (type == CERT_TYPE_SERVER_CA)
    return server_ca_certs_;
  DCHECK(type == CERT_TYPE_DEFAULT);
  return certs_;
}

}  // namespace chromeos
