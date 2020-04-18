// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_KEYED_SERVICE_CORE_KEYED_SERVICE_H_
#define COMPONENTS_KEYED_SERVICE_CORE_KEYED_SERVICE_H_

#include "components/keyed_service/core/keyed_service_export.h"

// Interface for keyed services that support two-phase destruction order.
//
// Two-phase shutdown allows keyed services to have a first pass shutdown phase
// where they drop references. Not all services will need this, so there's a
// default implementation. Only once every service has been given a chance to
// drop references are services deleted.
class KEYED_SERVICE_EXPORT KeyedService {
 public:
  KeyedService();

  // The second pass is the actual deletion of each object.
  virtual ~KeyedService();

  // The first pass is to call Shutdown on a KeyedService.
  virtual void Shutdown();
};

#endif  // COMPONENTS_KEYED_SERVICE_CORE_KEYED_SERVICE_H_
