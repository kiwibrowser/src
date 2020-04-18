// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/extensions/api/messaging/native_message_process_host.h"

#include <stddef.h>
#include <stdint.h>

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_file.h"
#include "base/files/scoped_temp_dir.h"
#include "base/json/json_reader.h"
#include "base/process/process_metrics.h"
#include "base/rand_util.h"
#include "base/run_loop.h"
#include "base/strings/stringprintf.h"
#include "base/test/test_timeouts.h"
#include "base/threading/platform_thread.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "chrome/browser/extensions/api/messaging/native_messaging_test_util.h"
#include "chrome/browser/extensions/api/messaging/native_process_launcher.h"
#include "components/version_info/version_info.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "extensions/common/extension.h"
#include "extensions/common/features/feature_channel.h"
#include "testing/gtest/include/gtest/gtest.h"

#if defined(OS_POSIX)
#include "base/files/file_descriptor_watcher_posix.h"
#endif

#if defined(OS_WIN)
#include <windows.h>
#include "base/win/scoped_handle.h"
#else
#include <unistd.h>
#endif

using content::BrowserThread;

namespace {

const char kTestMessage[] = "{\"text\": \"Hello.\"}";

}  // namespace

namespace extensions {

class FakeLauncher : public NativeProcessLauncher {
 public:
  FakeLauncher(base::File read_file, base::File write_file)
      : read_file_(std::move(read_file)), write_file_(std::move(write_file)) {}

  static std::unique_ptr<NativeProcessLauncher> Create(
      base::FilePath read_file,
      base::FilePath write_file) {
    int read_flags = base::File::FLAG_OPEN | base::File::FLAG_READ;
    int write_flags = base::File::FLAG_CREATE | base::File::FLAG_WRITE;
#if !defined(OS_POSIX)
    read_flags |= base::File::FLAG_ASYNC;
    write_flags |= base::File::FLAG_ASYNC;
#endif
    return std::unique_ptr<NativeProcessLauncher>(
        new FakeLauncher(base::File(read_file, read_flags),
                         base::File(write_file, write_flags)));
  }

  static std::unique_ptr<NativeProcessLauncher> CreateWithPipeInput(
      base::File read_pipe,
      base::FilePath write_file) {
    int write_flags = base::File::FLAG_CREATE | base::File::FLAG_WRITE;
#if !defined(OS_POSIX)
    write_flags |= base::File::FLAG_ASYNC;
#endif

    return std::unique_ptr<NativeProcessLauncher>(new FakeLauncher(
        std::move(read_pipe), base::File(write_file, write_flags)));
  }

  void Launch(const GURL& origin,
              const std::string& native_host_name,
              const LaunchedCallback& callback) const override {
    callback.Run(NativeProcessLauncher::RESULT_SUCCESS, base::Process(),
                 std::move(read_file_), std::move(write_file_));
  }

 private:
  mutable base::File read_file_;
  mutable base::File write_file_;
};

class NativeMessagingTest : public ::testing::Test,
                            public NativeMessageHost::Client,
                            public base::SupportsWeakPtr<NativeMessagingTest> {
 protected:
  NativeMessagingTest()
      : current_channel_(version_info::Channel::DEV),
        thread_bundle_(content::TestBrowserThreadBundle::IO_MAINLOOP),
        channel_closed_(false) {}

  void SetUp() override { ASSERT_TRUE(temp_dir_.CreateUniqueTempDir()); }

  void TearDown() override {
    if (native_message_host_) {
      BrowserThread::DeleteSoon(
          BrowserThread::IO, FROM_HERE, native_message_host_.release());
    }
    base::RunLoop().RunUntilIdle();
  }

  void PostMessageFromNativeHost(const std::string& message) override {
    last_message_ = message;

    // Parse the message.
    std::unique_ptr<base::DictionaryValue> dict_value =
        base::DictionaryValue::From(base::JSONReader::Read(message));
    if (dict_value) {
      last_message_parsed_ = std::move(dict_value);
    } else {
      LOG(ERROR) << "Failed to parse " << message;
      last_message_parsed_.reset();
    }

    if (run_loop_)
      run_loop_->Quit();
  }

  void CloseChannel(const std::string& error_message) override {
    channel_closed_ = true;
    if (run_loop_)
      run_loop_->Quit();
  }

 protected:
  std::string FormatMessage(const std::string& message) {
    uint32_t length = message.length();
    return std::string(reinterpret_cast<char*>(&length), 4).append(message);
  }

  base::FilePath CreateTempFileWithMessage(const std::string& message) {
    base::FilePath filename;
    if (!base::CreateTemporaryFileInDir(temp_dir_.GetPath(), &filename))
      return base::FilePath();

    std::string message_with_header = FormatMessage(message);
    int bytes_written = base::WriteFile(
        filename, message_with_header.data(), message_with_header.size());
    if (bytes_written < 0 ||
        (message_with_header.size() != static_cast<size_t>(bytes_written))) {
      return base::FilePath();
    }
    return filename;
  }

  base::ScopedTempDir temp_dir_;
  // Force the channel to be dev.
  ScopedCurrentChannel current_channel_;
  std::unique_ptr<NativeMessageHost> native_message_host_;
  std::unique_ptr<base::RunLoop> run_loop_;
  content::TestBrowserThreadBundle thread_bundle_;

  std::string last_message_;
  std::unique_ptr<base::DictionaryValue> last_message_parsed_;
  bool channel_closed_;
};

// Read a single message from a local file.
TEST_F(NativeMessagingTest, SingleSendMessageRead) {
  base::FilePath temp_output_file = temp_dir_.GetPath().AppendASCII("output");
#if defined(OS_WIN)
  base::FilePath temp_input_file = CreateTempFileWithMessage(kTestMessage);
  ASSERT_FALSE(temp_input_file.empty());
  std::unique_ptr<NativeProcessLauncher> launcher =
      FakeLauncher::Create(temp_input_file, temp_output_file);
#else   // defined(OS_WIN)
  base::PlatformFile pipe_handles[2];
  ASSERT_EQ(0, pipe(pipe_handles));
  base::File read_file(pipe_handles[0]);
  std::string formatted_message = FormatMessage(kTestMessage);
  ASSERT_GT(base::GetPageSize(), formatted_message.size());
  ASSERT_TRUE(base::WriteFileDescriptor(
      pipe_handles[1], formatted_message.data(), formatted_message.size()));
  base::File write_file(pipe_handles[1]);
  std::unique_ptr<NativeProcessLauncher> launcher =
      FakeLauncher::CreateWithPipeInput(std::move(read_file), temp_output_file);
#endif  // defined(OS_WIN)
  native_message_host_ = NativeMessageProcessHost::CreateWithLauncher(
      ScopedTestNativeMessagingHost::kExtensionId, "empty_app.py",
      std::move(launcher));
  ASSERT_TRUE(last_message_.empty());
  native_message_host_->Start(this);

  ASSERT_TRUE(native_message_host_);
  run_loop_.reset(new base::RunLoop());
  run_loop_->Run();

  EXPECT_EQ(kTestMessage, last_message_);
}

// Tests sending a single message. The message should get written to
// |temp_file| and should match the contents of single_message_request.msg.
TEST_F(NativeMessagingTest, SingleSendMessageWrite) {
  base::FilePath temp_output_file = temp_dir_.GetPath().AppendASCII("output");

  base::File read_file;
#if defined(OS_WIN)
  base::string16 pipe_name = base::StringPrintf(
      L"\\\\.\\pipe\\chrome.nativeMessaging.out.%llx", base::RandUint64());
  base::File write_handle = base::File::CreateForAsyncHandle(
      CreateNamedPipeW(pipe_name.c_str(),
                       PIPE_ACCESS_OUTBOUND | FILE_FLAG_OVERLAPPED |
                           FILE_FLAG_FIRST_PIPE_INSTANCE,
                       PIPE_TYPE_BYTE, 1, 0, 0, 5000, NULL));
  ASSERT_TRUE(write_handle.IsValid());
  base::File read_handle = base::File::CreateForAsyncHandle(
      CreateFileW(pipe_name.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING,
                  FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, NULL));
  ASSERT_TRUE(read_handle.IsValid());

  read_file = std::move(read_handle);
#else  // defined(OS_WIN)
  base::PlatformFile pipe_handles[2];
  ASSERT_EQ(0, pipe(pipe_handles));
  read_file = base::File(pipe_handles[0]);
  base::File write_file(pipe_handles[1]);
#endif  // !defined(OS_WIN)

  std::unique_ptr<NativeProcessLauncher> launcher =
      FakeLauncher::CreateWithPipeInput(std::move(read_file), temp_output_file);
  native_message_host_ = NativeMessageProcessHost::CreateWithLauncher(
      ScopedTestNativeMessagingHost::kExtensionId, "empty_app.py",
      std::move(launcher));
  native_message_host_->Start(this);
  ASSERT_TRUE(native_message_host_);
  base::RunLoop().RunUntilIdle();

  native_message_host_->OnMessage(kTestMessage);
  base::RunLoop().RunUntilIdle();

  std::string output;
  base::TimeTicks start_time = base::TimeTicks::Now();
  while (base::TimeTicks::Now() - start_time < TestTimeouts::action_timeout()) {
    ASSERT_TRUE(base::ReadFileToString(temp_output_file, &output));
    if (!output.empty())
      break;
    base::PlatformThread::YieldCurrentThread();
  }

  EXPECT_EQ(FormatMessage(kTestMessage), output);
}

// Test send message with a real client. The client just echo's back the text
// it received.
TEST_F(NativeMessagingTest, EchoConnect) {
  ScopedTestNativeMessagingHost test_host;
  ASSERT_NO_FATAL_FAILURE(test_host.RegisterTestHost(false));
  std::string error_message;
  native_message_host_ = NativeMessageProcessHost::Create(
      NULL,
      ScopedTestNativeMessagingHost::kExtensionId,
      ScopedTestNativeMessagingHost::kHostName,
      false,
      &error_message);
  native_message_host_->Start(this);
  ASSERT_TRUE(native_message_host_);

  native_message_host_->OnMessage("{\"text\": \"Hello.\"}");
  run_loop_.reset(new base::RunLoop());
  run_loop_->Run();
  ASSERT_FALSE(last_message_.empty());
  ASSERT_TRUE(last_message_parsed_);

  std::string expected_url = std::string("chrome-extension://") +
      ScopedTestNativeMessagingHost::kExtensionId + "/";
  int id;
  EXPECT_TRUE(last_message_parsed_->GetInteger("id", &id));
  EXPECT_EQ(1, id);
  std::string text;
  EXPECT_TRUE(last_message_parsed_->GetString("echo.text", &text));
  EXPECT_EQ("Hello.", text);
  std::string url;
  EXPECT_TRUE(last_message_parsed_->GetString("caller_url", &url));
  EXPECT_EQ(expected_url, url);

  native_message_host_->OnMessage("{\"foo\": \"bar\"}");
  run_loop_.reset(new base::RunLoop());
  run_loop_->Run();
  EXPECT_TRUE(last_message_parsed_->GetInteger("id", &id));
  EXPECT_EQ(2, id);
  EXPECT_TRUE(last_message_parsed_->GetString("echo.foo", &text));
  EXPECT_EQ("bar", text);
  EXPECT_TRUE(last_message_parsed_->GetString("caller_url", &url));
  EXPECT_EQ(expected_url, url);
}

TEST_F(NativeMessagingTest, UserLevel) {
  ScopedTestNativeMessagingHost test_host;
  ASSERT_NO_FATAL_FAILURE(test_host.RegisterTestHost(true));

  std::string error_message;
  native_message_host_ = NativeMessageProcessHost::Create(
      NULL,
      ScopedTestNativeMessagingHost::kExtensionId,
      ScopedTestNativeMessagingHost::kHostName,
      true,
      &error_message);
  native_message_host_->Start(this);
  ASSERT_TRUE(native_message_host_);

  native_message_host_->OnMessage("{\"text\": \"Hello.\"}");
  run_loop_.reset(new base::RunLoop());
  run_loop_->Run();
  ASSERT_FALSE(last_message_.empty());
  ASSERT_TRUE(last_message_parsed_);
}

TEST_F(NativeMessagingTest, DisallowUserLevel) {
  ScopedTestNativeMessagingHost test_host;
  ASSERT_NO_FATAL_FAILURE(test_host.RegisterTestHost(true));

  std::string error_message;
  native_message_host_ = NativeMessageProcessHost::Create(
      NULL,
      ScopedTestNativeMessagingHost::kExtensionId,
      ScopedTestNativeMessagingHost::kHostName,
      false,
      &error_message);
  native_message_host_->Start(this);
  ASSERT_TRUE(native_message_host_);
  run_loop_.reset(new base::RunLoop());
  run_loop_->Run();

  // The host should fail to start.
  ASSERT_TRUE(channel_closed_);
}

}  // namespace extensions
