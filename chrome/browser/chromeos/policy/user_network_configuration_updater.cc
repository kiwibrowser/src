// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/user_network_configuration_updater.h"

#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/logging.h"
#include "base/values.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/chromeos/login/session/user_session_manager.h"
#include "chrome/browser/net/nss_context.h"
#include "chrome/browser/profiles/profile.h"
#include "chromeos/network/managed_network_configuration_handler.h"
#include "chromeos/network/onc/onc_certificate_importer_impl.h"
#include "chromeos/network/onc/onc_parsed_certificates.h"
#include "chromeos/network/onc/onc_utils.h"
#include "components/policy/policy_constants.h"
#include "components/user_manager/user.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_source.h"
#include "net/cert/x509_certificate.h"
#include "net/cert/x509_util_nss.h"

namespace policy {

UserNetworkConfigurationUpdater::~UserNetworkConfigurationUpdater() {}

// static
std::unique_ptr<UserNetworkConfigurationUpdater>
UserNetworkConfigurationUpdater::CreateForUserPolicy(
    Profile* profile,
    bool allow_trusted_certs_from_policy,
    const user_manager::User& user,
    PolicyService* policy_service,
    chromeos::ManagedNetworkConfigurationHandler* network_config_handler) {
  std::unique_ptr<UserNetworkConfigurationUpdater> updater(
      new UserNetworkConfigurationUpdater(
          profile, allow_trusted_certs_from_policy, user, policy_service,
          network_config_handler));
  updater->Init();
  return updater;
}

void UserNetworkConfigurationUpdater::AddTrustedCertsObserver(
    WebTrustedCertsObserver* observer) {
  observer_list_.AddObserver(observer);
}

void UserNetworkConfigurationUpdater::RemoveTrustedCertsObserver(
    WebTrustedCertsObserver* observer) {
  observer_list_.RemoveObserver(observer);
}

UserNetworkConfigurationUpdater::UserNetworkConfigurationUpdater(
    Profile* profile,
    bool allow_trusted_certs_from_policy,
    const user_manager::User& user,
    PolicyService* policy_service,
    chromeos::ManagedNetworkConfigurationHandler* network_config_handler)
    : NetworkConfigurationUpdater(onc::ONC_SOURCE_USER_POLICY,
                                  key::kOpenNetworkConfiguration,
                                  policy_service,
                                  network_config_handler),
      allow_trusted_certificates_from_policy_(allow_trusted_certs_from_policy),
      user_(&user),
      weak_factory_(this) {
  // The updater is created with |certificate_importer_| unset and is
  // responsible for creating it. This requires |GetNSSCertDatabaseForProfile|
  // call, which is not safe before the profile initialization is finalized.
  // Thus, listen for PROFILE_ADDED notification, on which |cert_importer_|
  // creation should start.
  registrar_.Add(this,
                 chrome::NOTIFICATION_PROFILE_ADDED,
                 content::Source<Profile>(profile));
}

void UserNetworkConfigurationUpdater::SetCertificateImporterForTest(
    std::unique_ptr<chromeos::onc::CertificateImporter> certificate_importer) {
  SetCertificateImporter(std::move(certificate_importer));
}

void UserNetworkConfigurationUpdater::GetWebTrustedCertificates(
    net::CertificateList* certs) const {
  *certs = web_trust_certs_;
}

void UserNetworkConfigurationUpdater::OnCertificatesImported(
    bool /* unused success */,
    net::ScopedCERTCertificateList onc_trusted_certificates) {
  web_trust_certs_.clear();
  if (allow_trusted_certificates_from_policy_) {
    web_trust_certs_ =
        net::x509_util::CreateX509CertificateListFromCERTCertificates(
            onc_trusted_certificates);
  }
  NotifyTrustAnchorsChanged();
}

void UserNetworkConfigurationUpdater::ImportCertificates(
    const base::ListValue& certificates_onc) {
  // If certificate importer is not yet set, cache the certificate onc. It will
  // be imported when the certificate importer gets set.
  if (!certificate_importer_) {
    pending_certificates_onc_.reset(certificates_onc.DeepCopy());
    return;
  }

  certificate_importer_->ImportCertificates(
      std::make_unique<chromeos::onc::OncParsedCertificates>(certificates_onc),
      onc_source_,
      base::Bind(&UserNetworkConfigurationUpdater::OnCertificatesImported,
                 base::Unretained(this)));
}

void UserNetworkConfigurationUpdater::ApplyNetworkPolicy(
    base::ListValue* network_configs_onc,
    base::DictionaryValue* global_network_config) {
  DCHECK(user_);
  chromeos::onc::ExpandStringPlaceholdersInNetworksForUser(user_,
                                                           network_configs_onc);

  // Call on UserSessionManager to send the user's password to session manager
  // if the password substitution variable exists in the ONC.
  bool send_password =
      chromeos::onc::HasUserPasswordSubsitutionVariable(network_configs_onc);
  chromeos::UserSessionManager::GetInstance()->OnUserNetworkPolicyParsed(
      send_password);

  network_config_handler_->SetPolicy(onc_source_,
                                     user_->username_hash(),
                                     *network_configs_onc,
                                     *global_network_config);
}

void UserNetworkConfigurationUpdater::Observe(
    int type,
    const content::NotificationSource& source,
    const content::NotificationDetails& details) {
  DCHECK_EQ(type, chrome::NOTIFICATION_PROFILE_ADDED);
  Profile* profile = content::Source<Profile>(source).ptr();

  GetNSSCertDatabaseForProfile(
      profile,
      base::Bind(
          &UserNetworkConfigurationUpdater::CreateAndSetCertificateImporter,
          weak_factory_.GetWeakPtr()));
}

void UserNetworkConfigurationUpdater::CreateAndSetCertificateImporter(
    net::NSSCertDatabase* database) {
  DCHECK(database);
  SetCertificateImporter(std::unique_ptr<chromeos::onc::CertificateImporter>(
      new chromeos::onc::CertificateImporterImpl(
          content::BrowserThread::GetTaskRunnerForThread(
              content::BrowserThread::IO),
          database)));
}

void UserNetworkConfigurationUpdater::SetCertificateImporter(
    std::unique_ptr<chromeos::onc::CertificateImporter> certificate_importer) {
  certificate_importer_ = std::move(certificate_importer);

  if (pending_certificates_onc_)
    ImportCertificates(*pending_certificates_onc_);
  pending_certificates_onc_.reset();
}

void UserNetworkConfigurationUpdater::NotifyTrustAnchorsChanged() {
  for (auto& observer : observer_list_)
    observer.OnTrustAnchorsChanged(web_trust_certs_);
}

}  // namespace policy
