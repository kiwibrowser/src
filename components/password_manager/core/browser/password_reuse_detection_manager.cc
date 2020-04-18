// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/browser/password_reuse_detection_manager.h"

#include "base/time/default_clock.h"
#include "components/password_manager/core/browser/browser_save_password_progress_logger.h"
#include "components/password_manager/core/browser/password_manager.h"
#include "components/password_manager/core/browser/password_manager_client.h"
#include "components/password_manager/core/browser/password_manager_metrics_util.h"
#include "components/password_manager/core/browser/password_manager_util.h"
#include "ui/events/keycodes/keyboard_codes_posix.h"

using base::Time;
using base::TimeDelta;

namespace password_manager {

namespace {
constexpr size_t kMaxNumberOfCharactersToStore = 30;
constexpr TimeDelta kMaxInactivityTime = TimeDelta::FromSeconds(10);
}

PasswordReuseDetectionManager::PasswordReuseDetectionManager(
    PasswordManagerClient* client)
    : client_(client), clock_(base::DefaultClock::GetInstance()) {
  DCHECK(client_);
}

PasswordReuseDetectionManager::~PasswordReuseDetectionManager() {}

void PasswordReuseDetectionManager::DidNavigateMainFrame(
    const GURL& main_frame_url) {
  if (main_frame_url.host() == main_frame_url_.host())
    return;

  main_frame_url_ = main_frame_url;
  input_characters_.clear();
  reuse_on_this_page_was_found_ = false;
}

void PasswordReuseDetectionManager::OnKeyPressed(const base::string16& text) {
  // Do not check reuse if it was already found on this page.
  if (reuse_on_this_page_was_found_)
    return;

  // Clear the buffer if last keystoke was more than kMaxInactivityTime ago.
  Time now = clock_->Now();
  if (!last_keystroke_time_.is_null() &&
      (now - last_keystroke_time_) >= kMaxInactivityTime) {
    input_characters_.clear();
  }
  last_keystroke_time_ = now;

  // Clear the buffer and return when enter is pressed.
  if (text.size() == 1 && text[0] == ui::VKEY_RETURN) {
    input_characters_.clear();
    return;
  }

  input_characters_ += text;
  if (input_characters_.size() > kMaxNumberOfCharactersToStore) {
    input_characters_.erase(
        0, input_characters_.size() - kMaxNumberOfCharactersToStore);
  }

  PasswordStore* store = client_->GetPasswordStore();
  if (!store)
    return;
  store->CheckReuse(input_characters_, main_frame_url_.GetOrigin().spec(),
                    this);
}

void PasswordReuseDetectionManager::OnReuseFound(
    size_t password_length,
    base::Optional<PasswordHashData> reused_protected_password_hash,
    const std::vector<std::string>& matching_domains,
    int saved_passwords) {
  reuse_on_this_page_was_found_ = true;
  std::unique_ptr<BrowserSavePasswordProgressLogger> logger;
  bool matches_sync_password = reused_protected_password_hash &&
                               reused_protected_password_hash->is_gaia_password;
  if (password_manager_util::IsLoggingActive(client_)) {
    logger.reset(
        new BrowserSavePasswordProgressLogger(client_->GetLogManager()));
    std::vector<std::string> domains_to_log(matching_domains);
    if (matches_sync_password)
      domains_to_log.push_back("CHROME SYNC PASSWORD");
    // TODO(nparker): Implement LogList() to log all domains in one call.
    for (const auto& domain : domains_to_log) {
      logger->LogString(BrowserSavePasswordProgressLogger::STRING_REUSE_FOUND,
                        domain);
    }
  }

  // PasswordManager could be nullptr in tests.
  bool password_field_detected =
      client_->GetPasswordManager()
          ? client_->GetPasswordManager()->IsPasswordFieldDetectedOnPage()
          : false;

  metrics_util::LogPasswordReuse(password_length, saved_passwords,
                                 matching_domains.size(),
                                 password_field_detected);
#if defined(SAFE_BROWSING_DB_LOCAL)
  // TODO(jialiul): After CSD whitelist being added to Android, we should gate
  // this by either SAFE_BROWSING_DB_LOCAL or SAFE_BROWSING_DB_REMOTE.
  if (matches_sync_password) {
    client_->LogPasswordReuseDetectedEvent();
  }

  client_->CheckProtectedPasswordEntry(matches_sync_password, matching_domains,
                                       password_field_detected);
#endif
}

void PasswordReuseDetectionManager::SetClockForTesting(base::Clock* clock) {
  clock_ = clock;
}

}  // namespace password_manager
