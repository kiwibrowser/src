// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WebURLLoaderMock_h
#define WebURLLoaderMock_h

#include <memory>
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "third_party/blink/public/platform/web_url_error.h"
#include "third_party/blink/public/platform/web_url_loader.h"

namespace blink {

class WebData;
class WebURLLoaderClient;
class WebURLLoaderMockFactoryImpl;
class WebURLLoaderTestDelegate;
class WebURLRequest;
class WebURLResponse;

const int kRedirectResponseOverheadBytes = 300;

// A simple class for mocking WebURLLoader.
// If the WebURLLoaderMockFactory it is associated with has been configured to
// mock the request it gets, it serves the mocked resource.  Otherwise it just
// forwards it to the default loader.
class WebURLLoaderMock : public WebURLLoader {
 public:
  // This object becomes the owner of |default_loader|.
  WebURLLoaderMock(WebURLLoaderMockFactoryImpl* factory,
                   std::unique_ptr<WebURLLoader> default_loader);
  ~WebURLLoaderMock() override;

  // Simulates the asynchronous request being served.
  void ServeAsynchronousRequest(WebURLLoaderTestDelegate* delegate,
                                const WebURLResponse& response,
                                const WebData& data,
                                const base::Optional<WebURLError>& error);

  // Simulates the redirect being served.
  WebURL ServeRedirect(const WebURLRequest& request,
                       const WebURLResponse& redirect_response);

  // WebURLLoader methods:
  void LoadSynchronously(const WebURLRequest&,
                         WebURLLoaderClient* client,
                         WebURLResponse&,
                         base::Optional<WebURLError>&,
                         WebData&,
                         int64_t& encoded_data_length,
                         int64_t& encoded_body_length,
                         base::Optional<int64_t>& downloaded_file_length,
                         blink::WebBlobInfo& downloaded_blob) override;
  void LoadAsynchronously(const WebURLRequest& request,
                          WebURLLoaderClient* client) override;
  void Cancel() override;
  void SetDefersLoading(bool defer) override;
  void DidChangePriority(WebURLRequest::Priority new_priority,
                         int intra_priority_value) override;

  bool is_deferred() { return is_deferred_; }
  bool is_cancelled() { return !client_; }

  base::WeakPtr<WebURLLoaderMock> GetWeakPtr();

 private:
  WebURLLoaderMockFactoryImpl* factory_ = nullptr;
  WebURLLoaderClient* client_ = nullptr;
  std::unique_ptr<WebURLLoader> default_loader_;
  bool using_default_loader_ = false;
  bool is_deferred_ = false;

  base::WeakPtrFactory<WebURLLoaderMock> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(WebURLLoaderMock);
};

}  // namespace blink

#endif  // WebURLLoaderMock_h
