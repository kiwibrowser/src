// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/printing/cloud_print/privet_local_printer_lister.h"

#include <stddef.h>
#include <stdint.h>

#include <memory>

#include "base/run_loop.h"
#include "base/threading/thread_task_runner_handle.h"
#include "chrome/browser/local_discovery/test_service_discovery_client.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "net/url_request/test_url_fetcher_factory.h"
#include "net/url_request/url_request_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using local_discovery::TestServiceDiscoveryClient;

using testing::StrictMock;
using testing::AtLeast;
using testing::_;

namespace cloud_print {

namespace {

const uint8_t kAnnouncePacket[] = {
    // Header
    0x00, 0x00,  // ID is zeroed out
    0x80, 0x00,  // Standard query response, no error
    0x00, 0x00,  // No questions (for simplicity)
    0x00, 0x05,  // 5 RR (answers)
    0x00, 0x00,  // 0 authority RRs
    0x00, 0x00,  // 0 additional RRs
    0x07, '_',  'p',  'r',  'i',  'v',  'e',  't',  0x04, '_',
    't',  'c',  'p',  0x05, 'l',  'o',  'c',  'a',  'l',  0x00,
    0x00, 0x0c,              // TYPE is PTR.
    0x00, 0x01,              // CLASS is IN.
    0x00, 0x00,              // TTL (4 bytes) is 32768 second.
    0x10, 0x00, 0x00, 0x0c,  // RDLENGTH is 12 bytes.
    0x09, 'm',  'y',  'S',  'e',  'r',  'v',  'i',  'c',  'e',
    0xc0, 0x0c, 0x09, 'm',  'y',  'S',  'e',  'r',  'v',  'i',
    'c',  'e',  0xc0, 0x0c, 0x00, 0x10,  // TYPE is TXT.
    0x00, 0x01,                          // CLASS is IN.
    0x00, 0x00,                          // TTL (4 bytes) is 32768 seconds.
    0x01, 0x00, 0x00, 0x44,              // RDLENGTH is 55 bytes.
    0x06, 'i',  'd',  '=',  'r',  'e',  'g',  0x10, 't',  'y',
    '=',  'S',  'a',  'm',  'p',  'l',  'e',  ' ',  'd',  'e',
    'v',  'i',  'c',  'e',  0x1e, 'n',  'o',  't',  'e',  '=',
    'S',  'a',  'm',  'p',  'l',  'e',  ' ',  'd',  'e',  'v',
    'i',  'c',  'e',  ' ',  'd',  'e',  's',  'c',  'r',  'i',
    'p',  't',  'i',  'o',  'n',  0x0c, 't',  'y',  'p',  'e',
    '=',  'p',  'r',  'i',  'n',  't',  'e',  'r',  0x09, 'm',
    'y',  'S',  'e',  'r',  'v',  'i',  'c',  'e',  0xc0, 0x0c,
    0x00, 0x21,                          // Type is SRV
    0x00, 0x01,                          // CLASS is IN
    0x00, 0x00,                          // TTL (4 bytes) is 32768 second.
    0x10, 0x00, 0x00, 0x17,              // RDLENGTH is 23
    0x00, 0x00, 0x00, 0x00, 0x22, 0xb8,  // port 8888
    0x09, 'm',  'y',  'S',  'e',  'r',  'v',  'i',  'c',  'e',
    0x05, 'l',  'o',  'c',  'a',  'l',  0x00, 0x09, 'm',  'y',
    'S',  'e',  'r',  'v',  'i',  'c',  'e',  0x05, 'l',  'o',
    'c',  'a',  'l',  0x00, 0x00, 0x01,  // Type is A
    0x00, 0x01,                          // CLASS is IN
    0x00, 0x00,                          // TTL (4 bytes) is 32768 second.
    0x10, 0x00, 0x00, 0x04,              // RDLENGTH is 4
    0x01, 0x02, 0x03, 0x04,              // 1.2.3.4
    0x09, 'm',  'y',  'S',  'e',  'r',  'v',  'i',  'c',  'e',
    0x05, 'l',  'o',  'c',  'a',  'l',  0x00, 0x00, 0x1C,  // Type is AAAA
    0x00, 0x01,                                            // CLASS is IN
    0x00, 0x00,              // TTL (4 bytes) is 32768 second.
    0x10, 0x00, 0x00, 0x10,  // RDLENGTH is 16
    0x01, 0x02, 0x03, 0x04,  // 1.2.3.4
    0x01, 0x02, 0x03, 0x04, 0x01, 0x02, 0x03, 0x04, 0x01, 0x02,
    0x03, 0x04,
};

const char kInfoIsLocalPrinter[] = "{"
    "\"api\" : [ \"/privet/printer/submitdoc\" ],"
    "\"x-privet-token\" : \"sample\""
    "}";

const char kInfoIsNotLocalPrinter[] = "{"
    "\"api\" : [ \"/privet/register\" ],"
    "\"x-privet-token\" : \"sample\""
    "}";

const char kServiceName[] = "myService._privet._tcp.local";

const char kPrivetInfoURL[] = "http://1.2.3.4:8888/privet/info";

class MockLocalPrinterListerDelegate
    : public PrivetLocalPrinterLister::Delegate {
 public:
  MockLocalPrinterListerDelegate() {}
  ~MockLocalPrinterListerDelegate() override {}

  MOCK_METHOD3(LocalPrinterChanged,
               void(const std::string& name,
                    bool has_local_printing,
                    const DeviceDescription& description));

  MOCK_METHOD1(LocalPrinterRemoved, void(const std::string& name));

  MOCK_METHOD0(LocalPrinterCacheFlushed, void());
};

class PrivetLocalPrinterListerTest : public testing::Test {
 public:
  PrivetLocalPrinterListerTest() {
    test_service_discovery_client_ = new TestServiceDiscoveryClient();
    test_service_discovery_client_->Start();
    url_request_context_ = new net::TestURLRequestContextGetter(
        base::ThreadTaskRunnerHandle::Get());
    local_printer_lister_.reset(new PrivetLocalPrinterLister(
        test_service_discovery_client_.get(),
        url_request_context_.get(),
        &delegate_));
  }

  ~PrivetLocalPrinterListerTest() override {}

  bool SuccessfulResponseToURL(const GURL& url,
                               const std::string& response) {
    net::TestURLFetcher* fetcher = fetcher_factory_.GetFetcherByID(0);
    EXPECT_TRUE(fetcher);
    EXPECT_EQ(url, fetcher->GetOriginalURL());

    if (!fetcher || url != fetcher->GetOriginalURL())
      return false;

    fetcher->SetResponseString(response);
    fetcher->set_status(net::URLRequestStatus(net::URLRequestStatus::SUCCESS,
                                              net::OK));
    fetcher->set_response_code(200);
    fetcher->delegate()->OnURLFetchComplete(fetcher);
    return true;
  }

  void SimulateReceive(const uint8_t* packet, size_t size) {
    test_service_discovery_client_->SimulateReceive(packet, size);
    base::RunLoop().RunUntilIdle();
  }

  void ExpectAnyPacket() {
    EXPECT_CALL(*test_service_discovery_client_.get(), OnSendTo(_))
        .Times(AtLeast(2));
  }

 protected:
  content::TestBrowserThreadBundle test_thread_bundle;
  scoped_refptr<TestServiceDiscoveryClient> test_service_discovery_client_;
  scoped_refptr<net::TestURLRequestContextGetter> url_request_context_;
  std::unique_ptr<PrivetLocalPrinterLister> local_printer_lister_;
  net::TestURLFetcherFactory fetcher_factory_;
  StrictMock<MockLocalPrinterListerDelegate> delegate_;
};

TEST_F(PrivetLocalPrinterListerTest, PrinterAddedTest) {
  ExpectAnyPacket();

  local_printer_lister_->Start();

  SimulateReceive(kAnnouncePacket, sizeof(kAnnouncePacket));

  EXPECT_CALL(delegate_, LocalPrinterChanged(kServiceName, true, _));

  EXPECT_TRUE(SuccessfulResponseToURL(
      GURL(kPrivetInfoURL),
      std::string(kInfoIsLocalPrinter)));
}

TEST_F(PrivetLocalPrinterListerTest, NonPrinterAddedTest) {
  ExpectAnyPacket();

  local_printer_lister_->Start();

  SimulateReceive(kAnnouncePacket, sizeof(kAnnouncePacket));

  EXPECT_CALL(delegate_, LocalPrinterChanged(kServiceName, false, _));

  EXPECT_TRUE(SuccessfulResponseToURL(
      GURL(kPrivetInfoURL),
      std::string(kInfoIsNotLocalPrinter)));
}

}  // namespace

}  // namespace cloud_print
