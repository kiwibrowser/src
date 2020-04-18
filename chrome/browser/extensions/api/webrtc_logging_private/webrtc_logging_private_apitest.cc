// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include <memory>
#include <utility>

#include "base/command_line.h"
#include "base/json/json_writer.h"
#include "base/strings/string_split.h"
#include "base/strings/stringprintf.h"
#include "base/test/scoped_command_line.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/extensions/api/webrtc_logging_private/webrtc_logging_private_api.h"
#include "chrome/browser/extensions/extension_apitest.h"
#include "chrome/browser/extensions/extension_function_test_utils.h"
#include "chrome/browser/extensions/extension_tab_util.h"
#include "chrome/browser/media/webrtc/webrtc_event_log_manager_common.h"
#include "chrome/browser/media/webrtc/webrtc_log_uploader.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/chrome_switches.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/webrtc_event_logger.h"
#include "content/public/test/test_utils.h"
#include "extensions/common/extension_builder.h"
#include "third_party/zlib/google/compression_utils.h"

using compression::GzipUncompress;
using extensions::Extension;
using extensions::WebrtcLoggingPrivateDiscardFunction;
using extensions::WebrtcLoggingPrivateSetMetaDataFunction;
using extensions::WebrtcLoggingPrivateStartFunction;
using extensions::WebrtcLoggingPrivateStartRtpDumpFunction;
using extensions::WebrtcLoggingPrivateStopFunction;
using extensions::WebrtcLoggingPrivateStopRtpDumpFunction;
using extensions::WebrtcLoggingPrivateStoreFunction;
using extensions::WebrtcLoggingPrivateUploadFunction;
using extensions::WebrtcLoggingPrivateUploadStoredFunction;
using extensions::WebrtcLoggingPrivateStartAudioDebugRecordingsFunction;
using extensions::WebrtcLoggingPrivateStopAudioDebugRecordingsFunction;
using extensions::WebrtcLoggingPrivateStartEventLoggingFunction;

namespace utils = extension_function_test_utils;

namespace {

static const char kTestLoggingSessionIdKey[] = "app_session_id";
static const char kTestLoggingSessionIdValue[] = "0123456789abcdef";
static const char kTestLoggingUrl[] = "dummy url string";

std::string ParamsToString(const base::ListValue& parameters) {
  std::string parameter_string;
  EXPECT_TRUE(base::JSONWriter::Write(parameters, &parameter_string));
  return parameter_string;
}

void InitializeTestMetaData(base::ListValue* parameters) {
  std::unique_ptr<base::DictionaryValue> meta_data_entry(
      new base::DictionaryValue());
  meta_data_entry->SetString("key", kTestLoggingSessionIdKey);
  meta_data_entry->SetString("value", kTestLoggingSessionIdValue);
  std::unique_ptr<base::ListValue> meta_data(new base::ListValue());
  meta_data->Append(std::move(meta_data_entry));
  meta_data_entry.reset(new base::DictionaryValue());
  meta_data_entry->SetString("key", "url");
  meta_data_entry->SetString("value", kTestLoggingUrl);
  meta_data->Append(std::move(meta_data_entry));
  parameters->Append(std::move(meta_data));
}

class WebrtcLoggingPrivateApiTest : public extensions::ExtensionApiTest {
 protected:
  void SetUp() override {
    auto* cl = scoped_command_line_.GetProcessCommandLine();
    cl->AppendSwitchASCII(::switches::kWebRtcRemoteEventLog, "enabled");
    extensions::ExtensionApiTest::SetUp();
    extension_ = extensions::ExtensionBuilder("Test").Build();
  }

  template<typename T>
  scoped_refptr<T> CreateFunction() {
    scoped_refptr<T> function(new T());
    function->set_extension(extension_.get());
    function->set_has_callback(true);
    return function;
  }

  content::WebContents* web_contents() {
    return browser()->tab_strip_model()->GetActiveWebContents();
  }

  void AppendTabIdAndUrl(base::ListValue* parameters) {
    std::unique_ptr<base::DictionaryValue> request_info(
        new base::DictionaryValue());
    request_info->SetInteger(
        "tabId", extensions::ExtensionTabUtil::GetTabId(web_contents()));
    parameters->Append(std::move(request_info));
    parameters->AppendString(web_contents()->GetURL().GetOrigin().spec());
  }

  // This function implicitly expects the function to succeed (test failure
  // initiated otherwise).
  // Returns whether the function that was run returned a value (NOT whether
  // it had succeeded or failed).
  // TODO(crbug.com/829419): Return success/failure of the executed function.
  bool RunFunction(UIThreadExtensionFunction* function,
                   const base::ListValue& parameters) {
    std::unique_ptr<base::Value> result(utils::RunFunctionAndReturnSingleResult(
        function, ParamsToString(parameters), browser()));
    return (result != nullptr);
  }

  // This function implicitly expects the function to succeed (test failure
  // initiated otherwise).
  // Returns whether the function that was run returned a value (NOT whether
  // it had succeeded or failed).
  // TODO(crbug.com/829419): Return success/failure of the executed function.
  template <typename Function>
  bool RunFunction(const base::ListValue& parameters) {
    scoped_refptr<Function> function(CreateFunction<Function>());
    return RunFunction(function.get(), parameters);
  }

  // This function implicitly expects the function to succeed (test failure
  // initiated otherwise).
  // Returns whether the function that was run returned a value (NOT whether
  // it had succeeded or failed).
  // TODO(crbug.com/829419): Return success/failure of the executed function.
  template <typename Function>
  bool RunNoArgsFunction() {
    base::ListValue params;
    AppendTabIdAndUrl(&params);
    scoped_refptr<Function> function(CreateFunction<Function>());
    return RunFunction(function.get(), params);
  }

  template <typename Function>
  void RunFunctionAndExpectError(const base::ListValue& parameters,
                                 const std::string& expected_error) {
    DCHECK(!expected_error.empty());
    scoped_refptr<Function> function(CreateFunction<Function>());
    const std::string error_message = utils::RunFunctionAndReturnError(
        function.get(), ParamsToString(parameters), browser());
    EXPECT_EQ(error_message, expected_error);
  }

  // This function implicitly expects the function to succeed (test failure
  // initiated otherwise).
  // Returns whether the function that was run returned a value, or avoided
  // returning a value, according to expectation.
  // TODO(crbug.com/829419): Return success/failure of the executed function.
  bool StartLogging() {
    constexpr bool result_expected = false;
    const bool result_returned =
        RunNoArgsFunction<WebrtcLoggingPrivateStartFunction>();
    return (result_expected == result_returned);
  }

  // This function implicitly expects the function to succeed (test failure
  // initiated otherwise).
  // Returns whether the function that was run returned a value, or avoided
  // returning a value, according to expectation.
  // TODO(crbug.com/829419): Return success/failure of the executed function.
  bool StopLogging() {
    constexpr bool result_expected = false;
    const bool result_returned =
        RunNoArgsFunction<WebrtcLoggingPrivateStopFunction>();
    return (result_expected == result_returned);
  }

  // This function implicitly expects the function to succeed (test failure
  // initiated otherwise).
  // Returns whether the function that was run returned a value, or avoided
  // returning a value, according to expectation.
  // TODO(crbug.com/829419): Return success/failure of the executed function.
  bool DiscardLog() {
    constexpr bool result_expected = false;
    const bool result_returned =
        RunNoArgsFunction<WebrtcLoggingPrivateDiscardFunction>();
    return (result_expected == result_returned);
  }

  // This function implicitly expects the function to succeed (test failure
  // initiated otherwise).
  // Returns whether the function that was run returned a value, or avoided
  // returning a value, according to expectation.
  // TODO(crbug.com/829419): Return success/failure of the executed function.
  bool UploadLog() {
    constexpr bool result_expected = true;
    const bool result_returned =
        RunNoArgsFunction<WebrtcLoggingPrivateUploadFunction>();
    return (result_expected == result_returned);
  }

  // This function implicitly expects the function to succeed (test failure
  // initiated otherwise).
  // Returns whether the function that was run returned a value, or avoided
  // returning a value, according to expectation.
  // TODO(crbug.com/829419): Return success/failure of the executed function.
  bool SetMetaData(const base::ListValue& data) {
    constexpr bool result_expected = false;
    const bool result_returned =
        RunFunction<WebrtcLoggingPrivateSetMetaDataFunction>(data);
    return (result_expected == result_returned);
  }

  // This function implicitly expects the function to succeed (test failure
  // initiated otherwise).
  // Returns whether the function that was run returned a value, or avoided
  // returning a value, according to expectation.
  // TODO(crbug.com/829419): Return success/failure of the executed function.
  bool StartRtpDump(bool incoming, bool outgoing) {
    base::ListValue params;
    AppendTabIdAndUrl(&params);
    params.AppendBoolean(incoming);
    params.AppendBoolean(outgoing);
    constexpr bool result_expected = false;
    const bool result_returned =
        RunFunction<WebrtcLoggingPrivateStartRtpDumpFunction>(params);
    return (result_expected == result_returned);
  }

  // This function implicitly expects the function to succeed (test failure
  // initiated otherwise).
  // Returns whether the function that was run returned a value, or avoided
  // returning a value, according to expectation.
  // TODO(crbug.com/829419): Return success/failure of the executed function.
  bool StopRtpDump(bool incoming, bool outgoing) {
    base::ListValue params;
    AppendTabIdAndUrl(&params);
    params.AppendBoolean(incoming);
    params.AppendBoolean(outgoing);
    constexpr bool result_expected = false;
    const bool result_returned =
        RunFunction<WebrtcLoggingPrivateStopRtpDumpFunction>(params);
    return (result_expected == result_returned);
  }

  // This function implicitly expects the function to succeed (test failure
  // initiated otherwise).
  // Returns whether the function that was run returned a value, or avoided
  // returning a value, according to expectation.
  // TODO(crbug.com/829419): Return success/failure of the executed function.
  bool StoreLog(const std::string& log_id) {
    base::ListValue params;
    AppendTabIdAndUrl(&params);
    params.AppendString(log_id);
    constexpr bool result_expected = false;
    const bool result_returned =
        RunFunction<WebrtcLoggingPrivateStoreFunction>(params);
    return (result_expected == result_returned);
  }

  // This function implicitly expects the function to succeed (test failure
  // initiated otherwise).
  // Returns whether the function that was run returned a value, or avoided
  // returning a value, according to expectation.
  // TODO(crbug.com/829419): Return success/failure of the executed function.
  bool UploadStoredLog(const std::string& log_id) {
    base::ListValue params;
    AppendTabIdAndUrl(&params);
    params.AppendString(log_id);
    constexpr bool result_expected = true;
    const bool result_returned =
        RunFunction<WebrtcLoggingPrivateUploadStoredFunction>(params);
    return (result_expected == result_returned);
  }

  // This function implicitly expects the function to succeed (test failure
  // initiated otherwise).
  // Returns whether the function that was run returned a value, or avoided
  // returning a value, according to expectation.
  // TODO(crbug.com/829419): Return success/failure of the executed function.
  bool StartAudioDebugRecordings(int seconds) {
    base::ListValue params;
    AppendTabIdAndUrl(&params);
    params.AppendInteger(seconds);
    constexpr bool result_expected = true;
    const bool result_returned =
        RunFunction<WebrtcLoggingPrivateStartAudioDebugRecordingsFunction>(
            params);
    return (result_expected == result_returned);
  }

  // This function implicitly expects the function to succeed (test failure
  // initiated otherwise).
  // Returns whether the function that was run returned a value, or avoided
  // returning a value, according to expectation.
  // TODO(crbug.com/829419): Return success/failure of the executed function.
  bool StopAudioDebugRecordings() {
    base::ListValue params;
    AppendTabIdAndUrl(&params);
    constexpr bool result_expected = true;
    const bool result_returned =
        RunFunction<WebrtcLoggingPrivateStopAudioDebugRecordingsFunction>(
            params);
    return (result_expected == result_returned);
  }

  // This function expects the function to succeed or fail according to
  // |expect_success| (test failure initiated otherwise). It also implicitly
  // expects that no value would be returned.
  // TODO(crbug.com/829419): Return success/failure of the executed function.
  void StartEventLogging(const std::string& peerConnectionId,
                         int maxLogSizeBytes,
                         const std::string& metadata,
                         bool expect_success,
                         const std::string& expected_error = std::string()) {
    DCHECK_EQ(expect_success, expected_error.empty());

    base::ListValue params;
    AppendTabIdAndUrl(&params);
    params.AppendString(peerConnectionId);
    params.AppendInteger(maxLogSizeBytes);
    params.AppendString(metadata);

    if (expect_success) {
      const bool result_returned =
          RunFunction<WebrtcLoggingPrivateStartEventLoggingFunction>(params);
      EXPECT_FALSE(result_returned);  // Should never return a value.
    } else {
      RunFunctionAndExpectError<WebrtcLoggingPrivateStartEventLoggingFunction>(
          params, expected_error);
    }
  }

  void SetUpPeerConnection(const std::string& peer_connection_id) {
    auto* manager = content::WebRtcEventLogger::Get();
    auto* rph = web_contents()->GetRenderViewHost()->GetProcess();

    const int render_process_id = rph->GetID();
    const int lid = 0;

    manager->PeerConnectionAdded(render_process_id, lid, peer_connection_id);
  }

  base::test::ScopedCommandLine scoped_command_line_;
  scoped_refptr<Extension> extension_;
};

class WebrtcLoggingPrivateApiTestDisabledRemoteLogging
    : public WebrtcLoggingPrivateApiTest {
 protected:
  void SetUp() override {
    auto* cl = scoped_command_line_.GetProcessCommandLine();
    cl->AppendSwitchASCII(::switches::kWebRtcRemoteEventLog, "disabled");
    extensions::ExtensionApiTest::SetUp();
    extension_ = extensions::ExtensionBuilder("Test").Build();
  }
};

// Helper class to temporarily tell the uploader to save the multipart buffer to
// a test string instead of uploading.
class ScopedOverrideUploadBuffer {
 public:
  ScopedOverrideUploadBuffer() {
    g_browser_process->webrtc_log_uploader()->
        OverrideUploadWithBufferForTesting(&multipart_);
  }

  ~ScopedOverrideUploadBuffer() {
    g_browser_process->webrtc_log_uploader()->
        OverrideUploadWithBufferForTesting(nullptr);
  }

  const std::string& multipart() const { return multipart_; }

 private:
  std::string multipart_;
};

}  // namespace

IN_PROC_BROWSER_TEST_F(WebrtcLoggingPrivateApiTest, TestStartStopDiscard) {
  ScopedOverrideUploadBuffer buffer_override;

  EXPECT_TRUE(StartLogging());
  EXPECT_TRUE(StopLogging());
  EXPECT_TRUE(DiscardLog());

  EXPECT_TRUE(buffer_override.multipart().empty());
}

// Tests WebRTC diagnostic logging. Sets up the browser to save the multipart
// contents to a buffer instead of uploading it, then verifies it after a calls.
// Example of multipart contents:
// ------**--yradnuoBgoLtrapitluMklaTelgooG--**----
// Content-Disposition: form-data; name="prod"
//
// Chrome_Linux
// ------**--yradnuoBgoLtrapitluMklaTelgooG--**----
// Content-Disposition: form-data; name="ver"
//
// 30.0.1554.0
// ------**--yradnuoBgoLtrapitluMklaTelgooG--**----
// Content-Disposition: form-data; name="guid"
//
// 0
// ------**--yradnuoBgoLtrapitluMklaTelgooG--**----
// Content-Disposition: form-data; name="type"
//
// webrtc_log
// ------**--yradnuoBgoLtrapitluMklaTelgooG--**----
// Content-Disposition: form-data; name="app_session_id"
//
// 0123456789abcdef
// ------**--yradnuoBgoLtrapitluMklaTelgooG--**----
// Content-Disposition: form-data; name="url"
//
// http://127.0.0.1:43213/webrtc/webrtc_jsep01_test.html
// ------**--yradnuoBgoLtrapitluMklaTelgooG--**----
// Content-Disposition: form-data; name="webrtc_log"; filename="webrtc_log.gz"
// Content-Type: application/gzip
//
// <compressed data (zip)>
// ------**--yradnuoBgoLtrapitluMklaTelgooG--**------
//
IN_PROC_BROWSER_TEST_F(WebrtcLoggingPrivateApiTest, TestStartStopUpload) {
  ScopedOverrideUploadBuffer buffer_override;

  base::ListValue parameters;
  AppendTabIdAndUrl(&parameters);
  InitializeTestMetaData(&parameters);

  SetMetaData(parameters);
  StartLogging();
  StopLogging();
  UploadLog();

  std::string multipart = buffer_override.multipart();
  ASSERT_FALSE(multipart.empty());

  // Check multipart data.

  const char boundary[] = "------**--yradnuoBgoLtrapitluMklaTelgooG--**----";

  // Move the compressed data to its own string, since it may contain "\r\n" and
  // it makes the tests below easier.
  const char zip_content_type[] = "Content-Type: application/gzip";
  size_t zip_pos = multipart.find(&zip_content_type[0]);
  ASSERT_NE(std::string::npos, zip_pos);
  // Move pos to where the zip begins. - 1 to remove '\0', + 4 for two "\r\n".
  zip_pos += sizeof(zip_content_type) + 3;
  size_t zip_length = multipart.find(boundary, zip_pos);
  ASSERT_NE(std::string::npos, zip_length);
  // Calculate length, adjust for a "\r\n".
  zip_length -= zip_pos + 2;
  ASSERT_GT(zip_length, 0u);
  std::string log_part = multipart.substr(zip_pos, zip_length);
  multipart.erase(zip_pos, zip_length);

  // Uncompress log and verify contents.
  EXPECT_TRUE(GzipUncompress(log_part, &log_part));
  EXPECT_GT(log_part.length(), 0u);
  // Verify that meta data exists.
  EXPECT_NE(std::string::npos, log_part.find(base::StringPrintf("%s: %s",
      kTestLoggingSessionIdKey, kTestLoggingSessionIdValue)));
  // Verify that the basic info generated at logging startup exists.
  EXPECT_NE(std::string::npos, log_part.find("Chrome version:"));
  EXPECT_NE(std::string::npos, log_part.find("Cpu brand:"));

  // Check the multipart contents.
  std::vector<std::string> multipart_lines = base::SplitStringUsingSubstr(
      multipart, "\r\n", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
  ASSERT_EQ(31, static_cast<int>(multipart_lines.size()));

  EXPECT_STREQ(&boundary[0], multipart_lines[0].c_str());
  EXPECT_STREQ("Content-Disposition: form-data; name=\"prod\"",
               multipart_lines[1].c_str());
  EXPECT_TRUE(multipart_lines[2].empty());
  EXPECT_NE(std::string::npos, multipart_lines[3].find("Chrome"));

  EXPECT_STREQ(&boundary[0], multipart_lines[4].c_str());
  EXPECT_STREQ("Content-Disposition: form-data; name=\"ver\"",
               multipart_lines[5].c_str());
  EXPECT_TRUE(multipart_lines[6].empty());
  // Just check that the version contains a dot.
  EXPECT_NE(std::string::npos, multipart_lines[7].find('.'));

  EXPECT_STREQ(&boundary[0], multipart_lines[8].c_str());
  EXPECT_STREQ("Content-Disposition: form-data; name=\"guid\"",
               multipart_lines[9].c_str());
  EXPECT_TRUE(multipart_lines[10].empty());
  EXPECT_STREQ("0", multipart_lines[11].c_str());

  EXPECT_STREQ(&boundary[0], multipart_lines[12].c_str());
  EXPECT_STREQ("Content-Disposition: form-data; name=\"type\"",
               multipart_lines[13].c_str());
  EXPECT_TRUE(multipart_lines[14].empty());
  EXPECT_STREQ("webrtc_log", multipart_lines[15].c_str());

  EXPECT_STREQ(&boundary[0], multipart_lines[16].c_str());
  EXPECT_STREQ("Content-Disposition: form-data; name=\"app_session_id\"",
               multipart_lines[17].c_str());
  EXPECT_TRUE(multipart_lines[18].empty());
  EXPECT_STREQ(kTestLoggingSessionIdValue, multipart_lines[19].c_str());

  EXPECT_STREQ(&boundary[0], multipart_lines[20].c_str());
  EXPECT_STREQ("Content-Disposition: form-data; name=\"url\"",
               multipart_lines[21].c_str());
  EXPECT_TRUE(multipart_lines[22].empty());
  EXPECT_STREQ(kTestLoggingUrl, multipart_lines[23].c_str());

  EXPECT_STREQ(&boundary[0], multipart_lines[24].c_str());
  EXPECT_STREQ("Content-Disposition: form-data; name=\"webrtc_log\";"
               " filename=\"webrtc_log.gz\"",
               multipart_lines[25].c_str());
  EXPECT_STREQ("Content-Type: application/gzip",
               multipart_lines[26].c_str());
  EXPECT_TRUE(multipart_lines[27].empty());
  EXPECT_TRUE(multipart_lines[28].empty());  // The removed zip part.
  std::string final_delimiter = boundary;
  final_delimiter += "--";
  EXPECT_STREQ(final_delimiter.c_str(), multipart_lines[29].c_str());
  EXPECT_TRUE(multipart_lines[30].empty());
}

IN_PROC_BROWSER_TEST_F(WebrtcLoggingPrivateApiTest, TestStartStopRtpDump) {
  // TODO(tommi): As is, these tests are missing verification of the actual
  // RTP dump data.  We should fix that, e.g. use SetDumpWriterForTesting.
  StartRtpDump(true, true);
  StopRtpDump(true, true);
}

// Tests trying to store a log when a log is not being captured.
// We should get a failure callback in this case.
IN_PROC_BROWSER_TEST_F(WebrtcLoggingPrivateApiTest, TestStoreWithoutLog) {
  base::ListValue parameters;
  AppendTabIdAndUrl(&parameters);
  parameters.AppendString("MyLogId");
  scoped_refptr<WebrtcLoggingPrivateStoreFunction> store(
      CreateFunction<WebrtcLoggingPrivateStoreFunction>());
  const std::string error = utils::RunFunctionAndReturnError(
      store.get(), ParamsToString(parameters), browser());
  ASSERT_FALSE(error.empty());
}

IN_PROC_BROWSER_TEST_F(WebrtcLoggingPrivateApiTest, TestStartStopStore) {
  ASSERT_TRUE(StartLogging());
  ASSERT_TRUE(StopLogging());
  EXPECT_TRUE(StoreLog("MyLogID"));
}

IN_PROC_BROWSER_TEST_F(WebrtcLoggingPrivateApiTest,
                       TestStartStopStoreAndUpload) {
  static const char kLogId[] = "TestStartStopStoreAndUpload";
  ASSERT_TRUE(StartLogging());
  ASSERT_TRUE(StopLogging());
  ASSERT_TRUE(StoreLog(kLogId));

  ScopedOverrideUploadBuffer buffer_override;
  EXPECT_TRUE(UploadStoredLog(kLogId));
  EXPECT_NE(std::string::npos,
            buffer_override.multipart().find("filename=\"webrtc_log.gz\""));
}

IN_PROC_BROWSER_TEST_F(WebrtcLoggingPrivateApiTest,
                       TestStartStopStoreAndUploadWithRtp) {
  static const char kLogId[] = "TestStartStopStoreAndUploadWithRtp";
  ASSERT_TRUE(StartLogging());
  ASSERT_TRUE(StartRtpDump(true, true));
  ASSERT_TRUE(StopLogging());
  ASSERT_TRUE(StopRtpDump(true, true));
  ASSERT_TRUE(StoreLog(kLogId));

  ScopedOverrideUploadBuffer buffer_override;
  EXPECT_TRUE(UploadStoredLog(kLogId));
  EXPECT_NE(std::string::npos,
            buffer_override.multipart().find("filename=\"webrtc_log.gz\""));
}

IN_PROC_BROWSER_TEST_F(WebrtcLoggingPrivateApiTest,
                       TestStartStopStoreAndUploadWithMetaData) {
  static const char kLogId[] = "TestStartStopStoreAndUploadWithRtp";
  ASSERT_TRUE(StartLogging());

  base::ListValue parameters;
  AppendTabIdAndUrl(&parameters);
  InitializeTestMetaData(&parameters);
  SetMetaData(parameters);

  ASSERT_TRUE(StopLogging());
  ASSERT_TRUE(StoreLog(kLogId));

  ScopedOverrideUploadBuffer buffer_override;
  EXPECT_TRUE(UploadStoredLog(kLogId));
  EXPECT_NE(std::string::npos,
            buffer_override.multipart().find("filename=\"webrtc_log.gz\""));
  EXPECT_NE(std::string::npos,
            buffer_override.multipart().find(kTestLoggingUrl));
}

IN_PROC_BROWSER_TEST_F(WebrtcLoggingPrivateApiTest,
                       TestStartStopAudioDebugRecordings) {
  // TODO(guidou): These tests are missing verification of the actual AEC dump
  // data. This will be fixed with a separate browser test.
  // See crbug.com/569957.
  base::CommandLine::ForCurrentProcess()->AppendSwitch(
      switches::kEnableAudioDebugRecordingsFromExtension);
  ASSERT_TRUE(StartAudioDebugRecordings(0));
  ASSERT_TRUE(StopAudioDebugRecordings());
}

IN_PROC_BROWSER_TEST_F(WebrtcLoggingPrivateApiTest,
                       TestStartTimedAudioDebugRecordings) {
  base::CommandLine::ForCurrentProcess()->AppendSwitch(
      switches::kEnableAudioDebugRecordingsFromExtension);
  ASSERT_TRUE(StartAudioDebugRecordings(1));
}

IN_PROC_BROWSER_TEST_F(WebrtcLoggingPrivateApiTest,
                       StartEventLoggingForKnownPeerConnectionSucceeds) {
  const std::string peer_connection_id = "id";
  SetUpPeerConnection(peer_connection_id);
  const int max_size_bytes = kMaxRemoteLogFileSizeBytes;
  const std::string metadata = "metadata";
  constexpr bool expect_success = true;
  StartEventLogging(peer_connection_id, max_size_bytes, metadata,
                    expect_success);
}

IN_PROC_BROWSER_TEST_F(WebrtcLoggingPrivateApiTest,
                       StartEventLoggingWithUnlimitedSizeFails) {
  const std::string peer_connection_id = "id";
  SetUpPeerConnection(peer_connection_id);
  const int max_size_bytes = kWebRtcEventLogManagerUnlimitedFileSize;
  const std::string metadata = "metadata";
  constexpr bool expect_success = false;
  const std::string error_message =
      kStartRemoteLoggingFailureUnlimitedSizeDisallowed;
  StartEventLogging(peer_connection_id, max_size_bytes, metadata,
                    expect_success, error_message);
}

IN_PROC_BROWSER_TEST_F(WebrtcLoggingPrivateApiTest,
                       StartEventLoggingWithExcessiveMaxSizeFails) {
  const std::string peer_connection_id = "id";
  SetUpPeerConnection(peer_connection_id);
  const int max_size_bytes = kMaxRemoteLogFileSizeBytes + 1;
  const std::string metadata = "metadata";
  constexpr bool expect_success = false;
  const std::string error_message = kStartRemoteLoggingFailureMaxSizeTooLarge;
  StartEventLogging(peer_connection_id, max_size_bytes, metadata,
                    expect_success, error_message);
}

IN_PROC_BROWSER_TEST_F(WebrtcLoggingPrivateApiTest,
                       StartEventLoggingWithExcessivelyLongMetadataFails) {
  const std::string peer_connection_id = "id";
  SetUpPeerConnection(peer_connection_id);
  const int max_size_bytes = kMaxRemoteLogFileSizeBytes;
  const std::string metadata(kMaxRemoteLogFileMetadataSizeBytes + 1, 'X');
  constexpr bool expect_success = false;
  const std::string error_message = kStartRemoteLoggingFailureMetadaTooLong;
  StartEventLogging(peer_connection_id, max_size_bytes, metadata,
                    expect_success, error_message);
}

IN_PROC_BROWSER_TEST_F(WebrtcLoggingPrivateApiTest,
                       StartEventLoggingWithMaxSizeTooSmallFails) {
  const std::string peer_connection_id = "id";
  SetUpPeerConnection(peer_connection_id);
  const std::string metadata = "metadata";
  const size_t max_size_bytes =
      kRemoteBoundLogFileHeaderSizeBytes + metadata.length();
  constexpr bool expect_success = false;
  const std::string error_message = kStartRemoteLoggingFailureMaxSizeTooSmall;
  StartEventLogging(peer_connection_id, max_size_bytes, metadata,
                    expect_success, error_message);
}

IN_PROC_BROWSER_TEST_F(WebrtcLoggingPrivateApiTest,
                       StartEventLoggingForNeverAddedPeerConnectionFails) {
  // Note that manager->PeerConnectionAdded() is not called.
  const std::string peer_connection_id = "id";
  const int max_size_bytes = kMaxRemoteLogFileSizeBytes;
  const std::string metadata = "metadata";
  constexpr bool expect_success = false;
  const std::string error_message =
      kStartRemoteLoggingFailureUnknownOrInactivePeerConnection;
  StartEventLogging(peer_connection_id, max_size_bytes, metadata,
                    expect_success, error_message);
}

IN_PROC_BROWSER_TEST_F(WebrtcLoggingPrivateApiTest,
                       StartEventLoggingForWrongPeerConnectionIdFails) {
  const std::string peer_connection_id_1 = "id1";
  const std::string peer_connection_id_2 = "id2";

  SetUpPeerConnection(peer_connection_id_1);
  const int max_size_bytes = kMaxRemoteLogFileSizeBytes;
  const std::string metadata = "metadata";
  constexpr bool expect_success = false;
  const std::string error_message =
      kStartRemoteLoggingFailureUnknownOrInactivePeerConnection;
  StartEventLogging(peer_connection_id_2, max_size_bytes, metadata,
                    expect_success, error_message);
}

IN_PROC_BROWSER_TEST_F(WebrtcLoggingPrivateApiTest,
                       StartEventLoggingForAlreadyLoggedPeerConnectionFails) {
  const std::string peer_connection_id = "id";
  SetUpPeerConnection(peer_connection_id);

  const int max_size_bytes = kMaxRemoteLogFileSizeBytes;
  const std::string metadata = "metadata";

  // First call succeeds.
  {
    constexpr bool expect_success = true;
    StartEventLogging(peer_connection_id, max_size_bytes, metadata,
                      expect_success);
  }

  // Second call fails.
  {
    constexpr bool expect_success = false;
    const std::string error_message = kStartRemoteLoggingFailureAlreadyLogging;
    StartEventLogging(peer_connection_id, max_size_bytes, metadata,
                      expect_success, error_message);
  }
}

IN_PROC_BROWSER_TEST_F(WebrtcLoggingPrivateApiTestDisabledRemoteLogging,
                       StartEventLoggingFails) {
  const std::string peer_connection_id = "id";
  SetUpPeerConnection(peer_connection_id);
  const int max_size_bytes = kMaxRemoteLogFileSizeBytes;
  const std::string metadata = "metadata";
  constexpr bool expect_success = false;
  const std::string error_message = kStartRemoteLoggingFailureFeatureDisabled;
  StartEventLogging(peer_connection_id, max_size_bytes, metadata,
                    expect_success, error_message);
}
