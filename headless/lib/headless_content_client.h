// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef HEADLESS_LIB_HEADLESS_CONTENT_CLIENT_H_
#define HEADLESS_LIB_HEADLESS_CONTENT_CLIENT_H_

#include "content/public/common/content_client.h"
#include "headless/public/headless_browser.h"

namespace headless {

class HeadlessContentClient : public content::ContentClient {
 public:
  explicit HeadlessContentClient(HeadlessBrowser::Options* options);
  ~HeadlessContentClient() override;

  // content::ContentClient implementation:
  std::string GetProduct() const override;
  std::string GetUserAgent() const override;
  base::string16 GetLocalizedString(int message_id) const override;
  base::StringPiece GetDataResource(
      int resource_id,
      ui::ScaleFactor scale_factor) const override;
  base::RefCountedMemory* GetDataResourceBytes(
      int resource_id) const override;
  gfx::Image& GetNativeImageNamed(int resource_id) const override;

 private:
  HeadlessBrowser::Options* options_;  // Not owned.

  DISALLOW_COPY_AND_ASSIGN(HeadlessContentClient);
};

}  // namespace headless

#endif  // HEADLESS_LIB_HEADLESS_CONTENT_CLIENT_H_
