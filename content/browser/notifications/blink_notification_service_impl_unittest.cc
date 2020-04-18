// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdint.h>
#include <memory>
#include <vector>

#include "base/callback_forward.h"
#include "base/callback_helpers.h"
#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "base/run_loop.h"
#include "base/test/test_simple_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/browser/notifications/blink_notification_service_impl.h"
#include "content/browser/notifications/platform_notification_context_impl.h"
#include "content/browser/service_worker/embedded_worker_test_helper.h"
#include "content/browser/service_worker/service_worker_context_wrapper.h"
#include "content/common/service_worker/service_worker_types.h"
#include "content/public/test/mock_resource_context.h"
#include "content/public/test/test_browser_context.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/public/test/test_utils.h"
#include "content/test/mock_platform_notification_service.h"
#include "content/test/test_content_browser_client.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/interface_request.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/platform/modules/notifications/notification_service.mojom.h"
#include "third_party/blink/public/platform/modules/permissions/permission_status.mojom.h"

namespace content {

namespace {

const int kFakeRenderProcessId = 1;

const char kTestOrigin[] = "https://example.com";
const char kTestServiceWorkerUrl[] = "https://example.com/sw.js";

class MockNonPersistentNotificationListener
    : public blink::mojom::NonPersistentNotificationListener {
 public:
  MockNonPersistentNotificationListener() : binding_(this) {}
  ~MockNonPersistentNotificationListener() override = default;

  blink::mojom::NonPersistentNotificationListenerPtr GetPtr() {
    blink::mojom::NonPersistentNotificationListenerPtr ptr;
    binding_.Bind(mojo::MakeRequest(&ptr));
    return ptr;
  }

  // NonPersistentNotificationListener interface.
  void OnShow() override {}
  void OnClick() override {}
  void OnClose(OnCloseCallback completed_closure) override {
    std::move(completed_closure).Run();
  }

 private:
  mojo::Binding<blink::mojom::NonPersistentNotificationListener> binding_;
};

}  // anonymous namespace

// This is for overriding the Platform Notification Service with a mock one.
class NotificationBrowserClient : public TestContentBrowserClient {
 public:
  NotificationBrowserClient(
      MockPlatformNotificationService* mock_platform_service)
      : platform_notification_service_(mock_platform_service) {}

  PlatformNotificationService* GetPlatformNotificationService() override {
    return platform_notification_service_;
  }

 private:
  MockPlatformNotificationService* platform_notification_service_;
};

class BlinkNotificationServiceImplTest : public ::testing::Test {
 public:
  // Using REAL_IO_THREAD would give better coverage for thread safety, but
  // at time of writing EmbeddedWorkerTestHelper didn't seem to support that.
  BlinkNotificationServiceImplTest()
      : thread_bundle_(TestBrowserThreadBundle::IO_MAINLOOP),
        embedded_worker_helper_(
            std::make_unique<EmbeddedWorkerTestHelper>(base::FilePath())),
        notification_browser_client_(&mock_platform_service_) {
    SetBrowserClientForTesting(&notification_browser_client_);
  }

  ~BlinkNotificationServiceImplTest() override = default;

  // ::testing::Test overrides.
  void SetUp() override {
    notification_context_ = new PlatformNotificationContextImpl(
        base::FilePath(), &browser_context_,
        embedded_worker_helper_->context_wrapper());
    notification_context_->Initialize();

    // Wait for notification context to be initialized to avoid TSAN detecting
    // a memory race in tests - in production the PlatformNotificationContext
    // will be initialized long before it is read from so this is fine.
    RunAllTasksUntilIdle();

    blink::mojom::NotificationServicePtr notification_service_ptr;
    notification_service_ = std::make_unique<BlinkNotificationServiceImpl>(
        notification_context_.get(), &browser_context_, &resource_context_,
        embedded_worker_helper_->context_wrapper(), kFakeRenderProcessId,
        url::Origin::Create(GURL(kTestOrigin)),
        mojo::MakeRequest(&notification_service_ptr));
  }

  void TearDown() override {
    embedded_worker_helper_.reset();

    // Give pending shutdown operations a chance to finish.
    base::RunLoop().RunUntilIdle();
  }

  void RegisterServiceWorker(
      scoped_refptr<ServiceWorkerRegistration>* service_worker_registration) {
    int64_t service_worker_registration_id =
        blink::mojom::kInvalidServiceWorkerRegistrationId;

    blink::mojom::ServiceWorkerRegistrationOptions options;
    options.scope = GURL(kTestOrigin);

    {
      base::RunLoop run_loop;
      embedded_worker_helper_->context()->RegisterServiceWorker(
          GURL(kTestServiceWorkerUrl), options,
          base::AdaptCallbackForRepeating(base::BindOnce(
              &BlinkNotificationServiceImplTest::DidRegisterServiceWorker,
              base::Unretained(this), &service_worker_registration_id,
              run_loop.QuitClosure())));
      run_loop.Run();
    }

    if (service_worker_registration_id ==
        blink::mojom::kInvalidServiceWorkerRegistrationId) {
      ADD_FAILURE() << "Could not obtain a valid Service Worker registration";
    }

    {
      base::RunLoop run_loop;
      embedded_worker_helper_->context()->storage()->FindRegistrationForId(
          service_worker_registration_id, GURL(kTestOrigin),
          base::BindOnce(&BlinkNotificationServiceImplTest::
                             DidFindServiceWorkerRegistration,
                         base::Unretained(this), service_worker_registration,
                         run_loop.QuitClosure()));

      run_loop.Run();
    }

    // Wait for the worker to be activated.
    base::RunLoop().RunUntilIdle();

    if (!*service_worker_registration) {
      ADD_FAILURE() << "Could not find the new Service Worker registration.";
    }
  }

  void DidRegisterServiceWorker(int64_t* out_service_worker_registration_id,
                                base::OnceClosure quit_closure,
                                ServiceWorkerStatusCode status,
                                const std::string& status_message,
                                int64_t service_worker_registration_id) {
    DCHECK(out_service_worker_registration_id);
    EXPECT_EQ(SERVICE_WORKER_OK, status) << ServiceWorkerStatusToString(status);

    *out_service_worker_registration_id = service_worker_registration_id;

    std::move(quit_closure).Run();
  }

  void DidFindServiceWorkerRegistration(
      scoped_refptr<ServiceWorkerRegistration>* out_service_worker_registration,
      base::OnceClosure quit_closure,
      ServiceWorkerStatusCode status,
      scoped_refptr<ServiceWorkerRegistration> service_worker_registration) {
    DCHECK(out_service_worker_registration);
    EXPECT_EQ(SERVICE_WORKER_OK, status) << ServiceWorkerStatusToString(status);

    *out_service_worker_registration = service_worker_registration;

    std::move(quit_closure).Run();
  }

  void DidGetPermissionStatus(
      blink::mojom::PermissionStatus permission_status) {
    permission_callback_result_ = permission_status;
  }

  blink::mojom::PermissionStatus GetPermissionCallbackResult() {
    return permission_callback_result_;
  }

  void DidDisplayPersistentNotification(
      base::OnceClosure quit_closure,
      blink::mojom::PersistentNotificationError error) {
    display_persistent_callback_result_ = error;
    std::move(quit_closure).Run();
  }

  void DidGetNotifications(
      base::OnceClosure quit_closure,
      const std::vector<std::string>& notification_ids,
      const std::vector<PlatformNotificationData>& notification_datas) {
    get_notifications_callback_result_ = notification_ids;
    std::move(quit_closure).Run();
  }

  void DidGetDisplayedNotifications(
      base::OnceClosure quit_closure,
      std::unique_ptr<std::set<std::string>> notification_ids,
      bool supports_synchronization) {
    get_displayed_callback_result_ = *notification_ids;
    std::move(quit_closure).Run();
  }

  void DidReadNotificationData(base::OnceClosure quit_closure,
                               bool success,
                               const NotificationDatabaseData& data) {
    read_notification_data_callback_result_ = success;
    std::move(quit_closure).Run();
  }

  void DisplayPersistentNotificationSync(
      int64_t service_worker_registration_id,
      const PlatformNotificationData& platform_notification_data,
      const NotificationResources& notification_resources) {
    base::RunLoop run_loop;
    notification_service_->DisplayPersistentNotification(
        service_worker_registration_id, platform_notification_data,
        notification_resources,
        base::BindOnce(
            &BlinkNotificationServiceImplTest::DidDisplayPersistentNotification,
            base::Unretained(this), run_loop.QuitClosure()));
    run_loop.Run();
  }

  std::vector<std::string> GetNotificationsSync(
      int64_t service_worker_registration_id,
      const std::string& filter_tag) {
    base::RunLoop run_loop;
    notification_service_->GetNotifications(
        service_worker_registration_id, filter_tag,
        base::BindOnce(&BlinkNotificationServiceImplTest::DidGetNotifications,
                       base::Unretained(this), run_loop.QuitClosure()));
    run_loop.Run();
    return get_notifications_callback_result_;
  }

  // Synchronous wrapper of
  // PlatformNotificationService::GetDisplayedNotifications
  std::set<std::string> GetDisplayedNotifications() {
    base::RunLoop run_loop;
    mock_platform_service_.GetDisplayedNotifications(
        &browser_context_,
        base::Bind(
            &BlinkNotificationServiceImplTest::DidGetDisplayedNotifications,
            base::Unretained(this), run_loop.QuitClosure()));
    run_loop.Run();
    return get_displayed_callback_result_;
  }

  // Synchronous wrapper of
  // PlatformNotificationContext::ReadNotificationData
  bool ReadNotificationData(const std::string& notification_id) {
    base::RunLoop run_loop;
    notification_context_->ReadNotificationData(
        notification_id, GURL(kTestOrigin),
        base::AdaptCallbackForRepeating(base::BindOnce(
            &BlinkNotificationServiceImplTest::DidReadNotificationData,
            base::Unretained(this), run_loop.QuitClosure())));
    run_loop.Run();
    return read_notification_data_callback_result_;
  }

 protected:
  TestBrowserThreadBundle thread_bundle_;  // Must be first member.

  std::unique_ptr<EmbeddedWorkerTestHelper> embedded_worker_helper_;

  std::unique_ptr<BlinkNotificationServiceImpl> notification_service_;

  TestBrowserContext browser_context_;

  scoped_refptr<PlatformNotificationContextImpl> notification_context_;

  MockPlatformNotificationService mock_platform_service_;

  MockNonPersistentNotificationListener non_persistent_notification_listener_;

  blink::mojom::PersistentNotificationError display_persistent_callback_result_;

 private:
  NotificationBrowserClient notification_browser_client_;

  blink::mojom::PermissionStatus permission_callback_result_ =
      blink::mojom::PermissionStatus::ASK;

  std::set<std::string> get_displayed_callback_result_;

  std::vector<std::string> get_notifications_callback_result_;

  bool read_notification_data_callback_result_ = false;

  MockResourceContext resource_context_;

  DISALLOW_COPY_AND_ASSIGN(BlinkNotificationServiceImplTest);
};

TEST_F(BlinkNotificationServiceImplTest, GetPermissionStatus) {
  mock_platform_service_.SetPermission(blink::mojom::PermissionStatus::GRANTED);

  notification_service_->GetPermissionStatus(
      base::BindOnce(&BlinkNotificationServiceImplTest::DidGetPermissionStatus,
                     base::Unretained(this)));

  EXPECT_EQ(blink::mojom::PermissionStatus::GRANTED,
            GetPermissionCallbackResult());

  mock_platform_service_.SetPermission(blink::mojom::PermissionStatus::DENIED);

  notification_service_->GetPermissionStatus(
      base::BindOnce(&BlinkNotificationServiceImplTest::DidGetPermissionStatus,
                     base::Unretained(this)));

  EXPECT_EQ(blink::mojom::PermissionStatus::DENIED,
            GetPermissionCallbackResult());

  mock_platform_service_.SetPermission(blink::mojom::PermissionStatus::ASK);
  notification_service_->GetPermissionStatus(
      base::BindOnce(&BlinkNotificationServiceImplTest::DidGetPermissionStatus,
                     base::Unretained(this)));

  EXPECT_EQ(blink::mojom::PermissionStatus::ASK, GetPermissionCallbackResult());
}

TEST_F(BlinkNotificationServiceImplTest,
       DisplayNonPersistentNotificationWithPermission) {
  mock_platform_service_.SetPermission(blink::mojom::PermissionStatus::GRANTED);

  notification_service_->DisplayNonPersistentNotification(
      "token", PlatformNotificationData(), NotificationResources(),
      non_persistent_notification_listener_.GetPtr());

  // TODO(https://crbug.com/787459): Pass a callback to
  // DisplayNonPersistentNotification instead of waiting for all tasks to run
  // here; a callback parameter will be needed anyway to enable
  // non-persistent notification event acknowledgements - see bug.
  RunAllTasksUntilIdle();

  EXPECT_EQ(1u, GetDisplayedNotifications().size());
}

TEST_F(BlinkNotificationServiceImplTest,
       DisplayNonPersistentNotificationWithoutPermission) {
  mock_platform_service_.SetPermission(blink::mojom::PermissionStatus::DENIED);

  notification_service_->DisplayNonPersistentNotification(
      "token", PlatformNotificationData(), NotificationResources(),
      non_persistent_notification_listener_.GetPtr());

  // TODO(https://crbug.com/787459): Pass a callback to
  // DisplayNonPersistentNotification instead of waiting for all tasks to run
  // here; a callback parameter will be needed anyway to enable
  // non-persistent notification event acknowledgements - see bug.
  RunAllTasksUntilIdle();

  EXPECT_EQ(0u, GetDisplayedNotifications().size());
}

TEST_F(BlinkNotificationServiceImplTest,
       DisplayPersistentNotificationWithPermission) {
  mock_platform_service_.SetPermission(blink::mojom::PermissionStatus::GRANTED);

  scoped_refptr<ServiceWorkerRegistration> registration;
  RegisterServiceWorker(&registration);

  DisplayPersistentNotificationSync(
      registration->id(), PlatformNotificationData(), NotificationResources());

  EXPECT_EQ(blink::mojom::PersistentNotificationError::NONE,
            display_persistent_callback_result_);

  // Wait for service to receive the Display call.
  RunAllTasksUntilIdle();

  EXPECT_EQ(1u, GetDisplayedNotifications().size());
}

TEST_F(BlinkNotificationServiceImplTest, CloseDisplayedPersistentNotification) {
  mock_platform_service_.SetPermission(blink::mojom::PermissionStatus::GRANTED);

  scoped_refptr<ServiceWorkerRegistration> registration;
  RegisterServiceWorker(&registration);

  DisplayPersistentNotificationSync(
      registration->id(), PlatformNotificationData(), NotificationResources());

  ASSERT_EQ(blink::mojom::PersistentNotificationError::NONE,
            display_persistent_callback_result_);

  // Wait for service to receive the Display call.
  RunAllTasksUntilIdle();

  std::set<std::string> notification_ids = GetDisplayedNotifications();
  ASSERT_EQ(1u, notification_ids.size());

  notification_service_->ClosePersistentNotification(*notification_ids.begin());

  // Wait for service to receive the Close call.
  RunAllTasksUntilIdle();

  EXPECT_EQ(0u, GetDisplayedNotifications().size());
}

TEST_F(BlinkNotificationServiceImplTest,
       ClosePersistentNotificationDeletesFromDatabase) {
  mock_platform_service_.SetPermission(blink::mojom::PermissionStatus::GRANTED);

  scoped_refptr<ServiceWorkerRegistration> registration;
  RegisterServiceWorker(&registration);

  DisplayPersistentNotificationSync(
      registration->id(), PlatformNotificationData(), NotificationResources());

  ASSERT_EQ(blink::mojom::PersistentNotificationError::NONE,
            display_persistent_callback_result_);

  // Wait for service to receive the Display call.
  RunAllTasksUntilIdle();

  std::set<std::string> notification_ids = GetDisplayedNotifications();
  ASSERT_EQ(1u, notification_ids.size());

  std::string notification_id = *notification_ids.begin();

  // Check data was indeed written.
  ASSERT_EQ(true /* success */, ReadNotificationData(notification_id));

  notification_service_->ClosePersistentNotification(notification_id);

  // Wait for service to receive the Close call.
  RunAllTasksUntilIdle();

  // Data should now be deleted.
  EXPECT_EQ(false /* success */, ReadNotificationData(notification_id));
}

TEST_F(BlinkNotificationServiceImplTest,
       DisplayPersistentNotificationWithoutPermission) {
  mock_platform_service_.SetPermission(blink::mojom::PermissionStatus::DENIED);

  scoped_refptr<ServiceWorkerRegistration> registration;
  RegisterServiceWorker(&registration);

  DisplayPersistentNotificationSync(
      registration->id(), PlatformNotificationData(), NotificationResources());

  EXPECT_EQ(blink::mojom::PersistentNotificationError::PERMISSION_DENIED,
            display_persistent_callback_result_);

  // Give Service a chance to receive any unexpected Display calls.
  RunAllTasksUntilIdle();

  EXPECT_EQ(0u, GetDisplayedNotifications().size());
}

TEST_F(BlinkNotificationServiceImplTest,
       DisplayMultiplePersistentNotifications) {
  mock_platform_service_.SetPermission(blink::mojom::PermissionStatus::GRANTED);

  scoped_refptr<ServiceWorkerRegistration> registration;
  RegisterServiceWorker(&registration);

  DisplayPersistentNotificationSync(
      registration->id(), PlatformNotificationData(), NotificationResources());

  DisplayPersistentNotificationSync(
      registration->id(), PlatformNotificationData(), NotificationResources());

  // Wait for service to receive all the Display calls.
  RunAllTasksUntilIdle();

  EXPECT_EQ(2u, GetDisplayedNotifications().size());
}

TEST_F(BlinkNotificationServiceImplTest, GetNotifications) {
  mock_platform_service_.SetPermission(blink::mojom::PermissionStatus::GRANTED);

  scoped_refptr<ServiceWorkerRegistration> registration;
  RegisterServiceWorker(&registration);

  EXPECT_EQ(
      0u, GetNotificationsSync(registration->id(), "" /* filter_tag */).size());

  DisplayPersistentNotificationSync(
      registration->id(), PlatformNotificationData(), NotificationResources());

  // Wait for service to receive all the Display calls.
  RunAllTasksUntilIdle();

  EXPECT_EQ(
      1u, GetNotificationsSync(registration->id(), "" /* filter_tag */).size());
}

TEST_F(BlinkNotificationServiceImplTest, GetNotificationsWithoutPermission) {
  mock_platform_service_.SetPermission(blink::mojom::PermissionStatus::GRANTED);

  scoped_refptr<ServiceWorkerRegistration> registration;
  RegisterServiceWorker(&registration);

  DisplayPersistentNotificationSync(
      registration->id(), PlatformNotificationData(), NotificationResources());

  // Wait for service to receive all the Display calls.
  RunAllTasksUntilIdle();

  mock_platform_service_.SetPermission(blink::mojom::PermissionStatus::DENIED);

  EXPECT_EQ(
      0u, GetNotificationsSync(registration->id(), "" /* filter_tag */).size());
}

TEST_F(BlinkNotificationServiceImplTest, GetNotificationsWithFilter) {
  mock_platform_service_.SetPermission(blink::mojom::PermissionStatus::GRANTED);

  scoped_refptr<ServiceWorkerRegistration> registration;
  RegisterServiceWorker(&registration);

  PlatformNotificationData platform_notification_data;
  platform_notification_data.tag = "tagA";

  PlatformNotificationData other_platform_notification_data;
  other_platform_notification_data.tag = "tagB";

  DisplayPersistentNotificationSync(
      registration->id(), platform_notification_data, NotificationResources());

  DisplayPersistentNotificationSync(registration->id(),
                                    other_platform_notification_data,
                                    NotificationResources());

  // Wait for service to receive all the Display calls.
  RunAllTasksUntilIdle();

  EXPECT_EQ(2u, GetNotificationsSync(registration->id(), "").size());
  EXPECT_EQ(1u, GetNotificationsSync(registration->id(), "tagA").size());
  EXPECT_EQ(1u, GetNotificationsSync(registration->id(), "tagB").size());
  EXPECT_EQ(0u, GetNotificationsSync(registration->id(), "tagC").size());
  EXPECT_EQ(0u, GetNotificationsSync(registration->id(), "tag").size());
}

}  // namespace content
