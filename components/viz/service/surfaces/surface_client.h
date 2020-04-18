// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_SERVICE_SURFACES_SURFACE_CLIENT_H_
#define COMPONENTS_VIZ_SERVICE_SURFACES_SURFACE_CLIENT_H_

#include <vector>

#include "base/macros.h"
#include "components/viz/service/viz_service_export.h"

namespace viz {
struct ReturnedResource;
class Surface;
struct TransferableResource;

class VIZ_SERVICE_EXPORT SurfaceClient {
 public:
  SurfaceClient() = default;

  virtual ~SurfaceClient() = default;

  // Called when |surface| has a new CompositorFrame available for display.
  virtual void OnSurfaceActivated(Surface* surface) = 0;

  // Called when |surface| is about to be destroyed.
  virtual void OnSurfaceDiscarded(Surface* surface) = 0;

  // Increments the reference count on resources specified by |resources|.
  virtual void RefResources(
      const std::vector<TransferableResource>& resources) = 0;

  // Decrements the reference count on resources specified by |resources|.
  virtual void UnrefResources(
      const std::vector<ReturnedResource>& resources) = 0;

  // ReturnResources gets called when the display compositor is done using the
  // resources so that the client can use them.
  virtual void ReturnResources(
      const std::vector<ReturnedResource>& resources) = 0;

  // Increments the reference count of resources received from a child
  // compositor.
  virtual void ReceiveFromChild(
      const std::vector<TransferableResource>& resources) = 0;

  // Takes all the CopyOutputRequests made at the client level that happened for
  // a LocalSurfaceId preceeding the given one.
  virtual std::vector<std::unique_ptr<CopyOutputRequest>>
  TakeCopyOutputRequests(const LocalSurfaceId& latest_surface_id) = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(SurfaceClient);
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_SERVICE_SURFACES_SURFACE_CLIENT_H_
