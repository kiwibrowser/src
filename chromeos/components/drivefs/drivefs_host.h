// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROMEOS_COMPONENTS_DRIVEFS_DRIVEFS_HOST_H_
#define CHROMEOS_COMPONENTS_DRIVEFS_DRIVEFS_HOST_H_

#include <memory>
#include <string>
#include <vector>

#include "base/component_export.h"
#include "base/files/file_path.h"
#include "base/files/scoped_file.h"
#include "base/macros.h"
#include "chromeos/components/drivefs/mojom/drivefs.mojom.h"
#include "chromeos/disks/disk_mount_manager.h"
#include "components/account_id/account_id.h"
#include "google_apis/gaia/oauth2_mint_token_flow.h"
#include "services/identity/public/mojom/identity_manager.mojom.h"

namespace net {
class URLRequestContextGetter;
}  // namespace net

namespace service_manager {
class Connector;
}  // namespace service_manager

namespace drivefs {

// A host for a DriveFS process. In addition to managing its lifetime via
// mounting and unmounting, it also bridges between the DriveFS process and the
// file manager.
class COMPONENT_EXPORT(DRIVEFS) DriveFsHost
    : public chromeos::disks::DiskMountManager::Observer {
 public:
  // Public for overriding in tests. A default implementation is used under
  // normal conditions.
  class MojoConnectionDelegate {
   public:
    virtual ~MojoConnectionDelegate() = default;

    // Prepare the mojo connection to be used to communicate with the DriveFS
    // process. Returns the mojo handle to use for bootstrapping.
    virtual mojom::DriveFsBootstrapPtrInfo InitializeMojoConnection() = 0;

    // Accepts the mojo connection over |handle|.
    virtual void AcceptMojoConnection(base::ScopedFD handle) = 0;
  };

  class Delegate {
   public:
    Delegate() = default;
    virtual ~Delegate() = default;

    virtual net::URLRequestContextGetter* GetRequestContext() = 0;
    virtual service_manager::Connector* GetConnector() = 0;
    virtual const AccountId& GetAccountId() = 0;
    virtual std::unique_ptr<OAuth2MintTokenFlow> CreateMintTokenFlow(
        OAuth2MintTokenFlow::Delegate* delegate,
        const std::string& client_id,
        const std::string& app_id,
        const std::vector<std::string>& scopes);
    virtual std::unique_ptr<MojoConnectionDelegate>
    CreateMojoConnectionDelegate();

    virtual void OnMounted(const base::FilePath& mount_path) = 0;

   private:
    DISALLOW_COPY_AND_ASSIGN(Delegate);
  };

  DriveFsHost(const base::FilePath& profile_path,
              Delegate* delegate);
  ~DriveFsHost() override;

  // Mount DriveFS.
  bool Mount();

  // Unmount DriveFS.
  void Unmount();

  // Returns whether DriveFS is mounted.
  bool IsMounted() const;

  // Returns the path where DriveFS is mounted. It is only valid to call when
  // |IsMounted()| returns true.
  const base::FilePath& GetMountPath() const;

 private:
  class MountState;

  // DiskMountManager::Observer:
  void OnMountEvent(chromeos::disks::DiskMountManager::MountEvent event,
                    chromeos::MountError error_code,
                    const chromeos::disks::DiskMountManager::MountPointInfo&
                        mount_info) override;

  SEQUENCE_CHECKER(sequence_checker_);

  // Returns the connection to the identity service, connecting lazily.
  identity::mojom::IdentityManager& GetIdentityManager();

  // The path to the user's profile.
  const base::FilePath profile_path_;

  Delegate* const delegate_;

  // State specific to the current mount, or null if not mounted.
  std::unique_ptr<MountState> mount_state_;

  // The connection to the identity service. Access via |GetIdentityManager()|.
  identity::mojom::IdentityManagerPtr identity_manager_;

  DISALLOW_COPY_AND_ASSIGN(DriveFsHost);
};

}  // namespace drivefs

#endif  // CHROMEOS_COMPONENTS_DRIVEFS_DRIVEFS_HOST_H_
