// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_ASH_ASSISTANT_ASSISTANT_IMAGE_DOWNLOADER_H_
#define CHROME_BROWSER_UI_ASH_ASSISTANT_ASSISTANT_IMAGE_DOWNLOADER_H_

#include "ash/public/interfaces/assistant_image_downloader.mojom.h"
#include "base/macros.h"
#include "mojo/public/cpp/bindings/binding.h"

class AccountId;

namespace service_manager {
class Connector;
}  // namespace service_manager

// AssistantImageDownloader is the class responsible for downloading images on
// behalf of Assistant UI in ash.
class AssistantImageDownloader : public ash::mojom::AssistantImageDownloader {
 public:
  explicit AssistantImageDownloader(service_manager::Connector* connector);
  ~AssistantImageDownloader() override;

  // ash::mojom::AssistantImageDownloader:
  void Download(
      const AccountId& account_id,
      const GURL& url,
      ash::mojom::AssistantImageDownloader::DownloadCallback callback) override;

 private:
  mojo::Binding<ash::mojom::AssistantImageDownloader> binding_;

  DISALLOW_COPY_AND_ASSIGN(AssistantImageDownloader);
};

#endif  // CHROME_BROWSER_UI_ASH_ASSISTANT_ASSISTANT_IMAGE_DOWNLOADER_H_
