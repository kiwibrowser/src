// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_DRIVE_CHROMEOS_DRIVE_TEST_UTIL_H_
#define COMPONENTS_DRIVE_CHROMEOS_DRIVE_TEST_UTIL_H_

#include <stdint.h>

#include <string>

#include "components/drive/chromeos/file_cache.h"
#include "content/public/test/test_utils.h"
#include "google_apis/drive/test_util.h"
#include "net/base/completion_callback.h"
#include "net/base/io_buffer.h"
#include "net/base/network_change_notifier.h"
#include "net/base/test_completion_callback.h"

class PrefRegistrySimple;

namespace net {
class IOBuffer;
}  // namespace net

namespace drive {

namespace test_util {

// Disk space size used by FakeFreeDiskSpaceGetter.
const int64_t kLotsOfSpace = drive::internal::kMinFreeSpaceInBytes * 10;

// Helper to destroy objects which needs Destroy() to be called on destruction.
// Note: When using this helper, you should destruct objects before
// BrowserThread.
struct DestroyHelperForTests {
  template<typename T>
  void operator()(T* object) const {
    if (object) {
      object->Destroy();
      content::RunAllTasksUntilIdle();  // Finish destruction.
    }
  }
};

// Reads all the data from |reader| and copies to |content|. Returns net::Error
// code.
template<typename Reader>
int ReadAllData(Reader* reader, std::string* content) {
  const int kBufferSize = 10;
  scoped_refptr<net::IOBuffer> buffer(new net::IOBuffer(kBufferSize));
  while (true) {
    net::TestCompletionCallback callback;
    int result = reader->Read(buffer.get(), kBufferSize, callback.callback());
    result = callback.GetResult(result);
    if (result <= 0) {
      // Found an error or EOF. Return it. Note: net::OK is 0.
      return result;
    }
    content->append(buffer->data(), result);
  }
}

// Registers Drive related preferences in |pref_registry|. Drive related
// preferences should be registered as TestingPrefServiceSimple will crash if
// unregistered preference is referenced.
void RegisterDrivePrefs(PrefRegistrySimple* pref_registry);

// Fake NetworkChangeNotifier implementation.
class FakeNetworkChangeNotifier : public net::NetworkChangeNotifier {
 public:
  FakeNetworkChangeNotifier();

  void SetConnectionType(ConnectionType type);

  // NetworkChangeNotifier override.
  ConnectionType GetCurrentConnectionType() const override;

 private:
  net::NetworkChangeNotifier::ConnectionType type_;
};

}  // namespace test_util
}  // namespace drive

#endif  // COMPONENTS_DRIVE_CHROMEOS_DRIVE_TEST_UTIL_H_
