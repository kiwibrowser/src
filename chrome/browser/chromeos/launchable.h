// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_CHROMEOS_LAUNCHABLE_H_
#define CHROME_BROWSER_CHROMEOS_LAUNCHABLE_H_

#include "base/macros.h"
#include "mash/public/mojom/launchable.mojom.h"
#include "mojo/public/cpp/bindings/binding_set.h"

namespace chromeos {

class Launchable : public mash::mojom::Launchable {
 public:
  Launchable();
  ~Launchable() override;

  void Bind(mash::mojom::LaunchableRequest request);

 private:
  // mash::mojom::Launchable:
  void Launch(uint32_t what, mash::mojom::LaunchMode how) override;

  void CreateNewWindowImpl(bool is_incognito);

  mojo::BindingSet<mash::mojom::Launchable> bindings_;

  DISALLOW_COPY_AND_ASSIGN(Launchable);
};

}  // namespace chromeos

#endif  // CHROME_BROWSER_CHROMEOS_LAUNCHABLE_H_
