// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DATA_REDUCTION_PROXY_CONTENT_BROWSER_CONTENT_LOFI_UI_SERVICE_H_
#define COMPONENTS_DATA_REDUCTION_PROXY_CONTENT_BROWSER_CONTENT_LOFI_UI_SERVICE_H_

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "components/data_reduction_proxy/core/common/lofi_ui_service.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace content {
class WebContents;
}

namespace net {
class URLRequest;
}

namespace data_reduction_proxy {

using OnLoFiResponseReceivedCallback =
    base::Callback<void(content::WebContents* web_contents)>;

// Passes notifications to the UI thread that a Lo-Fi response has been
// received. These notifications may be used to show Lo-Fi UI. This object lives
// on the IO thread and OnLoFiReponseReceived should be called from there.
class ContentLoFiUIService : public LoFiUIService {
 public:
  ContentLoFiUIService(
      const scoped_refptr<base::SingleThreadTaskRunner>& ui_task_runner,
      const OnLoFiResponseReceivedCallback& on_lofi_response_received_callback);
  ~ContentLoFiUIService() override;

  // LoFiUIService implementation:
  void OnLoFiReponseReceived(const net::URLRequest& request) override;

 private:
  // Using the |render_process_id| and |render_frame_id|, gets the associated
  // WebContents if it exists and runs the
  // |notify_lofi_response_received_callback_|.
  void OnLoFiResponseReceivedOnUIThread(int render_process_id,
                                        int render_frame_id);

  // A task runner to post calls to OnLoFiReponseReceivedOnUI on the UI
  // thread.
  const scoped_refptr<base::SingleThreadTaskRunner> ui_task_runner_;
  const OnLoFiResponseReceivedCallback on_lofi_response_received_callback_;

  DISALLOW_COPY_AND_ASSIGN(ContentLoFiUIService);
};

}  // namespace data_reduction_proxy

#endif  // COMPONENTS_DATA_REDUCTION_PROXY_CONTENT_BROWSER_CONTENT_LOFI_UI_SERVICE_H_
