// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/safe_browsing/phishing_classifier_delegate.h"

#include <memory>

#include "base/strings/utf_string_conversions.h"
#include "chrome/renderer/safe_browsing/features.h"
#include "chrome/renderer/safe_browsing/phishing_classifier.h"
#include "chrome/renderer/safe_browsing/scorer.h"
#include "chrome/test/base/chrome_render_view_test.h"
#include "chrome/test/base/chrome_unit_test_suite.h"
#include "components/safe_browsing/proto/csd.pb.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_view.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/service_manager/public/cpp/interface_provider.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "third_party/blink/public/platform/web_url.h"
#include "third_party/blink/public/platform/web_url_request.h"
#include "url/gurl.h"

using base::ASCIIToUTF16;
using ::testing::_;
using ::testing::InSequence;
using ::testing::Mock;
using ::testing::Pointee;
using ::testing::StrictMock;

namespace safe_browsing {

namespace {

class MockPhishingClassifier : public PhishingClassifier {
 public:
  explicit MockPhishingClassifier(content::RenderFrame* render_frame)
      : PhishingClassifier(render_frame, NULL /* clock */) {}

  ~MockPhishingClassifier() override {}

  MOCK_METHOD2(BeginClassification,
               void(const base::string16*, const DoneCallback&));
  MOCK_METHOD0(CancelPendingClassification, void());

 private:
  DISALLOW_COPY_AND_ASSIGN(MockPhishingClassifier);
};

class MockScorer : public Scorer {
 public:
  MockScorer() : Scorer() {}
  ~MockScorer() override {}

  MOCK_CONST_METHOD1(ComputeScore, double(const FeatureMap&));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockScorer);
};
}  // namespace

class FakePhishingDetectorClient : public mojom::PhishingDetectorClient {
 public:
  FakePhishingDetectorClient() = default;

  ~FakePhishingDetectorClient() override = default;

  void BindRequest(mojom::PhishingDetectorClientRequest request) {
    bindings_.AddBinding(this, std::move(request));
  }

  // mojom::PhishingDetectorClient
  void PhishingDetectionDone(const std::string& request_proto) override {
    ClientPhishingRequest verdict;
    ASSERT_TRUE(verdict.ParseFromString(request_proto));
    EXPECT_EQ("http://host.com/", verdict.url());
    EXPECT_EQ(0.8f, verdict.client_score());
    EXPECT_FALSE(verdict.is_phishing());
  }

 private:
  mojo::BindingSet<mojom::PhishingDetectorClient> bindings_;
  DISALLOW_COPY_AND_ASSIGN(FakePhishingDetectorClient);
};

class PhishingClassifierDelegateTest : public ChromeRenderViewTest {
 protected:
  void SetUp() override {
    ChromeRenderViewTest::SetUp();

    content::RenderFrame* render_frame = view_->GetMainRenderFrame();
    classifier_ = new StrictMock<MockPhishingClassifier>(render_frame);
    delegate_ = PhishingClassifierDelegate::Create(render_frame, classifier_);
  }

  void TearDown() override { ChromeRenderViewTest::TearDown(); }

  void RegisterMainFrameRemoteInterfaces() override {
    service_manager::InterfaceProvider* remote_interfaces =
        view_->GetMainRenderFrame()->GetRemoteInterfaces();
    service_manager::InterfaceProvider::TestApi test_api(remote_interfaces);
    test_api.SetBinderForName(
        mojom::PhishingDetectorClient::Name_,
        base::BindRepeating(
            &PhishingClassifierDelegateTest::BindPhishingDetectorClient,
            base::Unretained(this)));
  }

  void BindPhishingDetectorClient(mojo::ScopedMessagePipeHandle handle) {
    fake_phishing_client_.BindRequest(
        mojom::PhishingDetectorClientRequest(std::move(handle)));
  }

  // Runs the ClassificationDone callback, then verify if message sent
  // by FakeRenderThread is correct.
  void RunAndVerifyClassificationDone(const ClientPhishingRequest& verdict) {
    delegate_->ClassificationDone(verdict);
  }

  void OnStartPhishingDetection(const GURL& url) {
    delegate_->StartPhishingDetection(url);
  }

  void PageCaptured(base::string16* page_text,
                    bool preliminary_capture,
                    const GURL& page_url) {
    if (preliminary_capture)
      return;

    delegate_->CancelPendingClassification(
        PhishingClassifierDelegate::PAGE_RECAPTURED);
    delegate_->last_finished_load_url_ = page_url;
    delegate_->classifier_page_text_.swap(*page_text);
    delegate_->have_page_text_ = true;
    delegate_->MaybeStartClassification();
  }

  void SimulateRedirection(const GURL& redir_url) {
    delegate_->last_url_received_from_browser_ = redir_url;
  }

  void SimulatePageTrantitionForwardOrBack(const char* html, const char* url) {
    LoadHTMLWithUrlOverride(html, url);
    delegate_->last_main_frame_transition_ = ui::PAGE_TRANSITION_FORWARD_BACK;
  }

  StrictMock<MockPhishingClassifier>* classifier_;  // Owned by |delegate_|.
  PhishingClassifierDelegate* delegate_;            // Owned by the RenderFrame.
  FakePhishingDetectorClient fake_phishing_client_;
};

TEST_F(PhishingClassifierDelegateTest, Navigation) {
  MockScorer scorer;
  delegate_->SetPhishingScorer(&scorer);
  ASSERT_TRUE(classifier_->is_ready());

  // Test an initial load.  We expect classification to happen normally.
  EXPECT_CALL(*classifier_, CancelPendingClassification());
  std::string html = "<html><body>dummy</body></html>";
  GURL url("http://host.com/index.html");
  LoadHTMLWithUrlOverride(html.c_str(), url.spec().c_str());
  Mock::VerifyAndClearExpectations(classifier_);

  OnStartPhishingDetection(url);
  base::string16 page_text = ASCIIToUTF16("dummy");
  {
    InSequence s;
    EXPECT_CALL(*classifier_, CancelPendingClassification());
    EXPECT_CALL(*classifier_, BeginClassification(Pointee(page_text), _));
    PageCaptured(&page_text, false, url);
    Mock::VerifyAndClearExpectations(classifier_);
  }

  // Reloading the same page should not trigger a reclassification.
  // However, it will cancel any pending classification since the
  // content is being replaced.
  EXPECT_CALL(*classifier_, CancelPendingClassification());
  LoadHTMLWithUrlOverride(html.c_str(), url.spec().c_str());
  Mock::VerifyAndClearExpectations(classifier_);

  OnStartPhishingDetection(url);
  page_text = ASCIIToUTF16("dummy");
  EXPECT_CALL(*classifier_, CancelPendingClassification());
  PageCaptured(&page_text, false, url);
  Mock::VerifyAndClearExpectations(classifier_);

  OnStartPhishingDetection(url);
  page_text = ASCIIToUTF16("dummy");
  EXPECT_CALL(*classifier_, CancelPendingClassification());
  PageCaptured(&page_text, false, url);
  Mock::VerifyAndClearExpectations(classifier_);

  // Same document navigation works similarly to a subframe navigation, but see
  // the TODO in PhishingClassifierDelegate::DidCommitProvisionalLoad.
  EXPECT_CALL(*classifier_, CancelPendingClassification());
  OnSameDocumentNavigation(GetMainFrame(), true, true);
  Mock::VerifyAndClearExpectations(classifier_);

  OnStartPhishingDetection(url);
  page_text = ASCIIToUTF16("dummy");
  EXPECT_CALL(*classifier_, CancelPendingClassification());
  PageCaptured(&page_text, false, url);
  Mock::VerifyAndClearExpectations(classifier_);

  // Now load a new toplevel page, which should trigger another classification.
  EXPECT_CALL(*classifier_, CancelPendingClassification());
  GURL new_url("http://host2.com");
  LoadHTMLWithUrlOverride("dummy2", new_url.spec().c_str());
  Mock::VerifyAndClearExpectations(classifier_);

  page_text = ASCIIToUTF16("dummy2");
  OnStartPhishingDetection(new_url);
  {
    InSequence s;
    EXPECT_CALL(*classifier_, CancelPendingClassification());
    EXPECT_CALL(*classifier_, BeginClassification(Pointee(page_text), _));
    PageCaptured(&page_text, false, new_url);
    Mock::VerifyAndClearExpectations(classifier_);
  }

  // No classification should happen on back/forward navigation.
  // Note: in practice, the browser will not send a StartPhishingDetection IPC
  // in this case.  However, we want to make sure that the delegate behaves
  // correctly regardless.
  EXPECT_CALL(*classifier_, CancelPendingClassification()).Times(1);
  // Simulate a go back navigation, i.e. back to http://host.com/index.html.
  SimulatePageTrantitionForwardOrBack(html.c_str(), url.spec().c_str());
  Mock::VerifyAndClearExpectations(classifier_);
  page_text = ASCIIToUTF16("dummy");
  OnStartPhishingDetection(new_url);
  EXPECT_CALL(*classifier_, CancelPendingClassification());
  PageCaptured(&page_text, false, new_url);
  Mock::VerifyAndClearExpectations(classifier_);

  EXPECT_CALL(*classifier_, CancelPendingClassification());
  // Simulate a go forward navigation, i.e. forward to http://host.com
  SimulatePageTrantitionForwardOrBack("dummy2", new_url.spec().c_str());
  Mock::VerifyAndClearExpectations(classifier_);

  page_text = ASCIIToUTF16("dummy2");
  OnStartPhishingDetection(url);
  EXPECT_CALL(*classifier_, CancelPendingClassification());
  PageCaptured(&page_text, false, url);
  Mock::VerifyAndClearExpectations(classifier_);

  // Now go back again and navigate to a different place within
  // the same page. No classification should happen.
  EXPECT_CALL(*classifier_, CancelPendingClassification());
  // Simulate a go back again to http://host.com/index.html
  SimulatePageTrantitionForwardOrBack(html.c_str(), url.spec().c_str());
  Mock::VerifyAndClearExpectations(classifier_);

  page_text = ASCIIToUTF16("dummy");
  OnStartPhishingDetection(url);
  EXPECT_CALL(*classifier_, CancelPendingClassification());
  PageCaptured(&page_text, false, url);
  Mock::VerifyAndClearExpectations(classifier_);

  EXPECT_CALL(*classifier_, CancelPendingClassification());
  // Same document navigation.
  OnSameDocumentNavigation(GetMainFrame(), true, true);
  Mock::VerifyAndClearExpectations(classifier_);

  OnStartPhishingDetection(url);
  page_text = ASCIIToUTF16("dummy");
  EXPECT_CALL(*classifier_, CancelPendingClassification());
  PageCaptured(&page_text, false, url);
  Mock::VerifyAndClearExpectations(classifier_);

  // The delegate will cancel pending classification on destruction.
  EXPECT_CALL(*classifier_, CancelPendingClassification());
}

TEST_F(PhishingClassifierDelegateTest, NoScorer) {
  // For this test, we'll create the delegate with no scorer available yet.
  ASSERT_FALSE(classifier_->is_ready());

  // Queue up a pending classification, cancel it, then queue up another one.
  GURL url("http://host.com");
  base::string16 page_text = ASCIIToUTF16("dummy");
  LoadHTMLWithUrlOverride("dummy", url.spec().c_str());
  OnStartPhishingDetection(url);
  PageCaptured(&page_text, false, url);

  GURL url2("http://host2.com");
  page_text = ASCIIToUTF16("dummy");
  LoadHTMLWithUrlOverride("dummy", url2.spec().c_str());
  OnStartPhishingDetection(url2);
  PageCaptured(&page_text, false, url2);

  // Now set a scorer, which should cause a classifier to be created and
  // the classification to proceed.
  page_text = ASCIIToUTF16("dummy");
  EXPECT_CALL(*classifier_, BeginClassification(Pointee(page_text), _));
  MockScorer scorer;
  delegate_->SetPhishingScorer(&scorer);
  Mock::VerifyAndClearExpectations(classifier_);

  // If we set a new scorer while a classification is going on the
  // classification should be cancelled.
  EXPECT_CALL(*classifier_, CancelPendingClassification());
  delegate_->SetPhishingScorer(&scorer);
  Mock::VerifyAndClearExpectations(classifier_);

  // The delegate will cancel pending classification on destruction.
  EXPECT_CALL(*classifier_, CancelPendingClassification());
}

TEST_F(PhishingClassifierDelegateTest, NoScorer_Ref) {
  // Similar to the last test, but navigates within the page before
  // setting the scorer.
  ASSERT_FALSE(classifier_->is_ready());

  // Queue up a pending classification, cancel it, then queue up another one.
  GURL url("http://host.com");
  base::string16 page_text = ASCIIToUTF16("dummy");
  LoadHTMLWithUrlOverride("dummy", url.spec().c_str());
  OnStartPhishingDetection(url);
  PageCaptured(&page_text, false, url);

  page_text = ASCIIToUTF16("dummy");
  OnStartPhishingDetection(url);
  PageCaptured(&page_text, false, url);

  // Now set a scorer, which should cause a classifier to be created and
  // the classification to proceed.
  page_text = ASCIIToUTF16("dummy");
  EXPECT_CALL(*classifier_, BeginClassification(Pointee(page_text), _));
  MockScorer scorer;
  delegate_->SetPhishingScorer(&scorer);
  Mock::VerifyAndClearExpectations(classifier_);

  // If we set a new scorer while a classification is going on the
  // classification should be cancelled.
  EXPECT_CALL(*classifier_, CancelPendingClassification());
  delegate_->SetPhishingScorer(&scorer);
  Mock::VerifyAndClearExpectations(classifier_);

  // The delegate will cancel pending classification on destruction.
  EXPECT_CALL(*classifier_, CancelPendingClassification());
}

TEST_F(PhishingClassifierDelegateTest, NoStartPhishingDetection) {
  // Tests the behavior when OnStartPhishingDetection has not yet been called
  // when the page load finishes.
  MockScorer scorer;
  delegate_->SetPhishingScorer(&scorer);
  ASSERT_TRUE(classifier_->is_ready());

  EXPECT_CALL(*classifier_, CancelPendingClassification());
  GURL url("http://host.com");
  LoadHTMLWithUrlOverride("<html><body>phish</body></html>",
                          url.spec().c_str());
  Mock::VerifyAndClearExpectations(classifier_);
  base::string16 page_text = ASCIIToUTF16("phish");
  EXPECT_CALL(*classifier_, CancelPendingClassification());
  PageCaptured(&page_text, false, url);
  Mock::VerifyAndClearExpectations(classifier_);
  // Now simulate the StartPhishingDetection IPC.  We expect classification
  // to begin.
  page_text = ASCIIToUTF16("phish");
  EXPECT_CALL(*classifier_, BeginClassification(Pointee(page_text), _));
  OnStartPhishingDetection(url);
  Mock::VerifyAndClearExpectations(classifier_);

  // Now try again, but this time we will navigate the page away before
  // the IPC is sent.
  EXPECT_CALL(*classifier_, CancelPendingClassification());
  GURL url2("http://host2.com");
  LoadHTMLWithUrlOverride("<html><body>phish</body></html>",
                          url2.spec().c_str());
  Mock::VerifyAndClearExpectations(classifier_);
  page_text = ASCIIToUTF16("phish");
  EXPECT_CALL(*classifier_, CancelPendingClassification());
  PageCaptured(&page_text, false, url2);
  Mock::VerifyAndClearExpectations(classifier_);

  EXPECT_CALL(*classifier_, CancelPendingClassification());
  GURL url3("http://host3.com");
  LoadHTMLWithUrlOverride("<html><body>phish</body></html>",
                          url3.spec().c_str());
  Mock::VerifyAndClearExpectations(classifier_);
  OnStartPhishingDetection(url);

  // In this test, the original page is a redirect, which we do not get a
  // StartPhishingDetection IPC for.  We simulate the redirection event to
  // load a new page while reusing the original session history entry, and
  // check that classification begins correctly for the landing page.
  EXPECT_CALL(*classifier_, CancelPendingClassification());
  GURL url4("http://host4.com");
  LoadHTMLWithUrlOverride("<html><body>phish</body></html>",
                          url4.spec().c_str());
  Mock::VerifyAndClearExpectations(classifier_);
  page_text = ASCIIToUTF16("abc");
  EXPECT_CALL(*classifier_, CancelPendingClassification());
  PageCaptured(&page_text, false, url4);
  Mock::VerifyAndClearExpectations(classifier_);
  EXPECT_CALL(*classifier_, CancelPendingClassification());

  GURL redir_url("http://host4.com/redir");
  LoadHTMLWithUrlOverride("123", redir_url.spec().c_str());
  Mock::VerifyAndClearExpectations(classifier_);
  OnStartPhishingDetection(url4);
  page_text = ASCIIToUTF16("123");
  {
    InSequence s;
    EXPECT_CALL(*classifier_, CancelPendingClassification());
    EXPECT_CALL(*classifier_, BeginClassification(Pointee(page_text), _));
    SimulateRedirection(redir_url);
    PageCaptured(&page_text, false, redir_url);
    Mock::VerifyAndClearExpectations(classifier_);
  }

  // The delegate will cancel pending classification on destruction.
  EXPECT_CALL(*classifier_, CancelPendingClassification());
}

TEST_F(PhishingClassifierDelegateTest, IgnorePreliminaryCapture) {
  // Tests that preliminary PageCaptured notifications are ignored.
  MockScorer scorer;
  delegate_->SetPhishingScorer(&scorer);
  ASSERT_TRUE(classifier_->is_ready());

  EXPECT_CALL(*classifier_, CancelPendingClassification());
  GURL url("http://host.com");
  LoadHTMLWithUrlOverride("<html><body>phish</body></html>",
                          url.spec().c_str());
  Mock::VerifyAndClearExpectations(classifier_);
  OnStartPhishingDetection(url);
  base::string16 page_text = ASCIIToUTF16("phish");
  PageCaptured(&page_text, true, url);

  // Once the non-preliminary capture happens, classification should begin.
  page_text = ASCIIToUTF16("phish");
  {
    InSequence s;
    EXPECT_CALL(*classifier_, CancelPendingClassification());
    EXPECT_CALL(*classifier_, BeginClassification(Pointee(page_text), _));
    PageCaptured(&page_text, false, url);
    Mock::VerifyAndClearExpectations(classifier_);
  }

  // The delegate will cancel pending classification on destruction.
  EXPECT_CALL(*classifier_, CancelPendingClassification());
}

TEST_F(PhishingClassifierDelegateTest, DuplicatePageCapture) {
  // Tests that a second PageCaptured notification causes classification to
  // be cancelled.
  MockScorer scorer;
  delegate_->SetPhishingScorer(&scorer);
  ASSERT_TRUE(classifier_->is_ready());

  EXPECT_CALL(*classifier_, CancelPendingClassification());
  GURL url("http://host.com");
  LoadHTMLWithUrlOverride("<html><body>phish</body></html>",
                          url.spec().c_str());
  Mock::VerifyAndClearExpectations(classifier_);
  OnStartPhishingDetection(url);
  base::string16 page_text = ASCIIToUTF16("phish");
  {
    InSequence s;
    EXPECT_CALL(*classifier_, CancelPendingClassification());
    EXPECT_CALL(*classifier_, BeginClassification(Pointee(page_text), _));
    PageCaptured(&page_text, false, url);
    Mock::VerifyAndClearExpectations(classifier_);
  }

  page_text = ASCIIToUTF16("phish");
  EXPECT_CALL(*classifier_, CancelPendingClassification());
  PageCaptured(&page_text, false, url);
  Mock::VerifyAndClearExpectations(classifier_);

  // The delegate will cancel pending classification on destruction.
  EXPECT_CALL(*classifier_, CancelPendingClassification());
}

TEST_F(PhishingClassifierDelegateTest, PhishingDetectionDone) {
  // Tests that a SafeBrowsingHostMsg_PhishingDetectionDone IPC is
  // sent to the browser whenever we finish classification.
  MockScorer scorer;
  delegate_->SetPhishingScorer(&scorer);
  ASSERT_TRUE(classifier_->is_ready());

  // Start by loading a page to populate the delegate's state.
  EXPECT_CALL(*classifier_, CancelPendingClassification());
  GURL url("http://host.com");
  LoadHTMLWithUrlOverride("<html><body>phish</body></html>",
                          url.spec().c_str());
  Mock::VerifyAndClearExpectations(classifier_);
  base::string16 page_text = ASCIIToUTF16("phish");
  OnStartPhishingDetection(url);
  {
    InSequence s;
    EXPECT_CALL(*classifier_, CancelPendingClassification());
    EXPECT_CALL(*classifier_, BeginClassification(Pointee(page_text), _));
    PageCaptured(&page_text, false, url);
    Mock::VerifyAndClearExpectations(classifier_);
  }

  // Now run the callback to simulate the classifier finishing.
  ClientPhishingRequest verdict;
  verdict.set_url(url.spec());
  verdict.set_client_score(0.8f);
  verdict.set_is_phishing(false);  // Send IPC even if site is not phishing.
  RunAndVerifyClassificationDone(verdict);

  // The delegate will cancel pending classification on destruction.
  EXPECT_CALL(*classifier_, CancelPendingClassification());
}

}  // namespace safe_browsing
