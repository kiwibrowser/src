// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_HIT_TEST_DATA_PROVIDER_AURA_H_
#define UI_AURA_HIT_TEST_DATA_PROVIDER_AURA_H_

#include "base/macros.h"
#include "components/viz/client/hit_test_data_provider.h"
#include "ui/aura/aura_export.h"

namespace aura {

class Window;

// A HitTestDataProvider that captures hit-test areas from a aura::Window tree
// and packages it to be submitted to compositor frame sink. The |window| used
// when creating the HitTestDataProviderAura should outlive the data provider.
class AURA_EXPORT HitTestDataProviderAura : public viz::HitTestDataProvider {
 public:
  explicit HitTestDataProviderAura(Window* window);
  ~HitTestDataProviderAura() override;

  // HitTestDataProvider:
  base::Optional<viz::HitTestRegionList> GetHitTestData(
      const viz::CompositorFrame& compositor_frame) const override;

 private:
  // Recursively walks the children of |window| and uses |window|'s
  // EventTargeter to generate hit-test data for the |window|'s descendants.
  // Populates |hit_test_region_list|.
  void GetHitTestDataRecursively(
      aura::Window* window,
      viz::HitTestRegionList* hit_test_region_list) const;

  aura::Window* const window_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(HitTestDataProviderAura);
};

}  // namespace aura

#endif  // UI_AURA_HIT_TEST_DATA_PROVIDER_AURA_H_
