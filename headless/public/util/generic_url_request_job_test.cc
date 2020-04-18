// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "headless/public/util/generic_url_request_job.h"

#include <memory>
#include <string>
#include <vector>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/message_loop/message_loop.h"
#include "base/message_loop/message_loop_current.h"
#include "base/run_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "headless/public/util/expedited_dispatcher.h"
#include "headless/public/util/testing/generic_url_request_mocks.h"
#include "headless/public/util/url_fetcher.h"
#include "net/base/completion_once_callback.h"
#include "net/base/elements_upload_data_stream.h"
#include "net/base/upload_bytes_element_reader.h"
#include "net/http/http_response_headers.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "net/url_request/url_request_job_factory_impl.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::AllOf;
using testing::ElementsAre;
using testing::Eq;
using testing::NotNull;
using testing::Property;
using testing::_;

std::ostream& operator<<(std::ostream& os, const base::DictionaryValue& value) {
  std::string json;
  base::JSONWriter::WriteWithOptions(
      value, base::JSONWriter::OPTIONS_PRETTY_PRINT, &json);
  os << json;
  return os;
}

MATCHER_P(MatchesJson, json, json.c_str()) {
  std::unique_ptr<base::Value> expected(
      base::JSONReader::Read(json, base::JSON_PARSE_RFC));
  return arg.Equals(expected.get());
}

namespace headless {

namespace {

class MockDelegate : public MockGenericURLRequestJobDelegate {
 public:
  MOCK_METHOD2(OnResourceLoadFailed,
               void(const Request* request, net::Error error));
};

class MockFetcher : public URLFetcher {
 public:
  MockFetcher(
      base::DictionaryValue* fetch_request,
      std::string* received_post_data,
      std::map<std::string, std::string>* json_fetch_reply_map,
      base::RepeatingCallback<void(const Request*)>* on_request_callback)
      : json_fetch_reply_map_(json_fetch_reply_map),
        fetch_request_(fetch_request),
        received_post_data_(received_post_data),
        on_request_callback_(on_request_callback) {}

  ~MockFetcher() override = default;

  void StartFetch(const Request* request,
                  ResultListener* result_listener) override {
    if (!on_request_callback_->is_null())
      on_request_callback_->Run(request);

    // Record the request.
    std::string url = request->GetURL().spec();
    fetch_request_->SetString("url", url);
    fetch_request_->SetString("method", request->GetMethod());
    std::unique_ptr<base::DictionaryValue> headers(new base::DictionaryValue);
    for (net::HttpRequestHeaders::Iterator it(request->GetHttpRequestHeaders());
         it.GetNext();) {
      headers->SetString(it.name(), it.value());
    }
    fetch_request_->Set("headers", std::move(headers));
    *received_post_data_ = request->GetPostData();
    if (!received_post_data_->empty() && received_post_data_->size() < 1024)
      fetch_request_->SetString("post_data", *received_post_data_);

    const auto find_it = json_fetch_reply_map_->find(url);
    if (find_it == json_fetch_reply_map_->end()) {
      result_listener->OnFetchStartError(net::ERR_ADDRESS_UNREACHABLE);
      return;
    }

    // Return the canned response.
    std::unique_ptr<base::Value> fetch_reply(
        base::JSONReader::Read(find_it->second, base::JSON_PARSE_RFC));
    CHECK(fetch_reply) << "Invalid json: " << find_it->second;

    base::DictionaryValue* reply_dictionary;
    ASSERT_TRUE(fetch_reply->GetAsDictionary(&reply_dictionary));
    base::DictionaryValue* reply_headers_dictionary;
    ASSERT_TRUE(
        reply_dictionary->GetDictionary("headers", &reply_headers_dictionary));
    scoped_refptr<net::HttpResponseHeaders> response_headers(
        new net::HttpResponseHeaders(""));
    for (base::DictionaryValue::Iterator it(*reply_headers_dictionary);
         !it.IsAtEnd(); it.Advance()) {
      response_headers->AddHeader(base::StringPrintf(
          "%s: %s", it.key().c_str(), it.value().GetString().c_str()));
    }

    // Set the fields needed for tracing, so that we can check
    // if they are forwarded correctly.
    net::LoadTimingInfo load_timing_info;
    load_timing_info.send_start = base::TimeTicks::Max();
    load_timing_info.receive_headers_end = base::TimeTicks::Max();
    const base::Value* final_url_value = reply_dictionary->FindKey("url");
    ASSERT_THAT(final_url_value, NotNull());
    const base::Value* response_data_value = reply_dictionary->FindKey("data");
    ASSERT_THAT(response_data_value, NotNull());
    response_data_ = response_data_value->GetString();
    const base::Value* total_received_bytes_value =
        reply_dictionary->FindKey("total_received_bytes");
    int total_received_bytes = 0;
    if (total_received_bytes_value)
      total_received_bytes = total_received_bytes_value->GetInt();
    result_listener->OnFetchComplete(
        GURL(final_url_value->GetString()), std::move(response_headers),
        response_data_.c_str(), response_data_.size(),
        scoped_refptr<net::IOBufferWithSize>(), load_timing_info,
        total_received_bytes);
  }

 private:
  std::map<std::string, std::string>* json_fetch_reply_map_;   // NOT OWNED
  base::DictionaryValue* fetch_request_;                       // NOT OWNED
  std::string* received_post_data_;                            // NOT OWNED
  base::RepeatingCallback<void(const Request*)>*
      on_request_callback_;    // NOT OWNED
  std::string response_data_;  // Here to ensure the required lifetime.
};

class MockProtocolHandler : public net::URLRequestJobFactory::ProtocolHandler {
 public:
  // Details of the fetch will be stored in |fetch_request|.
  // The fetch response will be created from parsing |json_fetch_reply_map|.
  MockProtocolHandler(
      base::DictionaryValue* fetch_request,
      std::string* received_post_data,
      std::map<std::string, std::string>* json_fetch_reply_map,
      URLRequestDispatcher* dispatcher,
      GenericURLRequestJob::Delegate* job_delegate,
      base::RepeatingCallback<void(const Request*)>* on_request_callback)
      : fetch_request_(fetch_request),
        received_post_data_(received_post_data),
        json_fetch_reply_map_(json_fetch_reply_map),
        job_delegate_(job_delegate),
        dispatcher_(dispatcher),
        on_request_callback_(on_request_callback) {}

  // net::URLRequestJobFactory::ProtocolHandler override.
  net::URLRequestJob* MaybeCreateJob(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate) const override {
    return new GenericURLRequestJob(
        request, network_delegate, dispatcher_,
        std::make_unique<MockFetcher>(fetch_request_, received_post_data_,
                                      json_fetch_reply_map_,
                                      on_request_callback_),
        job_delegate_, nullptr);
  }

 private:
  base::DictionaryValue* fetch_request_;                       // NOT OWNED
  std::string* received_post_data_;                            // NOT OWNED
  std::map<std::string, std::string>* json_fetch_reply_map_;   // NOT OWNED
  GenericURLRequestJob::Delegate* job_delegate_;               // NOT OWNED
  URLRequestDispatcher* dispatcher_;                           // NOT OWNED
  base::RepeatingCallback<void(const Request*)>*
      on_request_callback_;  // NOT OWNED
};

}  // namespace

class GenericURLRequestJobTest : public testing::Test {
 public:
  GenericURLRequestJobTest() : dispatcher_(message_loop_.task_runner()) {
    url_request_job_factory_.SetProtocolHandler(
        "https",
        base::WrapUnique(new MockProtocolHandler(
            &fetch_request_, &received_post_data_, &json_fetch_reply_map_,
            &dispatcher_, &job_delegate_, &on_request_callback_)));
    url_request_context_.set_job_factory(&url_request_job_factory_);
    url_request_context_.set_cookie_store(&cookie_store_);
  }

  std::unique_ptr<net::URLRequest> CreateAndCompleteGetJob(
      const GURL& url,
      const std::string& json_reply) {
    json_fetch_reply_map_[url.spec()] = json_reply;

    std::unique_ptr<net::URLRequest> request(url_request_context_.CreateRequest(
        url, net::DEFAULT_PRIORITY, &request_delegate_,
        TRAFFIC_ANNOTATION_FOR_TESTS));
    request->Start();
    base::RunLoop().RunUntilIdle();
    return request;
  }

  std::unique_ptr<net::URLRequest> CreateAndCompletePostJob(
      const GURL& url,
      const std::string& post_data,
      const std::string& json_reply) {
    json_fetch_reply_map_[url.spec()] = json_reply;

    std::unique_ptr<net::URLRequest> request(url_request_context_.CreateRequest(
        url, net::DEFAULT_PRIORITY, &request_delegate_,
        TRAFFIC_ANNOTATION_FOR_TESTS));
    request->set_method("POST");
    request->set_upload(net::ElementsUploadDataStream::CreateWithReader(
        std::make_unique<net::UploadBytesElementReader>(post_data.data(),
                                                        post_data.size()),
        0));
    request->Start();
    base::RunLoop().RunUntilIdle();
    return request;
  }

 protected:
  base::MessageLoop message_loop_;
  ExpeditedDispatcher dispatcher_;

  net::URLRequestJobFactoryImpl url_request_job_factory_;
  net::URLRequestContext url_request_context_;
  MockCookieStore cookie_store_;

  MockURLRequestDelegate request_delegate_;
  base::DictionaryValue fetch_request_;  // The request sent to MockFetcher.
  std::string received_post_data_;       // The POST data (useful if large).
  std::map<std::string, std::string>
      json_fetch_reply_map_;  // Replies to be sent by MockFetcher.
  MockDelegate job_delegate_;
  base::RepeatingCallback<void(const Request*)> on_request_callback_;
};

TEST_F(GenericURLRequestJobTest, BasicGetRequestParams) {

  json_fetch_reply_map_["https://example.com/"] = R"(
      {
        "url": "https://example.com",
        "data": "Reply",
        "headers": {
          "Content-Type": "text/html; charset=UTF-8"
        }
      })";

  std::unique_ptr<net::URLRequest> request(url_request_context_.CreateRequest(
      GURL("https://example.com"), net::DEFAULT_PRIORITY, &request_delegate_,
      TRAFFIC_ANNOTATION_FOR_TESTS));
  request->SetReferrer("https://referrer.example.com");
  request->SetExtraRequestHeaderByName("Extra-Header", "Value", true);
  request->Start();
  base::RunLoop().RunUntilIdle();

  std::string expected_request_json = R"(
      {
        "url": "https://example.com/",
        "method": "GET",
        "headers": {
          "Extra-Header": "Value",
          "Referer": "https://referrer.example.com/"
        }
      })";

  EXPECT_THAT(fetch_request_, MatchesJson(expected_request_json));
}

TEST_F(GenericURLRequestJobTest, BasicPostRequestParams) {
  json_fetch_reply_map_["https://example.com/"] = R"(
      {
        "url": "https://example.com",
        "data": "Reply",
        "headers": {
          "Content-Type": "text/html; charset=UTF-8"
        }
      })";

  std::unique_ptr<net::URLRequest> request(url_request_context_.CreateRequest(
      GURL("https://example.com"), net::DEFAULT_PRIORITY, &request_delegate_,
      TRAFFIC_ANNOTATION_FOR_TESTS));
  request->SetReferrer("https://referrer.example.com");
  request->SetExtraRequestHeaderByName("Extra-Header", "Value", true);
  request->SetExtraRequestHeaderByName("User-Agent", "TestBrowser", true);
  request->SetExtraRequestHeaderByName("Accept", "text/plain", true);
  request->set_method("POST");

  std::string post_data = "lorem ipsom";
  request->set_upload(net::ElementsUploadDataStream::CreateWithReader(
      std::make_unique<net::UploadBytesElementReader>(post_data.data(),
                                                      post_data.size()),
      0));
  request->Start();
  base::RunLoop().RunUntilIdle();

  std::string expected_request_json = R"(
      {
        "url": "https://example.com/",
        "method": "POST",
        "post_data": "lorem ipsom",
        "headers": {
          "Accept": "text/plain",
          "Extra-Header": "Value",
          "Referer": "https://referrer.example.com/",
          "User-Agent": "TestBrowser"
        }
      })";

  EXPECT_THAT(fetch_request_, MatchesJson(expected_request_json));
}

TEST_F(GenericURLRequestJobTest, LargePostData) {
  json_fetch_reply_map_["https://example.com/"] = R"(
      {
        "url": "https://example.com",
        "data": "Reply",
        "headers": {
          "Content-Type": "text/html; charset=UTF-8"
        }
      })";

  std::unique_ptr<net::URLRequest> request(url_request_context_.CreateRequest(
      GURL("https://example.com"), net::DEFAULT_PRIORITY, &request_delegate_,
      TRAFFIC_ANNOTATION_FOR_TESTS));
  request->SetReferrer("https://referrer.example.com");
  request->SetExtraRequestHeaderByName("Extra-Header", "Value", true);
  request->SetExtraRequestHeaderByName("User-Agent", "TestBrowser", true);
  request->SetExtraRequestHeaderByName("Accept", "text/plain", true);
  request->set_method("POST");

  std::vector<char> post_data(4000000);
  for (size_t i = 0; i < post_data.size(); i++)
    post_data[i] = i & 127;

  request->set_upload(net::ElementsUploadDataStream::CreateWithReader(
      std::make_unique<net::UploadBytesElementReader>(&post_data[0],
                                                      post_data.size()),
      0));
  request->Start();
  base::RunLoop().RunUntilIdle();

  // Make sure we captured the expected post.
  for (size_t i = 0; i < received_post_data_.size(); i++) {
    EXPECT_EQ(static_cast<char>(i & 127), post_data[i]);
  }

  EXPECT_EQ(post_data.size(), received_post_data_.size());
}

TEST_F(GenericURLRequestJobTest, BasicRequestProperties) {
  std::string reply = R"(
      {
        "url": "https://example.com",
        "data": "Reply",
        "headers": {
          "Content-Type": "text/html; charset=UTF-8"
        }
      })";

  std::unique_ptr<net::URLRequest> request(
      CreateAndCompleteGetJob(GURL("https://example.com"), reply));

  EXPECT_EQ(200, request->GetResponseCode());

  std::string mime_type;
  request->GetMimeType(&mime_type);
  EXPECT_EQ("text/html", mime_type);

  std::string charset;
  request->GetCharset(&charset);
  EXPECT_EQ("utf-8", charset);

  std::string content_type;
  EXPECT_TRUE(request->response_info().headers->GetNormalizedHeader(
      "Content-Type", &content_type));
  EXPECT_EQ("text/html; charset=UTF-8", content_type);
}

TEST_F(GenericURLRequestJobTest, BasicRequestContents) {
  std::string reply = R"(
      {
        "url": "https://example.com",
        "data": "Reply",
        "headers": {
          "Content-Type": "text/html; charset=UTF-8"
        },
        "total_received_bytes": 100
      })";

  std::unique_ptr<net::URLRequest> request(
      CreateAndCompleteGetJob(GURL("https://example.com"), reply));

  const int kBufferSize = 256;
  scoped_refptr<net::IOBuffer> buffer(new net::IOBuffer(kBufferSize));
  int bytes_read;
  EXPECT_TRUE(request->Read(buffer.get(), kBufferSize, &bytes_read));
  EXPECT_EQ(5, bytes_read);
  EXPECT_EQ("Reply", std::string(buffer->data(), 5));
  EXPECT_EQ(100, request->GetTotalReceivedBytes());

  net::LoadTimingInfo load_timing_info;
  request->GetLoadTimingInfo(&load_timing_info);
  // Check that the send_start and receive_headers_end timings are
  // forwarded correctly, as they are used by tracing.
  EXPECT_EQ(base::TimeTicks::Max(), load_timing_info.send_start);
  EXPECT_EQ(base::TimeTicks::Max(), load_timing_info.receive_headers_end);
}

TEST_F(GenericURLRequestJobTest, ReadInParts) {
  std::string reply = R"(
      {
        "url": "https://example.com",
        "data": "Reply",
        "headers": {
          "Content-Type": "text/html; charset=UTF-8"
        }
      })";

  std::unique_ptr<net::URLRequest> request(
      CreateAndCompleteGetJob(GURL("https://example.com"), reply));

  const int kBufferSize = 3;
  scoped_refptr<net::IOBuffer> buffer(new net::IOBuffer(kBufferSize));
  int bytes_read;
  EXPECT_TRUE(request->Read(buffer.get(), kBufferSize, &bytes_read));
  EXPECT_EQ(3, bytes_read);
  EXPECT_EQ("Rep", std::string(buffer->data(), bytes_read));

  EXPECT_TRUE(request->Read(buffer.get(), kBufferSize, &bytes_read));
  EXPECT_EQ(2, bytes_read);
  EXPECT_EQ("ly", std::string(buffer->data(), bytes_read));

  EXPECT_TRUE(request->Read(buffer.get(), kBufferSize, &bytes_read));
  EXPECT_EQ(0, bytes_read);
}

TEST_F(GenericURLRequestJobTest, RequestWithCookies) {
  net::CookieList* cookies = cookie_store_.cookies();

  // Basic matching cookie.
  cookies->push_back(net::CanonicalCookie(
      "basic_cookie", "1", ".example.com", "/", base::Time(), base::Time(),
      base::Time(),
      /* secure */ false,
      /* http_only */ false, net::CookieSameSite::NO_RESTRICTION,
      net::COOKIE_PRIORITY_DEFAULT));

  // Matching secure cookie.
  cookies->push_back(net::CanonicalCookie(
      "secure_cookie", "2", ".example.com", "/", base::Time(), base::Time(),
      base::Time(),
      /* secure */ true,
      /* http_only */ false, net::CookieSameSite::NO_RESTRICTION,
      net::COOKIE_PRIORITY_DEFAULT));

  // Matching http-only cookie.
  cookies->push_back(net::CanonicalCookie(
      "http_only_cookie", "3", ".example.com", "/", base::Time(), base::Time(),
      base::Time(),
      /* secure */ false,
      /* http_only */ true, net::CookieSameSite::NO_RESTRICTION,
      net::COOKIE_PRIORITY_DEFAULT));

  // Matching cookie with path.
  cookies->push_back(net::CanonicalCookie(
      "cookie_with_path", "4", ".example.com", "/widgets", base::Time(),
      base::Time(), base::Time(),
      /* secure */ false,
      /* http_only */ false, net::CookieSameSite::NO_RESTRICTION,
      net::COOKIE_PRIORITY_DEFAULT));

  // Matching cookie with subdomain.
  cookies->push_back(net::CanonicalCookie(
      "bad_subdomain_cookie", "5", ".cdn.example.com", "/", base::Time(),
      base::Time(), base::Time(),
      /* secure */ false,
      /* http_only */ false, net::CookieSameSite::NO_RESTRICTION,
      net::COOKIE_PRIORITY_DEFAULT));

  // Non-matching cookie (different site).
  cookies->push_back(net::CanonicalCookie(
      "bad_site_cookie", "6", ".zombo.com", "/", base::Time(), base::Time(),
      base::Time(),
      /* secure */ false,
      /* http_only */ false, net::CookieSameSite::NO_RESTRICTION,
      net::COOKIE_PRIORITY_DEFAULT));

  // Non-matching cookie (different path).
  cookies->push_back(net::CanonicalCookie(
      "bad_path_cookie", "7", ".example.com", "/gadgets", base::Time(),
      base::Time(), base::Time(),
      /* secure */ false,
      /* http_only */ false, net::CookieSameSite::NO_RESTRICTION,
      net::COOKIE_PRIORITY_DEFAULT));

  std::string reply = R"(
      {
        "url": "https://example.com",
        "data": "Reply",
        "headers": {
          "Content-Type": "text/html; charset=UTF-8"
        }
      })";

  std::unique_ptr<net::URLRequest> request(
      CreateAndCompleteGetJob(GURL("https://example.com"), reply));

  std::string expected_request_json = R"(
      {
        "url": "https://example.com/",
        "method": "GET",
        "headers": {
          "Cookie": "basic_cookie=1; secure_cookie=2; http_only_cookie=3"
        }
      })";

  EXPECT_THAT(fetch_request_, MatchesJson(expected_request_json));
}

TEST_F(GenericURLRequestJobTest, ResponseWithCookies) {
  std::string reply = R"(
      {
        "url": "https://example.com",
        "data": "Reply",
        "headers": {
          "Set-Cookie": "A=foobar; path=/; "
        }
      })";

  std::unique_ptr<net::URLRequest> request(
      CreateAndCompleteGetJob(GURL("https://example.com"), reply));

  EXPECT_THAT(*cookie_store_.cookies(),
              ElementsAre(AllOf(
                  Property(&net::CanonicalCookie::Name, Eq("A")),
                  Property(&net::CanonicalCookie::Value, Eq("foobar")),
                  Property(&net::CanonicalCookie::Domain, Eq("example.com")),
                  Property(&net::CanonicalCookie::Path, Eq("/")))));
}

TEST_F(GenericURLRequestJobTest, OnResourceLoadFailed) {
  EXPECT_CALL(job_delegate_,
              OnResourceLoadFailed(_, net::ERR_ADDRESS_UNREACHABLE));

  std::unique_ptr<net::URLRequest> request(url_request_context_.CreateRequest(
      GURL("https://i-dont-exist.com"), net::DEFAULT_PRIORITY,
      &request_delegate_, TRAFFIC_ANNOTATION_FOR_TESTS));
  request->Start();
  base::RunLoop().RunUntilIdle();
}

TEST_F(GenericURLRequestJobTest, RequestsHaveDistinctIds) {
  std::string reply = R"(
      {
        "url": "https://example.com",
        "http_response_code": 200,
        "data": "Reply",
        "headers": {
          "Content-Type": "text/html; charset=UTF-8"
        }
      })";

  std::set<uint64_t> ids;
  on_request_callback_ = base::Bind(
      [](std::set<uint64_t>* ids, const Request* request) {
        ids->insert(request->GetRequestId());
      },
      &ids);

  CreateAndCompleteGetJob(GURL("https://example.com"), reply);
  CreateAndCompleteGetJob(GURL("https://example.com"), reply);
  CreateAndCompleteGetJob(GURL("https://example.com"), reply);

  // We expect three distinct ids.
  EXPECT_EQ(3u, ids.size());
}

TEST_F(GenericURLRequestJobTest, GetPostData) {
  std::string reply = R"(
      {
        "url": "https://example.com",
        "http_response_code": 200,
        "data": "Reply",
        "headers": {
          "Content-Type": "text/html; charset=UTF-8"
        }
      })";

  std::string post_data;
  uint64_t post_data_size;
  on_request_callback_ = base::Bind(
      [](std::string* post_data, uint64_t* post_data_size,
         const Request* request) {
        *post_data = request->GetPostData();
        *post_data_size = request->GetPostDataSize();
      },
      &post_data, &post_data_size);

  CreateAndCompletePostJob(GURL("https://example.com"), "payload", reply);

  EXPECT_EQ("payload", post_data);
  EXPECT_EQ(post_data_size, post_data.size());
}

namespace {
class ByteAtATimeUploadElementReader : public net::UploadElementReader {
 public:
  explicit ByteAtATimeUploadElementReader(const std::string& content)
      : content_(content) {}

  // net::UploadElementReader implementation:
  int Init(net::CompletionOnceCallback callback) override {
    offset_ = 0;
    return net::OK;
  }

  uint64_t GetContentLength() const override { return content_.size(); }

  uint64_t BytesRemaining() const override { return content_.size() - offset_; }

  bool IsInMemory() const override { return false; }

  int Read(net::IOBuffer* buf,
           int buf_length,
           net::CompletionOnceCallback callback) override {
    if (!BytesRemaining())
      return net::OK;

    base::MessageLoopCurrent::Get()->task_runner()->PostTask(
        FROM_HERE,
        base::BindOnce(&ByteAtATimeUploadElementReader::ReadImpl,
                       base::Unretained(this), base::WrapRefCounted(buf),
                       buf_length, std::move(callback)));
    return net::ERR_IO_PENDING;
  }

 private:
  void ReadImpl(scoped_refptr<net::IOBuffer> buf,
                int buf_length,
                net::CompletionOnceCallback callback) {
    if (BytesRemaining()) {
      *buf->data() = content_[offset_++];
      std::move(callback).Run(1u);
    } else {
      std::move(callback).Run(0u);
    }
  }

  std::string content_;
  uint64_t offset_ = 0;
};
}  // namespace

TEST_F(GenericURLRequestJobTest, GetPostDataAsync) {
  std::string json_reply = R"(
      {
        "url": "https://example.com",
        "http_response_code": 200,
        "data": "Reply",
        "headers": {
          "Content-Type": "text/html; charset=UTF-8"
        }
      })";

  std::string post_data;
  uint64_t post_data_size;
  on_request_callback_ = base::Bind(
      [](std::string* post_data, uint64_t* post_data_size,
         const Request* request) {
        *post_data = request->GetPostData();
        *post_data_size = request->GetPostDataSize();
      },
      &post_data, &post_data_size);

  GURL url("https://example.com");
  std::unique_ptr<net::URLRequest> request(url_request_context_.CreateRequest(
      url, net::DEFAULT_PRIORITY, &request_delegate_,
      TRAFFIC_ANNOTATION_FOR_TESTS));
  request->set_method("POST");

  json_fetch_reply_map_[url.spec()] = json_reply;

  request->set_upload(net::ElementsUploadDataStream::CreateWithReader(
      std::make_unique<ByteAtATimeUploadElementReader>("payload"), 0));
  request->Start();
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ("payload", post_data);
  EXPECT_EQ(post_data_size, post_data.size());
}

TEST_F(GenericURLRequestJobTest, LargePostDataNotByteReader) {
  json_fetch_reply_map_["https://example.com/"] = R"(
      {
        "url": "https://example.com",
        "data": "Reply",
        "headers": {
          "Content-Type": "text/html; charset=UTF-8"
        }
      })";

  std::unique_ptr<net::URLRequest> request(url_request_context_.CreateRequest(
      GURL("https://example.com"), net::DEFAULT_PRIORITY, &request_delegate_,
      TRAFFIC_ANNOTATION_FOR_TESTS));
  request->SetReferrer("https://referrer.example.com");
  request->SetExtraRequestHeaderByName("Extra-Header", "Value", true);
  request->SetExtraRequestHeaderByName("User-Agent", "TestBrowser", true);
  request->SetExtraRequestHeaderByName("Accept", "text/plain", true);
  request->set_method("POST");

  std::string post_data;
  post_data.reserve(4000000);
  for (size_t i = 0; i < post_data.size(); i++)
    post_data.at(i) = i & 127;

  request->set_upload(net::ElementsUploadDataStream::CreateWithReader(
      std::make_unique<ByteAtATimeUploadElementReader>(post_data), 0));
  request->Start();
  base::RunLoop().RunUntilIdle();

  // Make sure we captured the expected post.
  for (size_t i = 0; i < received_post_data_.size(); i++) {
    EXPECT_EQ(static_cast<char>(i & 127), post_data[i]);
  }

  EXPECT_EQ(post_data.size(), received_post_data_.size());
}

TEST_F(GenericURLRequestJobTest, PostWithMultipleElements) {
  json_fetch_reply_map_["https://example.com/"] = R"(
      {
        "url": "https://example.com",
        "data": "Reply",
        "headers": {
          "Content-Type": "text/html; charset=UTF-8"
        }
      })";

  std::unique_ptr<net::URLRequest> request(url_request_context_.CreateRequest(
      GURL("https://example.com"), net::DEFAULT_PRIORITY, &request_delegate_,
      TRAFFIC_ANNOTATION_FOR_TESTS));
  request->SetReferrer("https://referrer.example.com");
  request->SetExtraRequestHeaderByName("Extra-Header", "Value", true);
  request->SetExtraRequestHeaderByName("User-Agent", "TestBrowser", true);
  request->SetExtraRequestHeaderByName("Accept", "text/plain", true);
  request->set_method("POST");

  std::vector<std::unique_ptr<net::UploadElementReader>> element_readers;
  element_readers.push_back(
      std::make_unique<ByteAtATimeUploadElementReader>("Does "));
  element_readers.push_back(
      std::make_unique<ByteAtATimeUploadElementReader>("this "));
  element_readers.push_back(
      std::make_unique<ByteAtATimeUploadElementReader>("work?"));

  request->set_upload(std::make_unique<net::ElementsUploadDataStream>(
      std::move(element_readers), 0));
  request->Start();
  base::RunLoop().RunUntilIdle();

  EXPECT_EQ("Does this work?", received_post_data_);
}

}  // namespace headless
