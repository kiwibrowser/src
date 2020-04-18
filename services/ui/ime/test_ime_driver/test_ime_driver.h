// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_UI_IME_TEST_IME_DRIVER_TEST_IME_DRIVER_H_
#define SERVICES_UI_IME_TEST_IME_DRIVER_TEST_IME_DRIVER_H_

#include <stdint.h>

#include <map>
#include <memory>

#include "services/ui/public/interfaces/ime/ime.mojom.h"

namespace ui {
namespace test {

class TestIMEDriver : public ui::mojom::IMEDriver {
 public:
  TestIMEDriver();
  ~TestIMEDriver() override;

 private:
  // ui::mojom::IMEDriver:
  void StartSession(ui::mojom::StartSessionDetailsPtr details) override;

  DISALLOW_COPY_AND_ASSIGN(TestIMEDriver);
};

}  // namespace test
}  // namespace ui

#endif  // SERVICES_UI_IME_TEST_IME_DRIVER_TEST_IME_DRIVER_H_
