// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_MEDIA_MEDIA_PERMISSION_DISPATCHER_H_
#define CONTENT_RENDERER_MEDIA_MEDIA_PERMISSION_DISPATCHER_H_

#include <stdint.h>

#include <map>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "content/common/content_export.h"
#include "media/base/media_permission.h"
#include "third_party/blink/public/platform/modules/permissions/permission.mojom.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace content {

// MediaPermission implementation using content PermissionService.
class CONTENT_EXPORT MediaPermissionDispatcher : public media::MediaPermission {
 public:
  using ConnectToServiceCB = base::RepeatingCallback<void(
      mojo::InterfaceRequest<blink::mojom::PermissionService>)>;
  using IsEncryptedMediaEnabledCB = base::RepeatingCallback<bool()>;

  MediaPermissionDispatcher(
      const ConnectToServiceCB& connect_to_service_cb,
      const IsEncryptedMediaEnabledCB& is_encrypted_media_enabled_cb);
  ~MediaPermissionDispatcher() override;

  // Called when the frame owning this MediaPermissionDispatcher is navigated.
  void OnNavigation();

  // media::MediaPermission implementation.
  // Note: Can be called on any thread. The |permission_status_cb| will always
  // be fired on the thread where these methods are called.
  void HasPermission(Type type,
                     const PermissionStatusCB& permission_status_cb) override;
  void RequestPermission(
      Type type,
      const PermissionStatusCB& permission_status_cb) override;
  bool IsEncryptedMediaEnabled() override;

 private:
  // Map of request IDs and pending PermissionStatusCBs.
  typedef std::map<uint32_t, PermissionStatusCB> RequestMap;

  // Register PermissionStatusCBs. Returns |request_id| that can be used to make
  // PermissionService calls.
  uint32_t RegisterCallback(const PermissionStatusCB& permission_status_cb);

  // Ensure there is a connection to the permission service and return it.
  blink::mojom::PermissionService* GetPermissionService();

  // Callback for |permission_service_| calls.
  void OnPermissionStatus(uint32_t request_id,
                          blink::mojom::PermissionStatus status);

  // Callback for |permission_service_| connection errors.
  void OnConnectionError();

  ConnectToServiceCB connect_to_service_cb_;
  IsEncryptedMediaEnabledCB is_encrypted_media_enabled_cb_;
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  uint32_t next_request_id_;
  RequestMap requests_;
  blink::mojom::PermissionServicePtr permission_service_;

  // Used to safely post MediaPermission calls for execution on |task_runner_|.
  base::WeakPtr<MediaPermissionDispatcher> weak_ptr_;

  base::WeakPtrFactory<MediaPermissionDispatcher> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(MediaPermissionDispatcher);
};

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_MEDIA_PERMISSION_DISPATCHER_H_
