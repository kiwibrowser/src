// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/passwords/manage_passwords_view_utils.h"

#include <stddef.h>

#include "base/macros.h"
#include "base/strings/string16.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/browser_process.h"
#include "chrome/grit/generated_resources.h"
#include "components/strings/grit/components_strings.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/gfx/range/range.h"
#include "url/gurl.h"

namespace {

// ScopedResourceOverride allows overriding localised strings in the shared
// instance of the resource bundle, while restoring the bundle state on
// destruction.
class ScopedResourceOverride {
 public:
  ScopedResourceOverride()
      : had_shared_instance_(ui::ResourceBundle::HasSharedInstance()),
        bundle_(GetOrCreateSharedInstance()),
        app_locale_(g_browser_process->GetApplicationLocale()) {}

  ~ScopedResourceOverride() {
    if (had_shared_instance_) {
      // Reloading the resources will discard all overrides.
      bundle_.ReloadLocaleResources(app_locale_);
    } else {
      ui::ResourceBundle::CleanupSharedInstance();
    }
  }

  void OverrideLocaleStringResource(int string_id, const base::string16& str) {
    bundle_.OverrideLocaleStringResource(string_id, str);
  }

 private:
  // Returns the shared resource bundle. Creates one if there was none.
  static ui::ResourceBundle& GetOrCreateSharedInstance() {
    if (!ui::ResourceBundle::HasSharedInstance()) {
      ui::ResourceBundle::InitSharedInstanceWithLocale(
          "en", nullptr, ui::ResourceBundle::LOAD_COMMON_RESOURCES);
    }
    return ui::ResourceBundle::GetSharedInstance();
  }

  const bool had_shared_instance_;  // Was there a shared bundle before?
  ui::ResourceBundle& bundle_;  // The shared bundle.
  const std::string app_locale_;

  DISALLOW_COPY_AND_ASSIGN(ScopedResourceOverride);
};

const struct {
  const char* const user_visible_url;
  const char* const form_origin_url;
  bool is_smartlock_branding_enabled;
  PasswordTitleType bubble_type;
  const char* const expected_domain_placeholder;  // "this site" or domain name
  size_t expected_link_range_start;
  size_t expected_link_range_end;
} kDomainsTestCases[] = {
    // Same domains.
    {"http://example.com/landing", "http://example.com/login#form?value=3",
     false, PasswordTitleType::SAVE_PASSWORD, "this site", 0, 0},
    {"http://example.com/landing", "http://example.com/login#form?value=3",
     true, PasswordTitleType::SAVE_PASSWORD, "this site", 12, 29},

    // Different subdomains.
    {"https://a.example.com/landing",
     "https://b.example.com/login#form?value=3", false,
     PasswordTitleType::SAVE_PASSWORD, "this site", 0, 0},
    {"https://a.example.com/landing",
     "https://b.example.com/login#form?value=3", true,
     PasswordTitleType::SAVE_PASSWORD, "this site", 12, 29},

    // Different domains.
    {"https://another.org", "https://example.com:/login#form?value=3", false,
     PasswordTitleType::SAVE_PASSWORD, "https://example.com", 0, 0},
    {"https://another.org", "https://example.com/login#form?value=3", true,
     PasswordTitleType::SAVE_PASSWORD, "https://example.com", 12, 29},

    // Different domains and password form origin url with
    // default port for the scheme.
    {"https://another.org", "https://example.com:443/login#form?value=3", false,
     PasswordTitleType::SAVE_PASSWORD, "https://example.com", 0, 0},
    {"https://another.org", "http://example.com:80/login#form?value=3", true,
     PasswordTitleType::SAVE_PASSWORD, "http://example.com", 12, 29},

    // Different domains and password form origin url with
    // non-default port for the scheme.
    {"https://another.org", "https://example.com:8001/login#form?value=3",
     false, PasswordTitleType::SAVE_PASSWORD, "https://example.com:8001", 0, 0},
    {"https://another.org", "https://example.com:8001/login#form?value=3", true,
     PasswordTitleType::SAVE_PASSWORD, "https://example.com:8001", 12, 29},

    // Update bubble, same domains.
    {"http://example.com/landing", "http://example.com/login#form?value=3",
     false, PasswordTitleType::UPDATE_PASSWORD, "this site", 0, 0},
    {"http://example.com/landing", "http://example.com/login#form?value=3",
     true, PasswordTitleType::UPDATE_PASSWORD, "this site", 12, 29},

    // Update bubble, different domains.
    {"https://another.org", "http://example.com/login#form?value=3", false,
     PasswordTitleType::UPDATE_PASSWORD, "http://example.com", 0, 0},
    {"https://another.org", "http://example.com/login#form?value=3", true,
     PasswordTitleType::UPDATE_PASSWORD, "http://example.com", 12, 29},

    // Same domains, federated credential.
    {"http://example.com/landing", "http://example.com/login#form?value=3",
     false, PasswordTitleType::SAVE_ACCOUNT, "this site", 0, 0},
    {"http://example.com/landing", "http://example.com/login#form?value=3",
     true, PasswordTitleType::SAVE_ACCOUNT, "this site", 12, 29},

    // Different subdomains, federated credential.
    {"https://a.example.com/landing",
     "https://b.example.com/login#form?value=3", false,
     PasswordTitleType::SAVE_ACCOUNT, "this site", 0, 0},
    {"https://a.example.com/landing",
     "https://b.example.com/login#form?value=3", true,
     PasswordTitleType::SAVE_ACCOUNT, "this site", 12, 29}};

}  // namespace

// Test for GetSavePasswordDialogTitleTextAndLinkRange().
TEST(ManagePasswordsViewUtilTest, GetSavePasswordDialogTitleTextAndLinkRange) {
  for (size_t i = 0; i < arraysize(kDomainsTestCases); ++i) {
    SCOPED_TRACE(testing::Message() << "user_visible_url = "
                                    << kDomainsTestCases[i].user_visible_url
                                    << ", form_origin_url = "
                                    << kDomainsTestCases[i].form_origin_url);

    base::string16 title;
    gfx::Range title_link_range;
    GetSavePasswordDialogTitleTextAndLinkRange(
        GURL(kDomainsTestCases[i].user_visible_url),
        GURL(kDomainsTestCases[i].form_origin_url),
        kDomainsTestCases[i].is_smartlock_branding_enabled,
        kDomainsTestCases[i].bubble_type, &title, &title_link_range);

    // Verify against expectations.
    base::string16 domain =
        base::ASCIIToUTF16(kDomainsTestCases[i].expected_domain_placeholder);
    EXPECT_TRUE(title.find(domain) != base::string16::npos);
    EXPECT_EQ(kDomainsTestCases[i].expected_link_range_start,
              title_link_range.start());
    EXPECT_EQ(kDomainsTestCases[i].expected_link_range_end,
              title_link_range.end());
    if (kDomainsTestCases[i].bubble_type ==
        PasswordTitleType::UPDATE_PASSWORD) {
      EXPECT_TRUE(title.find(base::ASCIIToUTF16("update")) !=
                  base::string16::npos);
    } else {
      EXPECT_TRUE(title.find(base::ASCIIToUTF16("save")) !=
                  base::string16::npos);
    }
  }
}

// Check that empty localised strings do not cause a crash.
TEST(ManagePasswordsViewUtilTest,
     GetSavePasswordDialogTitleTextAndLinkRange_EmptyStrings) {
  ScopedResourceOverride resource_override;

  // Ensure that the resource bundle returns an empty string for the UI.
  resource_override.OverrideLocaleStringResource(IDS_SAVE_PASSWORD,
                                                 base::string16());

  base::string16 title;
  gfx::Range title_link_range;
  const GURL kExample("http://example.org");
  const bool kBrandingEnabled = true;
  // The arguments passed below have this importance for the codepath:
  // * The first two URLs need to be the same, otherwise
  //   IDS_SAVE_PASSWORD_DIFFERENT_DOMAINS_TITLE will be used instead of
  //   IDS_SAVE_PASSWORD overridden above.
  // * |kBrandingEnabled| needs to be true, otherwise the code won't try to
  //   dereference placeholder offsets from the localised string, which
  //   triggers the crash in http://crbug.com/658902.
  // * SAVE_PASSWORD dialog type needs to be passed to match the
  //   IDS_SAVE_PASSWORD overridden above.
  GetSavePasswordDialogTitleTextAndLinkRange(
      kExample, kExample, kBrandingEnabled, PasswordTitleType::SAVE_PASSWORD,
      &title, &title_link_range);
  // Verify that the test did not pass just because
  // GetSavePasswordDialogTitleTextAndLinkRange changed the resource IDs it uses
  // (and hence did not get the overridden empty string). If the empty localised
  // string was used, the title and the range will be empty as well.
  EXPECT_TRUE(title_link_range.is_empty());
  EXPECT_THAT(title, testing::IsEmpty());
}

TEST(ManagePasswordsViewUtilTest, GetManagePasswordsDialogTitleText) {
  for (size_t i = 0; i < arraysize(kDomainsTestCases); ++i) {
    SCOPED_TRACE(testing::Message() << "user_visible_url = "
                                    << kDomainsTestCases[i].user_visible_url
                                    << ", password_origin_url = "
                                    << kDomainsTestCases[i].form_origin_url);

    base::string16 title;
    GetManagePasswordsDialogTitleText(
        GURL(kDomainsTestCases[i].user_visible_url),
        GURL(kDomainsTestCases[i].form_origin_url), true, &title);

    // Verify against expectations.
    base::string16 domain =
        base::ASCIIToUTF16(kDomainsTestCases[i].expected_domain_placeholder);
    EXPECT_TRUE(title.find(domain) != base::string16::npos);
  }
}

// The parameter is |many_accounts| passed to
// GetAccountChooserDialogTitleTextAndLinkRange
class AccountChooserDialogTitleTest : public ::testing::TestWithParam<bool> {
};

TEST_P(AccountChooserDialogTitleTest,
       GetAccountChooserDialogTitleTextAndLinkRangeSmartLockUsers) {
  base::string16 branding =
      l10n_util::GetStringUTF16(IDS_PASSWORD_MANAGER_SMART_LOCK);
  base::string16 title;
  gfx::Range title_link_range;
  GetAccountChooserDialogTitleTextAndLinkRange(
      true /* is_smartlock_branding_enabled */,
      GetParam(),
      &title, &title_link_range);

  // Check that branding string is a part of a title.
  EXPECT_LT(title.find(branding, 0), title.size());
  EXPECT_GT(title.find(branding, 0), 0U);
  // Check that link range is not empty.
  EXPECT_NE(0U, title_link_range.start());
  EXPECT_NE(0U, title_link_range.end());
}

TEST_P(AccountChooserDialogTitleTest,
       GetAccountChooserDialogTitleTextAndLinkRangeNonSmartLockUsers) {
  base::string16 branding =
      l10n_util::GetStringUTF16(IDS_PASSWORD_MANAGER_SMART_LOCK);
  base::string16 title;
  gfx::Range title_link_range;
  GetAccountChooserDialogTitleTextAndLinkRange(
      false /* is_smartlock_branding_enabled */,
      GetParam(),
      &title, &title_link_range);
  EXPECT_GE(title.find(branding, 0), title.size());
  EXPECT_EQ(0U, title_link_range.start());
  EXPECT_EQ(0U, title_link_range.end());
}

INSTANTIATE_TEST_CASE_P(, AccountChooserDialogTitleTest,
                        ::testing::Bool());
