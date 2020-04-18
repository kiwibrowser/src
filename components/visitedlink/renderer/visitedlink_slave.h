// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VISITEDLINK_RENDERER_VISITEDLINK_SLAVE_H_
#define COMPONENTS_VISITEDLINK_RENDERER_VISITEDLINK_SLAVE_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "base/memory/read_only_shared_memory_region.h"
#include "base/memory/shared_memory_mapping.h"
#include "base/memory/weak_ptr.h"
#include "components/visitedlink/common/visitedlink.mojom.h"
#include "components/visitedlink/common/visitedlink_common.h"
#include "mojo/public/cpp/bindings/binding.h"

namespace visitedlink {

// Reads the link coloring database provided by the master. There can be any
// number of slaves reading the same database.
class VisitedLinkSlave : public VisitedLinkCommon,
                         public mojom::VisitedLinkNotificationSink {
 public:
  VisitedLinkSlave();
  ~VisitedLinkSlave() override;

  base::Callback<void(mojom::VisitedLinkNotificationSinkRequest)>
  GetBindCallback();

  // mojom::VisitedLinkNotificationSink overrides.
  void UpdateVisitedLinks(
      base::ReadOnlySharedMemoryRegion table_region) override;
  void AddVisitedLinks(
      const std::vector<VisitedLinkSlave::Fingerprint>& fingerprints) override;
  void ResetVisitedLinks(bool invalidate_hashes) override;

 private:
  void FreeTable();

  void Bind(mojom::VisitedLinkNotificationSinkRequest request);

  base::ReadOnlySharedMemoryMapping table_mapping_;

  mojo::Binding<mojom::VisitedLinkNotificationSink> binding_;

  base::WeakPtrFactory<VisitedLinkSlave> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(VisitedLinkSlave);
};

}  // namespace visitedlink

#endif  // COMPONENTS_VISITEDLINK_RENDERER_VISITEDLINK_SLAVE_H_
