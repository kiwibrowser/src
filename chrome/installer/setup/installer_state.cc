// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/installer/setup/installer_state.h"

#include <stddef.h>

#include <string>
#include <utility>

#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/win/registry.h"
#include "chrome/install_static/install_modes.h"
#include "chrome/installer/setup/setup_util.h"
#include "chrome/installer/util/app_registration_data.h"
#include "chrome/installer/util/google_update_settings.h"
#include "chrome/installer/util/helper.h"
#include "chrome/installer/util/install_util.h"
#include "chrome/installer/util/installation_state.h"
#include "chrome/installer/util/master_preferences.h"
#include "chrome/installer/util/master_preferences_constants.h"
#include "chrome/installer/util/product.h"
#include "chrome/installer/util/work_item.h"
#include "chrome/installer/util/work_item_list.h"

namespace installer {

InstallerState::InstallerState()
    : operation_(UNINITIALIZED),
      level_(UNKNOWN_LEVEL),
      root_key_(NULL),
      msi_(false),
      background_mode_(false),
      verbose_logging_(false),
      is_migrating_to_single_(false) {}

InstallerState::InstallerState(Level level)
    : operation_(UNINITIALIZED),
      level_(UNKNOWN_LEVEL),
      root_key_(NULL),
      msi_(false),
      background_mode_(false),
      verbose_logging_(false),
      is_migrating_to_single_(false) {
  // Use set_level() so that root_key_ is updated properly.
  set_level(level);
}

InstallerState::~InstallerState() {
}

void InstallerState::Initialize(const base::CommandLine& command_line,
                                const MasterPreferences& prefs,
                                const InstallationState& machine_state) {
  Clear();

  bool pref_bool;
  if (!prefs.GetBool(master_preferences::kSystemLevel, &pref_bool))
    pref_bool = false;
  set_level(pref_bool ? SYSTEM_LEVEL : USER_LEVEL);

  if (!prefs.GetBool(master_preferences::kVerboseLogging, &verbose_logging_))
    verbose_logging_ = false;

  if (!prefs.GetBool(master_preferences::kMsi, &msi_))
    msi_ = false;

  const bool is_uninstall = command_line.HasSwitch(switches::kUninstall);

  Product* p = AddProductFromPreferences(prefs, machine_state);
  BrowserDistribution* dist = p->distribution();
  VLOG(1) << (is_uninstall ? "Uninstall" : "Install")
          << " distribution: " << dist->GetDisplayName();

  state_key_ = dist->GetStateKey();

  if (is_uninstall) {
    operation_ = UNINSTALL;
  } else {
    operation_ = SINGLE_INSTALL_OR_UPDATE;
    // Is this a migration from multi-install to single-install?
    const ProductState* state = machine_state.GetProductState(system_install());
    is_migrating_to_single_ = state && state->is_multi_install();
  }

  // Parse --critical-update-version=W.X.Y.Z
  std::string critical_version_value(
      command_line.GetSwitchValueASCII(switches::kCriticalUpdateVersion));
  critical_update_version_ = base::Version(critical_version_value);
}

void InstallerState::set_level(Level level) {
  level_ = level;
  switch (level) {
    case USER_LEVEL:
      root_key_ = HKEY_CURRENT_USER;
      break;
    case SYSTEM_LEVEL:
      root_key_ = HKEY_LOCAL_MACHINE;
      break;
    default:
      DCHECK(level == UNKNOWN_LEVEL);
      level_ = UNKNOWN_LEVEL;
      root_key_ = NULL;
      break;
  }
}

// Evaluates a product's eligibility for participation in this operation.
// We never expect these checks to fail, hence they all terminate the process in
// debug builds.  See the log messages for details.
bool InstallerState::CanAddProduct(const base::FilePath* product_dir) const {
  if (product_) {
    LOG(DFATAL) << "Cannot process more than one product.";
    return false;
  }
  return true;
}

// Adds |product|, installed in |product_dir| to this object's collection.  If
// |product_dir| is NULL, the product's default install location is used.
// Returns NULL if |product| is incompatible with this object.  Otherwise,
// returns a pointer to the product (ownership is held by this object).
Product* InstallerState::AddProductInDirectory(
    const base::FilePath* product_dir,
    std::unique_ptr<Product> product) {
  DCHECK(product);
  const Product& the_product = *product;

  if (!CanAddProduct(product_dir))
    return nullptr;

  if (target_path_.empty()) {
    DCHECK_EQ(BrowserDistribution::GetDistribution(),
              the_product.distribution());
    target_path_ =
        product_dir ? *product_dir : GetChromeInstallPath(system_install());
  }

  if (state_key_.empty())
    state_key_ = the_product.distribution()->GetStateKey();

  product_ = std::move(product);
  return product_.get();
}

Product* InstallerState::AddProduct(std::unique_ptr<Product> product) {
  return AddProductInDirectory(nullptr, std::move(product));
}

// Adds a product constructed on the basis of |prefs|, setting this object's msi
// flag if the product is represented in |machine_state| and is msi-installed.
// Returns the product that was added, or NULL if |state| is incompatible with
// this object.  Ownership is not passed to the caller.
Product* InstallerState::AddProductFromPreferences(
    const MasterPreferences& prefs,
    const InstallationState& machine_state) {
  std::unique_ptr<Product> product_ptr(
      new Product(BrowserDistribution::GetDistribution()));

  Product* product = AddProductInDirectory(nullptr, std::move(product_ptr));

  if (product != NULL && !msi_) {
    const ProductState* product_state =
        machine_state.GetProductState(system_install());
    if (product_state != NULL)
      msi_ = product_state->is_msi();
  }

  return product;
}

Product* InstallerState::AddProductFromState(
    const ProductState& state) {
  std::unique_ptr<Product> product_ptr(
      new Product(BrowserDistribution::GetDistribution()));

  // Strip off <version>/Installer/setup.exe; see GetInstallerDirectory().
  base::FilePath product_dir =
      state.GetSetupPath().DirName().DirName().DirName();

  Product* product =
      AddProductInDirectory(&product_dir, std::move(product_ptr));

  if (product != NULL)
    msi_ |= state.is_msi();

  return product;
}

bool InstallerState::system_install() const {
  DCHECK(level_ == USER_LEVEL || level_ == SYSTEM_LEVEL);
  return level_ == SYSTEM_LEVEL;
}

base::Version* InstallerState::GetCurrentVersion(
    const InstallationState& machine_state) const {
  DCHECK(product_);
  std::unique_ptr<base::Version> current_version;
  const ProductState* product_state =
      machine_state.GetProductState(level_ == SYSTEM_LEVEL);

  if (product_state != NULL) {
    const base::Version* version = NULL;

    // Be aware that there might be a pending "new_chrome.exe" already in the
    // installation path.  If so, we use old_version, which holds the version of
    // "chrome.exe" itself.
    if (base::PathExists(target_path().Append(kChromeNewExe)))
      version = product_state->old_version();

    if (version == NULL)
      version = &product_state->version();

    current_version.reset(new base::Version(*version));
  }

  return current_version.release();
}

base::Version InstallerState::DetermineCriticalVersion(
    const base::Version* current_version,
    const base::Version& new_version) const {
  DCHECK(current_version == NULL || current_version->IsValid());
  DCHECK(new_version.IsValid());
  if (critical_update_version_.IsValid() &&
      (current_version == NULL ||
       (current_version->CompareTo(critical_update_version_) < 0)) &&
      new_version.CompareTo(critical_update_version_) >= 0) {
    return critical_update_version_;
  }
  return base::Version();
}

base::FilePath InstallerState::GetInstallerDirectory(
    const base::Version& version) const {
  return target_path().AppendASCII(version.GetString()).Append(kInstallerDir);
}

void InstallerState::Clear() {
  operation_ = UNINITIALIZED;
  target_path_.clear();
  state_key_.clear();
  product_.reset();
  critical_update_version_ = base::Version();
  level_ = UNKNOWN_LEVEL;
  root_key_ = NULL;
  msi_ = false;
  verbose_logging_ = false;
  is_migrating_to_single_ = false;
}

void InstallerState::SetStage(InstallerStage stage) const {
  GoogleUpdateSettings::SetProgress(system_install(), state_key_,
                                    progress_calculator_.Calculate(stage));
}

void InstallerState::UpdateChannels() const {
  DCHECK_NE(UNINSTALL, operation_);
  // Update the "ap" value for the product being installed/updated.  Use the
  // current value in the registry since the InstallationState instance used by
  // the bulk of the installer does not track changes made by UpdateStage.
  // Create the app's ClientState key if it doesn't exist.
  ChannelInfo channel_info;
  base::win::RegKey state_key;
  LONG result =
      state_key.Create(root_key_,
                       state_key_.c_str(),
                       KEY_QUERY_VALUE | KEY_SET_VALUE | KEY_WOW64_32KEY);
  if (result == ERROR_SUCCESS) {
    channel_info.Initialize(state_key);

    // Multi-install has been deprecated. All installs and updates are single.
    bool modified = channel_info.SetMultiInstall(false);

    // Remove all multi-install products from the channel name.
    modified |= channel_info.SetChrome(false);
    modified |= channel_info.SetChromeFrame(false);
    modified |= channel_info.SetAppLauncher(false);

    VLOG(1) << "ap: " << channel_info.value();

    // Write the results if needed.
    if (modified)
      channel_info.Write(&state_key);
  } else {
    LOG(ERROR) << "Failed opening key " << state_key_
               << " to update app channels; result: " << result;
  }
}

void InstallerState::WriteInstallerResult(
    InstallStatus status,
    int string_resource_id,
    const base::string16* const launch_cmd) const {
  // Use a no-rollback list since this is a best-effort deal.
  std::unique_ptr<WorkItemList> install_list(WorkItem::CreateWorkItemList());
  install_list->set_log_message("Write Installer Result");
  install_list->set_best_effort(true);
  install_list->set_rollback_enabled(false);
  const bool system_install = this->system_install();
  // Write the value for the product upon which we're operating.
  InstallUtil::AddInstallerResultItems(
      system_install, product_->distribution()->GetStateKey(), status,
      string_resource_id, launch_cmd, install_list.get());
  if (is_migrating_to_single() && InstallUtil::GetInstallReturnCode(status)) {
#if defined(GOOGLE_CHROME_BUILD)
    // Write to the binaries on error if this is a migration back to
    // single-install for Google Chrome builds. Skip this for Chromium builds
    // because they lump the "ClientState" and "Clients" keys into a single
    // key. As a consequence, writing this value causes Software\Chromium to be
    // re-created after it was deleted during the migration to single-install.
    // Google Chrome builds don't suffer this since the two keys are distinct
    // and have different lifetimes. The result is only written on failure since
    // for success, the binaries have been uninstalled and therefore the result
    // will not be read by Google Update.
    InstallUtil::AddInstallerResultItems(
        system_install, install_static::GetBinariesClientStateKeyPath(), status,
        string_resource_id, launch_cmd, install_list.get());
#endif
  }
  install_list->Do();
}

bool InstallerState::RequiresActiveSetup() const {
  return system_install();
}

}  // namespace installer
