// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_PAYMENTS_PAYMENT_REQUEST_FACTORY_H_
#define CHROME_BROWSER_PAYMENTS_PAYMENT_REQUEST_FACTORY_H_

#include "third_party/blink/public/platform/modules/payments/payment_request.mojom.h"

namespace content {
class RenderFrameHost;
}

namespace payments {

// Will create a PaymentRequest based on the contents of |request|. The
// |request| was initiated by the frame hosted by |render_frame_host|, which is
// inside of |web_contents|. This function is called every time a new instance
// of PaymentRequest is created in the renderer.
void CreatePaymentRequest(mojom::PaymentRequestRequest request,
                          content::RenderFrameHost* render_frame_host);

}  // namespace payments

#endif  // CHROME_BROWSER_PAYMENTS_PAYMENT_REQUEST_FACTORY_H_
