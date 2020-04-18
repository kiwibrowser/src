// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_TEST_FAKE_SERVER_FAKE_SERVER_HTTP_POST_PROVIDER_H_
#define COMPONENTS_SYNC_TEST_FAKE_SERVER_FAKE_SERVER_HTTP_POST_PROVIDER_H_

#include <string>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/sequenced_task_runner.h"
#include "base/synchronization/waitable_event.h"
#include "components/sync/engine/net/http_post_provider_factory.h"
#include "components/sync/engine/net/http_post_provider_interface.h"

namespace fake_server {

class FakeServer;

class FakeServerHttpPostProvider
    : public syncer::HttpPostProviderInterface,
      public base::RefCountedThreadSafe<FakeServerHttpPostProvider> {
 public:
  FakeServerHttpPostProvider(
      const base::WeakPtr<FakeServer>& fake_server,
      scoped_refptr<base::SequencedTaskRunner> fake_server_task_runner);

  // HttpPostProviderInterface implementation.
  void SetExtraRequestHeaders(const char* headers) override;
  void SetURL(const char* url, int port) override;
  void SetPostPayload(const char* content_type,
                      int content_length,
                      const char* content) override;
  bool MakeSynchronousPost(int* error_code, int* response_code) override;
  void Abort() override;
  int GetResponseContentLength() const override;
  const char* GetResponseContent() const override;
  const std::string GetResponseHeaderValue(
      const std::string& name) const override;

 protected:
  ~FakeServerHttpPostProvider() override;

 private:
  friend class base::RefCountedThreadSafe<FakeServerHttpPostProvider>;

  // |fake_server_| should only be dereferenced on the same thread as
  // |fake_server_task_runner_| runs on.
  base::WeakPtr<FakeServer> fake_server_;
  scoped_refptr<base::SequencedTaskRunner> fake_server_task_runner_;

  std::string response_;
  std::string request_url_;
  int request_port_;
  std::string request_content_;
  std::string request_content_type_;
  std::string extra_request_headers_;
  int post_error_code_;
  int post_response_code_;

  DISALLOW_COPY_AND_ASSIGN(FakeServerHttpPostProvider);
};

class FakeServerHttpPostProviderFactory
    : public syncer::HttpPostProviderFactory {
 public:
  FakeServerHttpPostProviderFactory(
      const base::WeakPtr<FakeServer>& fake_server,
      scoped_refptr<base::SequencedTaskRunner> fake_server_task_runner);
  ~FakeServerHttpPostProviderFactory() override;

  // HttpPostProviderFactory:
  void Init(
      const std::string& user_agent,
      const syncer::BindToTrackerCallback& bind_to_tracker_callback) override;
  syncer::HttpPostProviderInterface* Create() override;
  void Destroy(syncer::HttpPostProviderInterface* http) override;

 private:
  // |fake_server_| should only be dereferenced on the same thread as
  // |fake_server_task_runner_| runs on.
  base::WeakPtr<FakeServer> fake_server_;
  scoped_refptr<base::SequencedTaskRunner> fake_server_task_runner_;

  DISALLOW_COPY_AND_ASSIGN(FakeServerHttpPostProviderFactory);
};

}  // namespace fake_server

#endif  // COMPONENTS_SYNC_TEST_FAKE_SERVER_FAKE_SERVER_HTTP_POST_PROVIDER_H_
