// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/subresource_filter/content/browser/content_ruleset_service.h"

#include <stddef.h>

#include <string>
#include <tuple>
#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "base/files/file.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/numerics/safe_conversions.h"
#include "base/run_loop.h"
#include "base/task_runner.h"
#include "base/test/test_simple_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/prefs/testing_pref_service.h"
#include "components/subresource_filter/content/common/subresource_filter_messages.h"
#include "components/subresource_filter/core/browser/ruleset_service.h"
#include "components/subresource_filter/core/common/test_ruleset_creator.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_source.h"
#include "content/public/browser/notification_types.h"
#include "content/public/test/mock_render_process_host.h"
#include "content/public/test/test_browser_context.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "ipc/ipc_platform_file.h"
#include "ipc/ipc_test_sink.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace subresource_filter {

namespace {

using MockClosureTarget =
    ::testing::StrictMock<::testing::MockFunction<void()>>;

class TestContentBrowserClient : public ::content::ContentBrowserClient {
 public:
  TestContentBrowserClient() {}

  // ::content::ContentBrowserClient:
  void PostAfterStartupTask(const base::Location&,
                            const scoped_refptr<base::TaskRunner>& task_runner,
                            base::OnceClosure task) override {
    scoped_refptr<base::TaskRunner> ui_task_runner =
        content::BrowserThread::GetTaskRunnerForThread(
            content::BrowserThread::UI);
    EXPECT_EQ(ui_task_runner, task_runner);
    last_task_ = std::move(task);
  }

  void RunAfterStartupTask() {
    if (!last_task_.is_null())
      std::move(last_task_).Run();
  }

 private:
  base::OnceClosure last_task_;

  DISALLOW_COPY_AND_ASSIGN(TestContentBrowserClient);
};

class NotifyingMockRenderProcessHost : public content::MockRenderProcessHost {
 public:
  explicit NotifyingMockRenderProcessHost(
      content::BrowserContext* browser_context)
      : content::MockRenderProcessHost(browser_context) {
    content::NotificationService::current()->Notify(
        content::NOTIFICATION_RENDERER_PROCESS_CREATED,
        content::Source<content::RenderProcessHost>(this),
        content::NotificationService::NoDetails());
  }
};

std::string ReadFileContents(base::File* file) {
  size_t length = base::checked_cast<size_t>(file->GetLength());
  std::string contents(length, 0);
  file->Read(0, &contents[0], base::checked_cast<int>(length));
  return contents;
}

// Extracts and takes ownership of the ruleset file handle in the IPC message.
base::File ExtractRulesetFromMessage(const IPC::Message* message) {
  std::tuple<IPC::PlatformFileForTransit> arg;
  SubresourceFilterMsg_SetRulesetForProcess::Read(message, &arg);
  return IPC::PlatformFileForTransitToFile(std::get<0>(arg));
}

}  // namespace

class SubresourceFilterContentRulesetServiceTest : public ::testing::Test {
 public:
  SubresourceFilterContentRulesetServiceTest()
      : old_browser_client_(nullptr), existing_renderer_(&browser_context_) {}

 protected:
  void SetUp() override {
    ASSERT_TRUE(scoped_temp_dir_.CreateUniqueTempDir());
    old_browser_client_ = content::SetBrowserClientForTesting(&browser_client_);
  }

  void TearDown() override {
    content::SetBrowserClientForTesting(old_browser_client_);
  }

  content::TestBrowserContext* browser_context() { return &browser_context_; }
  TestContentBrowserClient* browser_client() { return &browser_client_; }

  base::FilePath scoped_temp_file() const {
    return scoped_temp_dir_.GetPath().AppendASCII("data");
  }

  void AssertSetRulesetForProcessMessageWithContent(
      const IPC::Message* message,
      const std::string& expected_contents) {
    ASSERT_EQ(
        static_cast<uint32_t>(SubresourceFilterMsg_SetRulesetForProcess::ID),
        message->type());
    base::File ruleset_file = ExtractRulesetFromMessage(message);
    ASSERT_TRUE(ruleset_file.IsValid());
    ASSERT_EQ(expected_contents, ReadFileContents(&ruleset_file));
  }

 private:
  base::ScopedTempDir scoped_temp_dir_;
  TestContentBrowserClient browser_client_;
  content::ContentBrowserClient* old_browser_client_;
  content::TestBrowserThreadBundle thread_bundle_;
  content::TestBrowserContext browser_context_;
  NotifyingMockRenderProcessHost existing_renderer_;

  DISALLOW_COPY_AND_ASSIGN(SubresourceFilterContentRulesetServiceTest);
};

TEST_F(SubresourceFilterContentRulesetServiceTest, NoRuleset_NoIPCMessages) {
  NotifyingMockRenderProcessHost existing_renderer(browser_context());
  ContentRulesetService service(base::ThreadTaskRunnerHandle::Get());
  NotifyingMockRenderProcessHost new_renderer(browser_context());
  base::RunLoop().RunUntilIdle();
  EXPECT_EQ(0u, existing_renderer.sink().message_count());
  EXPECT_EQ(0u, new_renderer.sink().message_count());
}

TEST_F(SubresourceFilterContentRulesetServiceTest,
       PublishedRuleset_IsDistributedToExistingAndNewRenderers) {
  const char kTestFileContents[] = "foobar";
  base::WriteFile(scoped_temp_file(), kTestFileContents,
                  strlen(kTestFileContents));

  base::File file;
  file.Initialize(scoped_temp_file(),
                  base::File::FLAG_OPEN | base::File::FLAG_READ);

  NotifyingMockRenderProcessHost existing_renderer(browser_context());
  ContentRulesetService service(base::ThreadTaskRunnerHandle::Get());
  MockClosureTarget publish_callback_target;
  service.SetRulesetPublishedCallbackForTesting(base::Bind(
      &MockClosureTarget::Call, base::Unretained(&publish_callback_target)));
  EXPECT_CALL(publish_callback_target, Call()).Times(1);
  service.PublishNewRulesetVersion(std::move(file));
  base::RunLoop().RunUntilIdle();
  ::testing::Mock::VerifyAndClearExpectations(&publish_callback_target);

  ASSERT_EQ(1u, existing_renderer.sink().message_count());
  ASSERT_NO_FATAL_FAILURE(AssertSetRulesetForProcessMessageWithContent(
      existing_renderer.sink().GetMessageAt(0), kTestFileContents));

  NotifyingMockRenderProcessHost second_renderer(browser_context());
  base::RunLoop().RunUntilIdle();

  ASSERT_EQ(1u, second_renderer.sink().message_count());
  ASSERT_NO_FATAL_FAILURE(AssertSetRulesetForProcessMessageWithContent(
      second_renderer.sink().GetMessageAt(0), kTestFileContents));
}

TEST_F(SubresourceFilterContentRulesetServiceTest, PostAfterStartupTask) {
  ContentRulesetService service(base::ThreadTaskRunnerHandle::Get());

  MockClosureTarget mock_closure_target;
  service.PostAfterStartupTask(base::Bind(
      &MockClosureTarget::Call, base::Unretained(&mock_closure_target)));

  EXPECT_CALL(mock_closure_target, Call()).Times(1);
  browser_client()->RunAfterStartupTask();
}

TEST_F(SubresourceFilterContentRulesetServiceTest,
       PublishesRulesetInOnePostTask) {
  // Regression test for crbug.com/817308. Test verifies that ruleset is
  // published on browser startup via exactly one PostTask.

  // Create a temporary directory for the indexed ruleset data.
  base::ScopedTempDir scoped_temp_dir;
  ASSERT_TRUE(scoped_temp_dir.CreateUniqueTempDir());
  const base::FilePath base_dir =
      scoped_temp_dir.GetPath().AppendASCII("Rules").AppendASCII("Indexed");

  // Create a testing ruleset.
  testing::TestRulesetPair ruleset;
  ASSERT_NO_FATAL_FAILURE(
      testing::TestRulesetCreator().CreateRulesetToDisallowURLsWithPathSuffix(
          "foo", &ruleset));

  // Create local state and save the ruleset version to emulate invariant that
  // the version of the last indexed ruleset is stored in the local state.
  TestingPrefServiceSimple prefs;
  IndexedRulesetVersion::RegisterPrefs(prefs.registry());
  IndexedRulesetVersion current_version(
      "1.2.3.4", IndexedRulesetVersion::CurrentFormatVersion());
  current_version.SaveToPrefs(&prefs);

  // Create ruleset data on a disk.
  const base::FilePath version_dir_path =
      IndexedRulesetLocator::GetSubdirectoryPathForVersion(base_dir,
                                                           current_version);
  ASSERT_EQ(RulesetService::IndexAndWriteRulesetResult::SUCCESS,
            RulesetService::WriteRuleset(version_dir_path,
                                         /* license_path =*/base::FilePath(),
                                         ruleset.indexed.contents.data(),
                                         ruleset.indexed.contents.size()));

  // Create a ruleset service and its harness.
  scoped_refptr<base::TestSimpleTaskRunner> blocking_task_runner =
      base::MakeRefCounted<base::TestSimpleTaskRunner>();
  scoped_refptr<base::TestSimpleTaskRunner> background_task_runner =
      base::MakeRefCounted<base::TestSimpleTaskRunner>();
  NotifyingMockRenderProcessHost renderer_host(browser_context());
  auto service = std::make_unique<ContentRulesetService>(blocking_task_runner);
  base::RunLoop callback_waiter;
  service->SetRulesetPublishedCallbackForTesting(callback_waiter.QuitClosure());

  // |RulesetService| constructor should read the last indexed ruleset version
  // and post ruleset setup on |blocking_task_runner|. (Yes, exactly
  // |blocking_task_runner| via |ContentRulesetService| as its delegate).
  ASSERT_EQ(0u, blocking_task_runner->NumPendingTasks());
  service->set_ruleset_service(std::make_unique<RulesetService>(
      &prefs, background_task_runner, service.get(), base_dir));

  // The key test assertion is that ruleset data is published via exactly one
  // post task on |blocking_task_runner|. It is important to run pending tasks
  // only once here.
  ASSERT_EQ(1u, blocking_task_runner->NumPendingTasks());
  blocking_task_runner->RunPendingTasks();
  callback_waiter.Run();

  // Check that the ruleset data is delivered to the renderer.
  EXPECT_EQ(1u, renderer_host.sink().message_count());
  const std::string expected_data(ruleset.indexed.contents.begin(),
                                  ruleset.indexed.contents.end());
  ASSERT_NO_FATAL_FAILURE(AssertSetRulesetForProcessMessageWithContent(
      renderer_host.sink().GetMessageAt(0), expected_data));

  // |ContentRulesetService| destruction requires additional tricks. Its member
  // |VerifiedRulesetDealer::Handle| posts task upon destruction on
  // |blocking_task_runner|.
  service.reset();
  // Need to wait for |VerifiedRulesetDealer| destruction on the
  // |blocking_task_runner|.
  blocking_task_runner->RunPendingTasks();
}

}  // namespace subresource_filter
