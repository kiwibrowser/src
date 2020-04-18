// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/page_load_metrics/observers/use_counter/ukm_features.h"
#include "base/containers/flat_set.h"

using WebFeature = blink::mojom::WebFeature;

// UKM-based UseCounter features (WebFeature) should be defined in
// opt_in_features list.
bool IsAllowedUkmFeature(blink::mojom::WebFeature feature) {
  CR_DEFINE_STATIC_LOCAL(
      const base::flat_set<WebFeature>, opt_in_features,
      ({
          WebFeature::kNavigatorVibrate, WebFeature::kNavigatorVibrateSubFrame,
          WebFeature::kTouchEventPreventedNoTouchAction,
          WebFeature::kTouchEventPreventedForcedDocumentPassiveNoTouchAction,
          // kDataUriHasOctothorpe may not be recorded correctly for iframes.
          // See https://crbug.com/796173 for details.
          WebFeature::kDataUriHasOctothorpe,
          WebFeature::kApplicationCacheManifestSelectInsecureOrigin,
          WebFeature::kApplicationCacheManifestSelectSecureOrigin,
          WebFeature::kMixedContentAudio, WebFeature::kMixedContentImage,
          WebFeature::kMixedContentVideo, WebFeature::kMixedContentPlugin,
          WebFeature::kOpenerNavigationWithoutGesture,
          WebFeature::kUsbRequestDevice, WebFeature::kXMLHttpRequestSynchronous,
          WebFeature::kPaymentHandler,
          WebFeature::kPaymentRequestShowWithoutGesture,
          WebFeature::kHTMLImports, WebFeature::kHTMLImportsHasStyleSheets,
          WebFeature::kElementCreateShadowRoot,
          WebFeature::kDocumentRegisterElement,
          WebFeature::kCredentialManagerCreatePublicKeyCredential,
          WebFeature::kCredentialManagerGetPublicKeyCredential,
          WebFeature::kCredentialManagerMakePublicKeyCredentialSuccess,
          WebFeature::kCredentialManagerGetPublicKeyCredentialSuccess,
          WebFeature::kV8AudioContext_Constructor,
      }));
  return opt_in_features.count(feature);
}
