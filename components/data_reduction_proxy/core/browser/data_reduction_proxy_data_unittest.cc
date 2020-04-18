// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/data_reduction_proxy/core/browser/data_reduction_proxy_data.h"

#include <stdint.h>

#include <memory>
#include <string>

#include "base/message_loop/message_loop.h"
#include "net/base/request_priority.h"
#include "net/nqe/effective_connection_type.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_context.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace data_reduction_proxy {

namespace {

class DataReductionProxyDataTest : public testing::Test {
 public:
  DataReductionProxyDataTest() {}

 private:
  base::MessageLoopForIO message_loop_;
};

TEST_F(DataReductionProxyDataTest, BasicSettersAndGetters) {
  std::unique_ptr<DataReductionProxyData> data(new DataReductionProxyData());
  EXPECT_FALSE(data->used_data_reduction_proxy());
  data->set_used_data_reduction_proxy(true);
  EXPECT_TRUE(data->used_data_reduction_proxy());
  data->set_used_data_reduction_proxy(false);
  EXPECT_FALSE(data->used_data_reduction_proxy());

  EXPECT_FALSE(data->lofi_requested());
  data->set_lofi_requested(true);
  EXPECT_TRUE(data->lofi_requested());
  data->set_lofi_requested(false);
  EXPECT_FALSE(data->lofi_requested());

  EXPECT_FALSE(data->lite_page_received());
  data->set_lite_page_received(true);
  EXPECT_TRUE(data->lite_page_received());
  data->set_lite_page_received(false);
  EXPECT_FALSE(data->lite_page_received());

  EXPECT_FALSE(data->lofi_received());
  data->set_lofi_received(true);
  EXPECT_TRUE(data->lofi_received());
  data->set_lofi_received(false);
  EXPECT_FALSE(data->lofi_received());

  EXPECT_EQ(std::string(), data->session_key());
  std::string session_key = "test-key";
  data->set_session_key(session_key);
  EXPECT_EQ(session_key, data->session_key());

  EXPECT_EQ(GURL(std::string()), data->request_url());
  GURL test_url("test-url");
  data->set_request_url(test_url);
  EXPECT_EQ(test_url, data->request_url());

  EXPECT_EQ(net::EFFECTIVE_CONNECTION_TYPE_UNKNOWN,
            data->effective_connection_type());
  data->set_effective_connection_type(net::EFFECTIVE_CONNECTION_TYPE_OFFLINE);
  EXPECT_EQ(net::EFFECTIVE_CONNECTION_TYPE_OFFLINE,
            data->effective_connection_type());

  EXPECT_FALSE(data->page_id());
  uint64_t page_id = 1;
  data->set_page_id(page_id);
  EXPECT_EQ(page_id, data->page_id().value());
}

TEST_F(DataReductionProxyDataTest, AddToURLRequest) {
  std::unique_ptr<net::URLRequestContext> context(new net::URLRequestContext());
  std::unique_ptr<net::URLRequest> fake_request(context->CreateRequest(
      GURL("http://www.google.com"), net::RequestPriority::IDLE, nullptr,
      TRAFFIC_ANNOTATION_FOR_TESTS));
  DataReductionProxyData* data = DataReductionProxyData::GetData(*fake_request);
  EXPECT_FALSE(data);
  data =
      DataReductionProxyData::GetDataAndCreateIfNecessary(fake_request.get());
  EXPECT_TRUE(data);
  data = DataReductionProxyData::GetData(*fake_request);
  EXPECT_TRUE(data);
  DataReductionProxyData* data2 =
      DataReductionProxyData::GetDataAndCreateIfNecessary(fake_request.get());
  EXPECT_EQ(data, data2);
}

TEST_F(DataReductionProxyDataTest, DeepCopy) {
  const struct {
    bool data_reduction_used;
    bool lofi_test_value;
  } tests[] = {
      {
          false, true,
      },
      {
          false, false,
      },
      {
          true, false,
      },
      {
          true, true,
      },
  };

  for (size_t i = 0; i < arraysize(tests); ++i) {
    static const char kSessionKey[] = "test-key";
    static const GURL kTestURL("test-url");
    std::unique_ptr<DataReductionProxyData> data(new DataReductionProxyData());
    data->set_used_data_reduction_proxy(tests[i].data_reduction_used);
    data->set_lofi_requested(tests[i].lofi_test_value);
    data->set_lite_page_received(tests[i].lofi_test_value);
    data->set_lofi_received(tests[i].lofi_test_value);
    data->set_session_key(kSessionKey);
    data->set_request_url(kTestURL);
    data->set_effective_connection_type(net::EFFECTIVE_CONNECTION_TYPE_OFFLINE);
    data->set_page_id(2u);
    std::unique_ptr<DataReductionProxyData> copy = data->DeepCopy();
    EXPECT_EQ(tests[i].lofi_test_value, copy->lofi_requested());
    EXPECT_EQ(tests[i].lofi_test_value, copy->lite_page_received());
    EXPECT_EQ(tests[i].lofi_test_value, copy->lofi_received());
    EXPECT_EQ(tests[i].data_reduction_used, copy->used_data_reduction_proxy());
    EXPECT_EQ(kSessionKey, copy->session_key());
    EXPECT_EQ(kTestURL, copy->request_url());
    EXPECT_EQ(net::EFFECTIVE_CONNECTION_TYPE_OFFLINE,
              copy->effective_connection_type());
    EXPECT_EQ(2u, data->page_id().value());
  }
}

TEST_F(DataReductionProxyDataTest, ClearData) {
  std::unique_ptr<net::URLRequestContext> context(new net::URLRequestContext());
  std::unique_ptr<net::URLRequest> fake_request(context->CreateRequest(
      GURL("http://www.google.com"), net::RequestPriority::IDLE, nullptr,
      TRAFFIC_ANNOTATION_FOR_TESTS));

  DataReductionProxyData* data =
      DataReductionProxyData::GetDataAndCreateIfNecessary(fake_request.get());
  EXPECT_TRUE(data);
  DataReductionProxyData::ClearData(fake_request.get());
  data = DataReductionProxyData::GetData(*fake_request);
  EXPECT_FALSE(data);
}

}  // namespace

}  // namespace data_reduction_proxy
