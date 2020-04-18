// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ppapi/tests/test_broker.h"

#if defined(_MSC_VER)
#define OS_WIN 1
#include <windows.h>
#else
#define OS_POSIX 1
#include <errno.h>
#include <unistd.h>
#endif

#include <stddef.h>
#include <stdint.h>

#include <cstdio>
#include <cstring>
#include <fstream>
#include <limits>

#include "ppapi/c/pp_errors.h"
#include "ppapi/c/trusted/ppb_broker_trusted.h"
#include "ppapi/c/trusted/ppp_broker.h"
#include "ppapi/tests/test_utils.h"
#include "ppapi/tests/testing_instance.h"

REGISTER_TEST_CASE(Broker);

namespace {

const char kHelloMessage[] = "Hello Plugin! This is Broker!";
// Message sent from broker to plugin if the broker is unsandboxed.
const char kBrokerUnsandboxed[] = "Broker is Unsandboxed!";
// Message sent from broker to plugin if the broker is sandboxed. This message
// needs to be longer than |kBrokerUnsandboxed| because the plugin is expecting
// |kBrokerUnsandboxed|. If it's shorter and the broker doesn't close its handle
// properly the plugin will hang waiting for all data of |kBrokerUnsandboxed| to
// be read.
const char kBrokerSandboxed[] = "Broker is Sandboxed! Verification failed!";

#if defined(OS_WIN)
typedef HANDLE PlatformFile;
const PlatformFile kInvalidPlatformFileValue = INVALID_HANDLE_VALUE;
const int32_t kInvalidHandle = static_cast<int32_t>(
    reinterpret_cast<intptr_t>(INVALID_HANDLE_VALUE));
#elif defined(OS_POSIX)
typedef int PlatformFile;
const PlatformFile kInvalidPlatformFileValue = -1;
const int32_t kInvalidHandle = -1;
#endif

PlatformFile IntToPlatformFile(int32_t handle) {
#if defined(OS_WIN)
  return reinterpret_cast<HANDLE>(static_cast<intptr_t>(handle));
#elif defined(OS_POSIX)
  return handle;
#endif
}

#if defined(OS_POSIX)

#define HANDLE_EINTR(x) ({ \
  decltype(x) eintr_wrapper_result; \
  do { \
    eintr_wrapper_result = (x); \
  } while (eintr_wrapper_result == -1 && errno == EINTR); \
  eintr_wrapper_result; \
})

#define IGNORE_EINTR(x) ({ \
  decltype(x) eintr_wrapper_result; \
  do { \
    eintr_wrapper_result = (x); \
    if (eintr_wrapper_result == -1 && errno == EINTR) { \
      eintr_wrapper_result = 0; \
    } \
  } while (0); \
  eintr_wrapper_result; \
})

#endif

bool ReadMessage(PlatformFile file, size_t message_len, char* message) {
#if defined(OS_WIN)
  assert(message_len < std::numeric_limits<DWORD>::max());
  DWORD read = 0;
  const DWORD size = static_cast<DWORD>(message_len);
  return ::ReadFile(file, message, size, &read, NULL) && read == size;
#elif defined(OS_POSIX)
  assert(message_len <
         static_cast<size_t>(std::numeric_limits<ssize_t>::max()));
  size_t total_read = 0;
  while (total_read < message_len) {
    ssize_t read = HANDLE_EINTR(::read(file, message + total_read,
                                       message_len - total_read));
    if (read <= 0)
      break;
    total_read += read;
  }
  return total_read == message_len;
#endif
}

bool WriteMessage(PlatformFile file, size_t message_len, const char* message) {
#if defined(OS_WIN)
  assert(message_len < std::numeric_limits<DWORD>::max());
  DWORD written = 0;
  const DWORD size = static_cast<DWORD>(message_len);
  return ::WriteFile(file, message, size, &written, NULL) && written == size;
#elif defined(OS_POSIX)
  assert(message_len <
         static_cast<size_t>(std::numeric_limits<ssize_t>::max()));
  size_t total_written = 0;
  while (total_written < message_len) {
    ssize_t written = HANDLE_EINTR(::write(file, message + total_written,
                                           message_len - total_written));
    if (written <= 0)
      break;
    total_written += written;
  }
  return total_written == message_len;
#endif
}

bool VerifyMessage(PlatformFile file, size_t message_len, const char* message) {
  char* message_received = new char[message_len];
  bool success = ReadMessage(file, message_len, message_received) &&
                 !::strcmp(message_received, message);
  delete [] message_received;
  return success;
}

bool ClosePlatformFile(PlatformFile file) {
#if defined(OS_WIN)
  return !!::CloseHandle(file);
#elif defined(OS_POSIX)
  return !IGNORE_EINTR(::close(file));
#endif
}

bool VerifyIsUnsandboxed() {
#if defined(OS_WIN)
  FILE* file = NULL;
  wchar_t temp_path[MAX_PATH] = {'\0'};
  wchar_t file_name[MAX_PATH] = {'\0'};
  if (!::GetTempPath(MAX_PATH, temp_path) ||
      !::GetTempFileName(temp_path, L"test_pepper_broker", 0, file_name) ||
      ::_wfopen_s(&file, file_name, L"w"))
    return false;

  if (::fclose(file)) {
    ::DeleteFile(file_name);
    return false;
  }

  return !!::DeleteFile(file_name);
#elif defined(OS_POSIX)
  char file_name[] = "/tmp/test_pepper_broker_XXXXXX";
  int fd = ::mkstemp(file_name);
  if (-1 == fd)
    return false;

  if (IGNORE_EINTR(::close(fd))) {
    ::remove(file_name);
    return false;
  }

  return !::remove(file_name);
#endif
}

// Callback in the broker when a new broker connection occurs.
int32_t OnInstanceConnected(PP_Instance instance, int32_t handle) {
  PlatformFile file = IntToPlatformFile(handle);
  if (file == kInvalidPlatformFileValue)
    return PP_ERROR_FAILED;

  // Send hello message.
  if (!WriteMessage(file, sizeof(kHelloMessage), kHelloMessage)) {
    ClosePlatformFile(file);
    return PP_ERROR_FAILED;
  }

  // Verify broker is not sandboxed and send result to plugin over the pipe.
  if (VerifyIsUnsandboxed()) {
    if (!WriteMessage(file, sizeof(kBrokerUnsandboxed), kBrokerUnsandboxed)) {
      ClosePlatformFile(file);
      return PP_ERROR_FAILED;
    }
  } else {
    if (!WriteMessage(file, sizeof(kBrokerSandboxed), kBrokerSandboxed)) {
      ClosePlatformFile(file);
      return PP_ERROR_FAILED;
    }
  }

  if (!ClosePlatformFile(file))
    return PP_ERROR_FAILED;

  return PP_OK;
}

}  // namespace

PP_EXPORT int32_t PPP_InitializeBroker(
    PP_ConnectInstance_Func* connect_instance_func) {
  *connect_instance_func = &OnInstanceConnected;
  return PP_OK;
}

PP_EXPORT void PPP_ShutdownBroker() {}

TestBroker::TestBroker(TestingInstance* instance)
    : TestCase(instance),
      broker_interface_(NULL) {
}

bool TestBroker::Init() {
  broker_interface_ = static_cast<const PPB_BrokerTrusted*>(
      pp::Module::Get()->GetBrowserInterface(PPB_BROKER_TRUSTED_INTERFACE));
  return !!broker_interface_;
}

void TestBroker::RunTests(const std::string& filter) {
  RUN_TEST(Create, filter);
  RUN_TEST(Create, filter);
  RUN_TEST(GetHandleFailure, filter);
  RUN_TEST_FORCEASYNC_AND_NOT(ConnectFailure, filter);
  RUN_TEST_FORCEASYNC_AND_NOT(ConnectAndPipe, filter);

  // The following tests require special setup, so only run them if they're
  // explicitly specified by the filter.
  if (!ShouldRunAllTests(filter)) {
    RUN_TEST(ConnectPermissionDenied, filter);
    RUN_TEST(ConnectPermissionGranted, filter);
    RUN_TEST(IsAllowedPermissionDenied, filter);
    RUN_TEST(IsAllowedPermissionGranted, filter);
  }
}

std::string TestBroker::TestCreate() {
  // Very simplistic test to make sure we can create a broker interface.
  // TODO(raymes): All of the resources created in this file are leaked. Write
  // a C++ wrapper for PPB_Broker_Trusted to avoid these leaks.
  PP_Resource broker = broker_interface_->CreateTrusted(
      instance_->pp_instance());
  ASSERT_TRUE(broker);

  ASSERT_FALSE(broker_interface_->IsBrokerTrusted(0));
  ASSERT_TRUE(broker_interface_->IsBrokerTrusted(broker));

  PASS();
}

// Test connection on invalid resource.
std::string TestBroker::TestConnectFailure() {
  TestCompletionCallback callback(instance_->pp_instance(), callback_type());
  callback.WaitForResult(broker_interface_->Connect(0,
      callback.GetCallback().pp_completion_callback()));
  CHECK_CALLBACK_BEHAVIOR(callback);
  ASSERT_EQ(PP_ERROR_BADRESOURCE, callback.result());

  PASS();
}

std::string TestBroker::TestGetHandleFailure() {
  int32_t handle = kInvalidHandle;

  // Test getting the handle for an invalid resource.
  ASSERT_EQ(PP_ERROR_BADRESOURCE, broker_interface_->GetHandle(0, &handle));

  // Connect hasn't been called so this should fail.
  PP_Resource broker = broker_interface_->CreateTrusted(
      instance_->pp_instance());
  ASSERT_TRUE(broker);
  ASSERT_EQ(PP_ERROR_FAILED, broker_interface_->GetHandle(broker, &handle));

  PASS();
}

std::string TestBroker::TestConnectAndPipe() {
  PP_Resource broker = broker_interface_->CreateTrusted(
      instance_->pp_instance());
  ASSERT_TRUE(broker);

  TestCompletionCallback callback(instance_->pp_instance(), callback_type());
  callback.WaitForResult(broker_interface_->Connect(broker,
      callback.GetCallback().pp_completion_callback()));
  CHECK_CALLBACK_BEHAVIOR(callback);
  ASSERT_EQ(PP_OK, callback.result());

  int32_t handle = kInvalidHandle;
  ASSERT_EQ(PP_OK, broker_interface_->GetHandle(broker, &handle));
  ASSERT_NE(kInvalidHandle, handle);

  PlatformFile file = IntToPlatformFile(handle);
  ASSERT_TRUE(VerifyMessage(file, sizeof(kHelloMessage), kHelloMessage));
  ASSERT_TRUE(VerifyMessage(file, sizeof(kBrokerUnsandboxed),
                            kBrokerUnsandboxed));

  ASSERT_TRUE(ClosePlatformFile(file));

  PASS();
}

std::string TestBroker::TestConnectPermissionDenied() {
  // This assumes that the browser side is set up to deny access to the broker.
  PP_Resource broker = broker_interface_->CreateTrusted(
      instance_->pp_instance());
  ASSERT_TRUE(broker);

  TestCompletionCallback callback(instance_->pp_instance(), callback_type());
  callback.WaitForResult(broker_interface_->Connect(broker,
      callback.GetCallback().pp_completion_callback()));
  CHECK_CALLBACK_BEHAVIOR(callback);
  ASSERT_EQ(PP_ERROR_NOACCESS, callback.result());

  PASS();
}

std::string TestBroker::TestConnectPermissionGranted() {
  // This assumes that the browser side is set up to allow access to the broker.
  PP_Resource broker = broker_interface_->CreateTrusted(
      instance_->pp_instance());
  ASSERT_TRUE(broker);

  TestCompletionCallback callback(instance_->pp_instance(), callback_type());
  callback.WaitForResult(broker_interface_->Connect(broker,
      callback.GetCallback().pp_completion_callback()));
  CHECK_CALLBACK_BEHAVIOR(callback);
  ASSERT_EQ(PP_OK, callback.result());

  PASS();
}

std::string TestBroker::TestIsAllowedPermissionDenied() {
  PP_Resource broker = broker_interface_->CreateTrusted(
      instance_->pp_instance());
  ASSERT_TRUE(broker);
  ASSERT_EQ(PP_FALSE, broker_interface_->IsAllowed(broker));

  PASS();
}

std::string TestBroker::TestIsAllowedPermissionGranted() {
  PP_Resource broker = broker_interface_->CreateTrusted(
      instance_->pp_instance());
  ASSERT_TRUE(broker);
  ASSERT_EQ(PP_TRUE, broker_interface_->IsAllowed(broker));

  PASS();
}
