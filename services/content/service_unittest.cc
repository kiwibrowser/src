// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/content/service.h"

#include "base/bind.h"
#include "base/callback.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/test/bind_test_util.h"
#include "base/test/scoped_task_environment.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "services/content/public/mojom/constants.mojom.h"
#include "services/content/public/mojom/view.mojom.h"
#include "services/content/public/mojom/view_factory.mojom.h"
#include "services/content/service_delegate.h"
#include "services/content/view_delegate.h"
#include "services/service_manager/public/cpp/test/test_connector_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace content {
namespace {

class TestViewClient : public mojom::ViewClient {
 public:
  TestViewClient() = default;
  ~TestViewClient() override = default;

 private:
  // mojom::ViewClient:
  void DidStopLoading() override {}

  DISALLOW_COPY_AND_ASSIGN(TestViewClient);
};

class TestViewDelegate : public ViewDelegate {
 public:
  TestViewDelegate() = default;
  ~TestViewDelegate() override = default;

  const GURL& last_navigated_url() const { return last_navigated_url_; }

  void set_navigation_callback(base::RepeatingClosure callback) {
    navigation_callback_ = std::move(callback);
  }

  // ViewDelegate:
  void Navigate(const GURL& url) override {
    last_navigated_url_ = url;
    if (navigation_callback_)
      navigation_callback_.Run();
  }

 private:
  GURL last_navigated_url_;
  base::RepeatingClosure navigation_callback_;

  DISALLOW_COPY_AND_ASSIGN(TestViewDelegate);
};

class TestServiceDelegate : public ServiceDelegate {
 public:
  TestServiceDelegate() = default;
  ~TestServiceDelegate() override = default;

  void set_view_delegate_created_callback(
      base::RepeatingCallback<void(TestViewDelegate*)> callback) {
    view_delegate_created_callback_ = std::move(callback);
  }

  // ServiceDelegate:
  void WillDestroyServiceInstance(Service* service) override {}

  std::unique_ptr<ViewDelegate> CreateViewDelegate(
      mojom::ViewClient* client) override {
    auto object = std::make_unique<TestViewDelegate>();
    if (view_delegate_created_callback_)
      view_delegate_created_callback_.Run(object.get());
    return object;
  }

 private:
  base::RepeatingCallback<void(TestViewDelegate*)>
      view_delegate_created_callback_;

  DISALLOW_COPY_AND_ASSIGN(TestServiceDelegate);
};

class ContentServiceTest : public testing::Test {
 public:
  ContentServiceTest() = default;
  ~ContentServiceTest() override = default;

  void SetUp() override {
    connector_factory_ =
        service_manager::TestConnectorFactory::CreateForUniqueService(
            std::make_unique<Service>(&delegate_));
    connector_ = connector_factory_->CreateConnector();
  }

 protected:
  TestServiceDelegate& delegate() { return delegate_; }

  template <typename T>
  void BindInterface(mojo::InterfaceRequest<T> request) {
    connector_->BindInterface(content::mojom::kServiceName, std::move(request));
  }

 private:
  base::test::ScopedTaskEnvironment task_environment_;
  std::unique_ptr<service_manager::TestConnectorFactory> connector_factory_;
  std::unique_ptr<service_manager::Connector> connector_;
  TestServiceDelegate delegate_;

  DISALLOW_COPY_AND_ASSIGN(ContentServiceTest);
};

TEST_F(ContentServiceTest, ViewCreation) {
  mojom::ViewFactoryPtr factory;
  BindInterface(mojo::MakeRequest(&factory));

  base::RunLoop loop;

  TestViewDelegate* view_delegate = nullptr;
  delegate().set_view_delegate_created_callback(
      base::BindLambdaForTesting([&](TestViewDelegate* delegate) {
        EXPECT_FALSE(view_delegate);
        view_delegate = delegate;
        loop.Quit();
      }));

  mojom::ViewPtr view;
  TestViewClient client_impl;
  mojom::ViewClientPtr client;
  mojo::Binding<mojom::ViewClient> client_binding(&client_impl,
                                                  mojo::MakeRequest(&client));
  factory->CreateView(mojom::ViewParams::New(), mojo::MakeRequest(&view),
                      std::move(client));
  loop.Run();

  base::RunLoop navigation_loop;
  ASSERT_TRUE(view_delegate);
  view_delegate->set_navigation_callback(navigation_loop.QuitClosure());

  const GURL kTestUrl("https://example.com/");
  view->Navigate(kTestUrl);
  navigation_loop.Run();

  EXPECT_EQ(kTestUrl, view_delegate->last_navigated_url());
}

}  // namespace
}  // namespace content
