// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/browser/notifications/platform_notification_context_impl.h"
#include "content/browser/service_worker/embedded_worker_test_helper.h"
#include "content/browser/service_worker/service_worker_context_wrapper.h"
#include "content/common/service_worker/service_worker_types.h"
#include "content/public/browser/notification_database_data.h"
#include "content/public/common/notification_resources.h"
#include "content/public/test/test_browser_context.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/test/mock_platform_notification_service.h"
#include "content/test/test_content_browser_client.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/mojom/service_worker/service_worker_registration.mojom.h"
#include "url/gurl.h"

namespace content {

// Fake Service Worker registration id to use in tests requiring one.
const int64_t kFakeServiceWorkerRegistrationId = 42;

class NotificationBrowserClient : public TestContentBrowserClient {
 public:
  NotificationBrowserClient()
      : platform_notification_service_(
            std::make_unique<MockPlatformNotificationService>()) {}

  PlatformNotificationService* GetPlatformNotificationService() override {
    return platform_notification_service_.get();
  }

 private:
  std::unique_ptr<PlatformNotificationService> platform_notification_service_;
};

class PlatformNotificationContextTest : public ::testing::Test {
 public:
  PlatformNotificationContextTest()
      : thread_bundle_(TestBrowserThreadBundle::IO_MAINLOOP), success_(false) {}

  // Callback to provide when reading a single notification from the database.
  void DidReadNotificationData(bool success,
                               const NotificationDatabaseData& database_data) {
    success_ = success;
    database_data_ = database_data;
  }

  // Callback to provide when writing a notification to the database.
  void DidWriteNotificationData(bool success,
                                const std::string& notification_id) {
    success_ = success;
    notification_id_ = notification_id;
  }

  // Callback to provide when deleting notification data from the database.
  void DidDeleteNotificationData(bool success) { success_ = success; }

  // Callback to provide when registering a Service Worker with a Service
  // Worker Context. Will write the registration id to |store_registration_id|.
  void DidRegisterServiceWorker(int64_t* store_registration_id,
                                ServiceWorkerStatusCode status,
                                const std::string& status_message,
                                int64_t service_worker_registration_id) {
    DCHECK(store_registration_id);
    EXPECT_EQ(SERVICE_WORKER_OK, status);

    *store_registration_id = service_worker_registration_id;
  }

  // Callback to provide when unregistering a Service Worker. Will write the
  // resulting status code to |store_status|.
  void DidUnregisterServiceWorker(ServiceWorkerStatusCode* store_status,
                                  ServiceWorkerStatusCode status) {
    DCHECK(store_status);
    *store_status = status;
  }

  // Callback to provide when reading multiple notifications from the database.
  // Will store the success value in the class member, and write the read
  // notification datas to |store_notification_datas|.
  void DidReadAllNotificationDatas(
      std::vector<NotificationDatabaseData>* store_notification_datas,
      bool success,
      const std::vector<NotificationDatabaseData>& notification_datas) {
    DCHECK(store_notification_datas);

    success_ = success;
    *store_notification_datas = notification_datas;
  }

 protected:
  // Creates a new PlatformNotificationContextImpl instance. When using this
  // method, the underlying database will always be created in memory.
  PlatformNotificationContextImpl* CreatePlatformNotificationContext() {
    PlatformNotificationContextImpl* context =
        new PlatformNotificationContextImpl(base::FilePath(), &browser_context_,
                                            nullptr);
    context->Initialize();

    OverrideTaskRunnerForTesting(context);
    return context;
  }

  // Overrides the task runner in |context| with the current message loop
  // proxy, to reduce the number of threads involved in the tests.
  void OverrideTaskRunnerForTesting(PlatformNotificationContextImpl* context) {
    context->SetTaskRunnerForTesting(base::ThreadTaskRunnerHandle::Get());
  }

  // Returns the testing browsing context that can be used for this test.
  BrowserContext* browser_context() { return &browser_context_; }

  // Returns whether the last invoked callback finished successfully.
  bool success() const { return success_; }

  // Returns the NotificationDatabaseData associated with the last invoked
  // ReadNotificationData callback.
  const NotificationDatabaseData& database_data() const {
    return database_data_;
  }

  // Returns the notification id of the notification last written.
  const std::string& notification_id() const { return notification_id_; }

 private:
  TestBrowserThreadBundle thread_bundle_;
  TestBrowserContext browser_context_;

  bool success_;
  NotificationDatabaseData database_data_;
  std::string notification_id_;
};

TEST_F(PlatformNotificationContextTest, ReadNonExistentNotification) {
  scoped_refptr<PlatformNotificationContextImpl> context =
      CreatePlatformNotificationContext();

  context->ReadNotificationData(
      "invalid-notification-id", GURL("https://example.com"),
      base::Bind(&PlatformNotificationContextTest::DidReadNotificationData,
                 base::Unretained(this)));

  base::RunLoop().RunUntilIdle();

  // The read operation should have failed, as it does not exist.
  ASSERT_FALSE(success());
}

TEST_F(PlatformNotificationContextTest, WriteReadNotification) {
  scoped_refptr<PlatformNotificationContextImpl> context =
      CreatePlatformNotificationContext();

  GURL origin("https://example.com");
  NotificationDatabaseData notification_database_data;
  notification_database_data.origin = origin;

  context->WriteNotificationData(
      origin, notification_database_data,
      base::Bind(&PlatformNotificationContextTest::DidWriteNotificationData,
                 base::Unretained(this)));

  base::RunLoop().RunUntilIdle();

  // The write operation should have succeeded with a notification id.
  ASSERT_TRUE(success());
  EXPECT_FALSE(notification_id().empty());

  context->ReadNotificationData(
      notification_id(), origin,
      base::Bind(&PlatformNotificationContextTest::DidReadNotificationData,
                 base::Unretained(this)));

  base::RunLoop().RunUntilIdle();

  // The read operation should have succeeded, with the right notification.
  ASSERT_TRUE(success());

  const NotificationDatabaseData& read_database_data = database_data();
  EXPECT_EQ(notification_database_data.origin, read_database_data.origin);
}

TEST_F(PlatformNotificationContextTest, WriteReadReplacedNotification) {
  scoped_refptr<PlatformNotificationContextImpl> context =
      CreatePlatformNotificationContext();

  const GURL origin("https://example.com");
  const std::string tag = "foo";

  NotificationDatabaseData notification_database_data;
  notification_database_data.service_worker_registration_id =
      kFakeServiceWorkerRegistrationId;
  notification_database_data.origin = origin;
  notification_database_data.notification_data.title =
      base::ASCIIToUTF16("First");
  notification_database_data.notification_data.tag = tag;

  // Write the first notification with the given |tag|.
  context->WriteNotificationData(
      origin, notification_database_data,
      base::Bind(&PlatformNotificationContextTest::DidWriteNotificationData,
                 base::Unretained(this)));

  base::RunLoop().RunUntilIdle();

  std::string read_notification_id = notification_id();

  // The write operation should have succeeded with a notification id.
  ASSERT_TRUE(success());
  EXPECT_FALSE(read_notification_id.empty());

  notification_database_data.notification_data.title =
      base::ASCIIToUTF16("Second");

  // Write the second notification with the given |tag|.
  context->WriteNotificationData(
      origin, notification_database_data,
      base::Bind(&PlatformNotificationContextTest::DidWriteNotificationData,
                 base::Unretained(this)));

  base::RunLoop().RunUntilIdle();

  ASSERT_TRUE(success());
  ASSERT_FALSE(notification_id().empty());
  ASSERT_EQ(notification_id(), read_notification_id);

  // Reading the notifications should only yield the second, replaced one.
  std::vector<NotificationDatabaseData> notification_database_datas;
  context->ReadAllNotificationDataForServiceWorkerRegistration(
      origin, kFakeServiceWorkerRegistrationId,
      base::Bind(&PlatformNotificationContextTest::DidReadAllNotificationDatas,
                 base::Unretained(this), &notification_database_datas));

  base::RunLoop().RunUntilIdle();

  // The read operation should have succeeded, with the right notification.
  ASSERT_TRUE(success());

  ASSERT_EQ(1u, notification_database_datas.size());

  EXPECT_EQ(tag, notification_database_datas[0].notification_data.tag);
  EXPECT_EQ(base::ASCIIToUTF16("Second"),
            notification_database_datas[0].notification_data.title);
}

TEST_F(PlatformNotificationContextTest, DeleteInvalidNotification) {
  scoped_refptr<PlatformNotificationContextImpl> context =
      CreatePlatformNotificationContext();

  context->DeleteNotificationData(
      "invalid-notification-id", GURL("https://example.com"),
      base::Bind(&PlatformNotificationContextTest::DidDeleteNotificationData,
                 base::Unretained(this)));

  base::RunLoop().RunUntilIdle();

  // The notification may not have existed, but since the goal of deleting data
  // is to make sure that it's gone, the goal has been satisfied. As such,
  // deleting a non-existent notification is considered to be a success.
  EXPECT_TRUE(success());
}

TEST_F(PlatformNotificationContextTest, DeleteNotification) {
  scoped_refptr<PlatformNotificationContextImpl> context =
      CreatePlatformNotificationContext();

  GURL origin("https://example.com");
  NotificationDatabaseData notification_database_data;

  context->WriteNotificationData(
      origin, notification_database_data,
      base::Bind(&PlatformNotificationContextTest::DidWriteNotificationData,
                 base::Unretained(this)));

  base::RunLoop().RunUntilIdle();

  // The write operation should have succeeded with a notification id.
  ASSERT_TRUE(success());
  EXPECT_FALSE(notification_id().empty());

  context->DeleteNotificationData(
      notification_id(), origin,
      base::Bind(&PlatformNotificationContextTest::DidDeleteNotificationData,
                 base::Unretained(this)));

  base::RunLoop().RunUntilIdle();

  // The notification existed, so it should have been removed successfully.
  ASSERT_TRUE(success());

  context->ReadNotificationData(
      notification_id(), origin,
      base::Bind(&PlatformNotificationContextTest::DidReadNotificationData,
                 base::Unretained(this)));

  base::RunLoop().RunUntilIdle();

  // The notification was removed, so we shouldn't be able to read it from
  // the database anymore.
  EXPECT_FALSE(success());
}

TEST_F(PlatformNotificationContextTest, ServiceWorkerUnregistered) {
  std::unique_ptr<EmbeddedWorkerTestHelper> embedded_worker_test_helper(
      new EmbeddedWorkerTestHelper(base::FilePath()));

  // Manually create the PlatformNotificationContextImpl so that the Service
  // Worker context wrapper can be passed in.
  scoped_refptr<PlatformNotificationContextImpl> notification_context(
      new PlatformNotificationContextImpl(
          base::FilePath(), browser_context(),
          embedded_worker_test_helper->context_wrapper()));
  notification_context->Initialize();

  OverrideTaskRunnerForTesting(notification_context.get());

  GURL origin("https://example.com");
  GURL script_url("https://example.com/worker.js");

  int64_t service_worker_registration_id =
      blink::mojom::kInvalidServiceWorkerRegistrationId;

  // Register a Service Worker to get a valid registration id.
  blink::mojom::ServiceWorkerRegistrationOptions options;
  options.scope = origin;
  embedded_worker_test_helper->context()->RegisterServiceWorker(
      script_url, options,
      base::BindOnce(&PlatformNotificationContextTest::DidRegisterServiceWorker,
                     base::Unretained(this), &service_worker_registration_id));

  base::RunLoop().RunUntilIdle();
  ASSERT_NE(service_worker_registration_id,
            blink::mojom::kInvalidServiceWorkerRegistrationId);

  NotificationDatabaseData notification_database_data;

  // Create a notification for that Service Worker registration.
  notification_context->WriteNotificationData(
      origin, notification_database_data,
      base::Bind(&PlatformNotificationContextTest::DidWriteNotificationData,
                 base::Unretained(this)));

  base::RunLoop().RunUntilIdle();

  ASSERT_TRUE(success());
  EXPECT_FALSE(notification_id().empty());

  ServiceWorkerStatusCode unregister_status;

  // Now drop the Service Worker registration which owns that notification.
  embedded_worker_test_helper->context()->UnregisterServiceWorker(
      origin, base::BindOnce(
                  &PlatformNotificationContextTest::DidUnregisterServiceWorker,
                  base::Unretained(this), &unregister_status));

  base::RunLoop().RunUntilIdle();
  ASSERT_EQ(SERVICE_WORKER_OK, unregister_status);

  // And verify that the associated notification has indeed been dropped.
  notification_context->ReadNotificationData(
      notification_id(), origin,
      base::Bind(&PlatformNotificationContextTest::DidReadNotificationData,
                 base::Unretained(this)));

  base::RunLoop().RunUntilIdle();

  EXPECT_FALSE(success());
}

TEST_F(PlatformNotificationContextTest, DestroyDatabaseOnStorageWiped) {
  scoped_refptr<PlatformNotificationContextImpl> context =
      CreatePlatformNotificationContext();

  GURL origin("https://example.com");
  NotificationDatabaseData notification_database_data;

  context->WriteNotificationData(
      origin, notification_database_data,
      base::Bind(&PlatformNotificationContextTest::DidWriteNotificationData,
                 base::Unretained(this)));

  base::RunLoop().RunUntilIdle();

  // The write operation should have succeeded with a notification id.
  ASSERT_TRUE(success());
  EXPECT_FALSE(notification_id().empty());

  // Call the OnStorageWiped override from the ServiceWorkerContextCoreObserver,
  // which indicates that the database should go away entirely.
  context->OnStorageWiped();

  // Verify that reading notification data fails because the data does not
  // exist anymore. Deliberately omit RunUntilIdle(), since this is unlikely to
  // be the case when OnStorageWiped gets called in production.
  context->ReadNotificationData(
      notification_id(), origin,
      base::Bind(&PlatformNotificationContextTest::DidReadNotificationData,
                 base::Unretained(this)));

  base::RunLoop().RunUntilIdle();

  EXPECT_FALSE(success());
}

TEST_F(PlatformNotificationContextTest, DestroyOnDiskDatabase) {
  base::ScopedTempDir database_dir;
  ASSERT_TRUE(database_dir.CreateUniqueTempDir());

  // Manually construct the PlatformNotificationContextImpl because this test
  // requires the database to be created on the filesystem.
  scoped_refptr<PlatformNotificationContextImpl> context(
      new PlatformNotificationContextImpl(database_dir.GetPath(),
                                          browser_context(), nullptr));

  OverrideTaskRunnerForTesting(context.get());

  // Trigger a read-operation to force creating the database.
  context->ReadNotificationData(
      "invalid-notification-id", GURL("https://example.com"),
      base::Bind(&PlatformNotificationContextTest::DidReadNotificationData,
                 base::Unretained(this)));

  base::RunLoop().RunUntilIdle();

  EXPECT_FALSE(IsDirectoryEmpty(database_dir.GetPath()));
  EXPECT_FALSE(success());

  // Blow away the database by faking a Service Worker Context wipe-out.
  context->OnStorageWiped();

  base::RunLoop().RunUntilIdle();

  // The database's directory should be empty at this point.
  EXPECT_TRUE(IsDirectoryEmpty(database_dir.GetPath()));
}

TEST_F(PlatformNotificationContextTest, ReadAllServiceWorkerDataEmpty) {
  scoped_refptr<PlatformNotificationContextImpl> context =
      CreatePlatformNotificationContext();

  GURL origin("https://example.com");

  std::vector<NotificationDatabaseData> notification_database_datas;
  context->ReadAllNotificationDataForServiceWorkerRegistration(
      origin, kFakeServiceWorkerRegistrationId,
      base::Bind(&PlatformNotificationContextTest::DidReadAllNotificationDatas,
                 base::Unretained(this), &notification_database_datas));

  base::RunLoop().RunUntilIdle();

  EXPECT_TRUE(success());
  EXPECT_EQ(0u, notification_database_datas.size());
}

TEST_F(PlatformNotificationContextTest, ReadAllServiceWorkerDataFilled) {
  scoped_refptr<PlatformNotificationContextImpl> context =
      CreatePlatformNotificationContext();

  GURL origin("https://example.com");

  NotificationDatabaseData notification_database_data;
  notification_database_data.origin = origin;
  notification_database_data.service_worker_registration_id =
      kFakeServiceWorkerRegistrationId;

  // Insert ten notifications into the database belonging to origin and the
  // test Service Worker Registration id.
  for (int i = 0; i < 10; ++i) {
    context->WriteNotificationData(
        origin, notification_database_data,
        base::Bind(&PlatformNotificationContextTest::DidWriteNotificationData,
                   base::Unretained(this)));

    base::RunLoop().RunUntilIdle();

    ASSERT_TRUE(success());
  }

  // Now read the notifications from the database again. There should be ten,
  // all set with the correct origin and Service Worker Registration id.
  std::vector<NotificationDatabaseData> notification_database_datas;
  context->ReadAllNotificationDataForServiceWorkerRegistration(
      origin, kFakeServiceWorkerRegistrationId,
      base::Bind(&PlatformNotificationContextTest::DidReadAllNotificationDatas,
                 base::Unretained(this), &notification_database_datas));

  base::RunLoop().RunUntilIdle();

  ASSERT_TRUE(success());
  ASSERT_EQ(10u, notification_database_datas.size());

  for (int i = 0; i < 10; ++i) {
    EXPECT_EQ(origin, notification_database_datas[i].origin);
    EXPECT_EQ(kFakeServiceWorkerRegistrationId,
              notification_database_datas[i].service_worker_registration_id);
  }
}

TEST_F(PlatformNotificationContextTest, SynchronizeNotifications) {
  NotificationBrowserClient notification_browser_client;
  SetBrowserClientForTesting(&notification_browser_client);

  scoped_refptr<PlatformNotificationContextImpl> context =
      CreatePlatformNotificationContext();

  GURL origin("https://example.com");
  NotificationDatabaseData notification_database_data;
  notification_database_data.service_worker_registration_id =
      kFakeServiceWorkerRegistrationId;
  PlatformNotificationData notification_data;
  content::NotificationResources notification_resources;

  context->WriteNotificationData(
      origin, notification_database_data,
      base::Bind(&PlatformNotificationContextTest::DidWriteNotificationData,
                 base::Unretained(this)));

  base::RunLoop().RunUntilIdle();
  ASSERT_TRUE(success());
  EXPECT_FALSE(notification_id().empty());

  PlatformNotificationService* service =
      notification_browser_client.GetPlatformNotificationService();

  service->DisplayPersistentNotification(browser_context(), notification_id(),
                                         origin, origin, notification_data,
                                         notification_resources);

  std::vector<NotificationDatabaseData> notification_database_datas;
  context->ReadAllNotificationDataForServiceWorkerRegistration(
      origin, kFakeServiceWorkerRegistrationId,
      base::Bind(&PlatformNotificationContextTest::DidReadAllNotificationDatas,
                 base::Unretained(this), &notification_database_datas));

  base::RunLoop().RunUntilIdle();

  ASSERT_TRUE(success());
  ASSERT_EQ(1u, notification_database_datas.size());

  // Delete the notification from the display service without removing it from
  // the database. It should automatically synchronize on the next read.
  service->ClosePersistentNotification(browser_context(), notification_id());
  context->ReadAllNotificationDataForServiceWorkerRegistration(
      origin, kFakeServiceWorkerRegistrationId,
      base::Bind(&PlatformNotificationContextTest::DidReadAllNotificationDatas,
                 base::Unretained(this), &notification_database_datas));
  base::RunLoop().RunUntilIdle();

  ASSERT_TRUE(success());
  ASSERT_EQ(0u, notification_database_datas.size());

  context->ReadNotificationData(
      notification_id(), origin,
      base::Bind(&PlatformNotificationContextTest::DidReadNotificationData,
                 base::Unretained(this)));

  base::RunLoop().RunUntilIdle();

  // The notification was removed, so we shouldn't be able to read it from
  // the database anymore.
  EXPECT_FALSE(success());
}

}  // namespace content
