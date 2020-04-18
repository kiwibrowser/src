// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/payments/core/payment_instrument.h"

namespace payments {

PaymentInstrument::PaymentInstrument(int icon_resource_id, Type type)
    : icon_resource_id_(icon_resource_id), type_(type) {}

PaymentInstrument::~PaymentInstrument() {}

const gfx::ImageSkia* PaymentInstrument::icon_image_skia() const {
  return nullptr;
}

}  // namespace payments
