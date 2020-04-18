// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_SSL_INSECURE_SENSITIVE_INPUT_DRIVER_H_
#define CHROME_BROWSER_SSL_INSECURE_SENSITIVE_INPUT_DRIVER_H_

#include "mojo/public/cpp/bindings/binding_set.h"
#include "third_party/blink/public/platform/modules/insecure_input/insecure_input_service.mojom.h"

namespace content {
class RenderFrameHost;
}

// The InsecureSensitiveInputDriver watches for calls from renderers and
// instructs the parent |InsecureSensitiveInputDriverFactory| monitoring the
// WebContents to update the SSLStatusInputEventData.
//
// There is one InsecureSensitiveInputDriver per RenderFrameHost.
// The lifetime is managed by the InsecureSensitiveInputDriverFactory.
class InsecureSensitiveInputDriver : public blink::mojom::InsecureInputService {
 public:
  explicit InsecureSensitiveInputDriver(
      content::RenderFrameHost* render_frame_host);
  ~InsecureSensitiveInputDriver() override;

  void BindInsecureInputServiceRequest(
      blink::mojom::InsecureInputServiceRequest request);

  // blink::mojom::InsecureInputService:
  void PasswordFieldVisibleInInsecureContext() override;
  void AllPasswordFieldsInInsecureContextInvisible() override;
  void DidEditFieldInInsecureContext() override;

 private:
  content::RenderFrameHost* render_frame_host_;

  mojo::BindingSet<blink::mojom::InsecureInputService> insecure_input_bindings_;

  DISALLOW_COPY_AND_ASSIGN(InsecureSensitiveInputDriver);
};

#endif  // CHROME_BROWSER_SSL_INSECURE_SENSITIVE_INPUT_DRIVER_H_
