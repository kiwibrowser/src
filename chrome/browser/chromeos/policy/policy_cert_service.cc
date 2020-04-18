// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/policy/policy_cert_service.h"

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "chrome/browser/chromeos/policy/policy_cert_service_factory.h"
#include "chrome/browser/chromeos/policy/policy_cert_verifier.h"
#include "components/user_manager/user_manager.h"
#include "content/public/browser/browser_thread.h"
#include "net/cert/x509_certificate.h"

namespace policy {

PolicyCertService::~PolicyCertService() {
  DCHECK(cert_verifier_)
      << "CreatePolicyCertVerifier() must be called after construction.";
}

PolicyCertService::PolicyCertService(
    const std::string& user_id,
    UserNetworkConfigurationUpdater* net_conf_updater,
    user_manager::UserManager* user_manager)
    : cert_verifier_(NULL),
      user_id_(user_id),
      net_conf_updater_(net_conf_updater),
      user_manager_(user_manager),
      has_trust_anchors_(false),
      weak_ptr_factory_(this) {
  DCHECK(net_conf_updater_);
  DCHECK(user_manager_);
}

PolicyCertService::PolicyCertService(const std::string& user_id,
                                     PolicyCertVerifier* verifier,
                                     user_manager::UserManager* user_manager)
    : cert_verifier_(verifier),
      user_id_(user_id),
      net_conf_updater_(NULL),
      user_manager_(user_manager),
      has_trust_anchors_(false),
      weak_ptr_factory_(this) {
}

std::unique_ptr<PolicyCertVerifier>
PolicyCertService::CreatePolicyCertVerifier() {
  base::Closure callback = base::Bind(
      &PolicyCertServiceFactory::SetUsedPolicyCertificates, user_id_);
  cert_verifier_ = new PolicyCertVerifier(
      base::Bind(base::IgnoreResult(&content::BrowserThread::PostTask),
                 content::BrowserThread::UI,
                 FROM_HERE,
                 callback));
  // Certs are forwarded to |cert_verifier_|, thus register here after
  // |cert_verifier_| is created.
  net_conf_updater_->AddTrustedCertsObserver(this);

  // Set the current list of trust anchors.
  net::CertificateList trust_anchors;
  net_conf_updater_->GetWebTrustedCertificates(&trust_anchors);
  OnTrustAnchorsChanged(trust_anchors);

  return base::WrapUnique(cert_verifier_);
}

void PolicyCertService::OnTrustAnchorsChanged(
    const net::CertificateList& trust_anchors) {
  DCHECK(cert_verifier_);

  // Do not use certificates installed via ONC policy if the current session has
  // multiple profiles. This is important to make sure that any possibly tainted
  // data is absolutely confined to the managed profile and never, ever leaks to
  // any other profile.
  if (!trust_anchors.empty() && user_manager_->GetLoggedInUsers().size() > 1u) {
    LOG(ERROR) << "Ignoring ONC-pushed certificates update because multiple "
               << "users are logged in.";
    return;
  }

  has_trust_anchors_ = !trust_anchors.empty();

  // It's safe to use base::Unretained here, because it's guaranteed that
  // |cert_verifier_| outlives this object (see description of
  // CreatePolicyCertVerifier).
  // Note: ProfileIOData, which owns the CertVerifier is deleted by a
  // DeleteSoon on IO, i.e. after all pending tasks on IO are finished.
  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::BindOnce(&PolicyCertVerifier::SetTrustAnchors,
                     base::Unretained(cert_verifier_), trust_anchors));
}

bool PolicyCertService::UsedPolicyCertificates() const {
  return PolicyCertServiceFactory::UsedPolicyCertificates(user_id_);
}

void PolicyCertService::Shutdown() {
  weak_ptr_factory_.InvalidateWeakPtrs();
  if (net_conf_updater_)
    net_conf_updater_->RemoveTrustedCertsObserver(this);
  OnTrustAnchorsChanged(net::CertificateList());
  net_conf_updater_ = NULL;
}

// static
std::unique_ptr<PolicyCertService> PolicyCertService::CreateForTesting(
    const std::string& user_id,
    PolicyCertVerifier* verifier,
    user_manager::UserManager* user_manager) {
  return base::WrapUnique(
      new PolicyCertService(user_id, verifier, user_manager));
}

}  // namespace policy
