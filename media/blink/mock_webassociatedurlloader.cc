// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/blink/mock_webassociatedurlloader.h"

#include "third_party/blink/public/platform/web_data.h"
#include "third_party/blink/public/platform/web_url_error.h"
#include "third_party/blink/public/platform/web_url_request.h"
#include "third_party/blink/public/platform/web_url_response.h"

namespace media {

MockWebAssociatedURLLoader::MockWebAssociatedURLLoader() = default;

MockWebAssociatedURLLoader::~MockWebAssociatedURLLoader() = default;

}  // namespace media
