// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ASH_WS_WINDOW_SERVICE_OWNER_H_
#define ASH_WS_WINDOW_SERVICE_OWNER_H_

#include <memory>

#include "ash/ash_export.h"
#include "ash/shell_init_params.h"
#include "base/memory/scoped_refptr.h"
#include "services/service_manager/public/mojom/service.mojom.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace service_manager {
class ServiceContext;
}

namespace ui {
namespace ws2 {
class GpuSupport;
class WindowService;
}  // namespace ws2
}  // namespace ui

namespace ash {

class WindowServiceDelegateImpl;

// WindowServiceOwner indirectly owns the WindowService. This class is
// responsible for responding to the ServiceRequest for the WindowService. When
// BindWindowService() is called the WindowService is created.
class ASH_EXPORT WindowServiceOwner {
 public:
  explicit WindowServiceOwner(std::unique_ptr<ui::ws2::GpuSupport> gpu_support);
  ~WindowServiceOwner();

  // Called from the ServiceManager when a request is made for the
  // WindowService.
  void BindWindowService(service_manager::mojom::ServiceRequest request);

 private:
  // Non-null until |service_context_| is created.
  std::unique_ptr<ui::ws2::GpuSupport> gpu_support_;

  // The following state is created once BindWindowService() is called.

  std::unique_ptr<WindowServiceDelegateImpl> window_service_delegate_;

  // Handles the ServiceRequest. Owns |window_service_|.
  std::unique_ptr<service_manager::ServiceContext> service_context_;

  // The WindowService. This is owned by |service_context_|.
  ui::ws2::WindowService* window_service_;

  DISALLOW_COPY_AND_ASSIGN(WindowServiceOwner);
};

// This function should be bound to a ServiceManagerConnection. The
// ServiceManager executes this on a background thread, so this posts a task
// to |main_runner| that calls BindWindowService().
ASH_EXPORT void BindWindowServiceOnIoThread(
    scoped_refptr<base::SingleThreadTaskRunner> main_runner,
    service_manager::mojom::ServiceRequest request);

}  // namespace ash

#endif  // ASH_WS_WINDOW_SERVICE_OWNER_H_
