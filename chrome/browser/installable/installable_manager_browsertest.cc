// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/installable/installable_manager.h"

#include "base/command_line.h"
#include "base/run_loop.h"
#include "base/strings/utf_string_conversions.h"
#include "base/test/histogram_tester.h"
#include "chrome/browser/banners/app_banner_manager_desktop.h"
#include "chrome/browser/installable/installable_manager.h"
#include "chrome/browser/installable/installable_metrics.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/ui_test_utils.h"
#include "content/public/test/browser_test_utils.h"
#include "net/test/embedded_test_server/embedded_test_server.h"

using IconPurpose = blink::Manifest::Icon::IconPurpose;

namespace {

InstallableParams GetManifestParams() {
  InstallableParams params;
  params.check_eligibility = true;
  params.wait_for_worker = true;
  return params;
}

InstallableParams GetWebAppParams() {
  InstallableParams params = GetManifestParams();
  params.valid_manifest = true;
  params.has_worker = true;
  params.valid_primary_icon = true;
  params.wait_for_worker = true;
  return params;
}

InstallableParams GetPrimaryIconParams() {
  InstallableParams params = GetManifestParams();
  params.valid_primary_icon = true;
  params.wait_for_worker = true;
  return params;
}

InstallableParams GetPrimaryIconAndBadgeIconParams() {
  InstallableParams params = GetManifestParams();
  params.valid_primary_icon = true;
  params.valid_badge_icon = true;
  params.wait_for_worker = true;
  return params;
}

}  // anonymous namespace

// Used only for testing pages with no service workers. This class will dispatch
// a RunLoop::QuitClosure when it begins waiting for a service worker to be
// registered.
class LazyWorkerInstallableManager : public InstallableManager {
 public:
  LazyWorkerInstallableManager(content::WebContents* web_contents,
                               base::Closure quit_closure)
      : InstallableManager(web_contents), quit_closure_(quit_closure) {}
  ~LazyWorkerInstallableManager() override {}

 protected:
  void OnWaitingForServiceWorker() override { quit_closure_.Run(); };

 private:
  base::Closure quit_closure_;
};

// Used only for testing pages where the manifest URL is changed. This class
// will dispatch a RunLoop::QuitClosure when internal state is reset.
class ResetDataInstallableManager : public InstallableManager {
 public:
  explicit ResetDataInstallableManager(content::WebContents* web_contents)
      : InstallableManager(web_contents) {}
  ~ResetDataInstallableManager() override {}

  void SetQuitClosure(base::Closure quit_closure) {
    quit_closure_ = quit_closure;
  }

 protected:
  void OnResetData() override {
    if (quit_closure_)
      quit_closure_.Run();
  }

 private:
  base::Closure quit_closure_;
};

class CallbackTester {
 public:
  explicit CallbackTester(base::Closure quit_closure)
      : quit_closure_(quit_closure) {}

  void OnDidFinishInstallableCheck(const InstallableData& data) {
    error_code_ = data.error_code;
    manifest_url_ = data.manifest_url;
    manifest_ = *data.manifest;
    primary_icon_url_ = data.primary_icon_url;
    if (data.primary_icon)
      primary_icon_.reset(new SkBitmap(*data.primary_icon));
    badge_icon_url_ = data.badge_icon_url;
    if (data.badge_icon)
      badge_icon_.reset(new SkBitmap(*data.badge_icon));
    valid_manifest_ = data.valid_manifest;
    has_worker_ = data.has_worker;
    quit_closure_.Run();
  }

  InstallableStatusCode error_code() const { return error_code_; }
  const GURL& manifest_url() const { return manifest_url_; }
  const blink::Manifest& manifest() const { return manifest_; }
  const GURL& primary_icon_url() const { return primary_icon_url_; }
  const SkBitmap* primary_icon() const { return primary_icon_.get(); }
  const GURL& badge_icon_url() const { return badge_icon_url_; }
  const SkBitmap* badge_icon() const { return badge_icon_.get(); }
  bool valid_manifest() const { return valid_manifest_; }
  bool has_worker() const { return has_worker_; }

 private:
  base::Closure quit_closure_;
  InstallableStatusCode error_code_;
  GURL manifest_url_;
  blink::Manifest manifest_;
  GURL primary_icon_url_;
  std::unique_ptr<SkBitmap> primary_icon_;
  GURL badge_icon_url_;
  std::unique_ptr<SkBitmap> badge_icon_;
  bool valid_manifest_;
  bool has_worker_;
};

class NestedCallbackTester {
 public:
  NestedCallbackTester(InstallableManager* manager,
                       const InstallableParams& params,
                       base::Closure quit_closure)
      : manager_(manager), params_(params), quit_closure_(quit_closure) {}

  void Run() {
    manager_->GetData(params_,
                      base::Bind(&NestedCallbackTester::OnDidFinishFirstCheck,
                                 base::Unretained(this)));
  }

  void OnDidFinishFirstCheck(const InstallableData& data) {
    error_code_ = data.error_code;
    manifest_url_ = data.manifest_url;
    manifest_ = *data.manifest;
    primary_icon_url_ = data.primary_icon_url;
    if (data.primary_icon)
      primary_icon_.reset(new SkBitmap(*data.primary_icon));
    valid_manifest_ = data.valid_manifest;
    has_worker_ = data.has_worker;

    manager_->GetData(params_,
                      base::Bind(&NestedCallbackTester::OnDidFinishSecondCheck,
                                 base::Unretained(this)));
  }

  void OnDidFinishSecondCheck(const InstallableData& data) {
    EXPECT_EQ(error_code_, data.error_code);
    EXPECT_EQ(manifest_url_, data.manifest_url);
    EXPECT_EQ(primary_icon_url_, data.primary_icon_url);
    EXPECT_EQ(primary_icon_.get(), data.primary_icon);
    EXPECT_EQ(valid_manifest_, data.valid_manifest);
    EXPECT_EQ(has_worker_, data.has_worker);
    EXPECT_EQ(manifest_.IsEmpty(), data.manifest->IsEmpty());
    EXPECT_EQ(manifest_.start_url, data.manifest->start_url);
    EXPECT_EQ(manifest_.display, data.manifest->display);
    EXPECT_EQ(manifest_.name, data.manifest->name);
    EXPECT_EQ(manifest_.short_name, data.manifest->short_name);

    quit_closure_.Run();
  }

 private:
  InstallableManager* manager_;
  InstallableParams params_;
  base::Closure quit_closure_;
  InstallableStatusCode error_code_;
  GURL manifest_url_;
  blink::Manifest manifest_;
  GURL primary_icon_url_;
  std::unique_ptr<SkBitmap> primary_icon_;
  bool valid_manifest_;
  bool has_worker_;
};

class InstallableManagerBrowserTest : public InProcessBrowserTest {
 public:
  void SetUpOnMainThread() override {
    ASSERT_TRUE(embedded_test_server()->Start());

    // Make sure app banners are disabled in the browser so they do not
    // interfere with the test.
    banners::AppBannerManagerDesktop::DisableTriggeringForTesting();
  }

  // Returns a test server URL to a page controlled by a service worker with
  // |manifest_url| injected as the manifest tag.
  std::string GetURLOfPageWithServiceWorkerAndManifest(
      const std::string& manifest_url) {
    return "/banners/manifest_test_page.html?manifest=" +
           embedded_test_server()->GetURL(manifest_url).spec();
  }

  void NavigateAndRunInstallableManager(Browser* browser,
                                        CallbackTester* tester,
                                        const InstallableParams& params,
                                        const std::string& url) {
    GURL test_url = embedded_test_server()->GetURL(url);
    ui_test_utils::NavigateToURL(browser, test_url);
    RunInstallableManager(browser, tester, params);
  }

  void RunInstallableManager(Browser* browser,
                             CallbackTester* tester,
                             const InstallableParams& params) {
    InstallableManager* manager = GetManager(browser);
    manager->GetData(params,
                     base::Bind(&CallbackTester::OnDidFinishInstallableCheck,
                                base::Unretained(tester)));
  }

  InstallableManager* GetManager(Browser* browser) {
    content::WebContents* web_contents =
        browser->tab_strip_model()->GetActiveWebContents();
    InstallableManager::CreateForWebContents(web_contents);
    InstallableManager* manager =
        InstallableManager::FromWebContents(web_contents);
    CHECK(manager);

    return manager;
  }
};

IN_PROC_BROWSER_TEST_F(InstallableManagerBrowserTest,
                       ManagerBeginsInEmptyState) {
  // Ensure that the InstallableManager starts off with everything null.
  InstallableManager* manager = GetManager(browser());

  EXPECT_TRUE(manager->manifest().IsEmpty());
  EXPECT_TRUE(manager->manifest_url().is_empty());
  EXPECT_TRUE(manager->icons_.empty());
  EXPECT_FALSE(manager->valid_manifest());
  EXPECT_FALSE(manager->has_worker());

  EXPECT_EQ(NO_ERROR_DETECTED, manager->manifest_error());
  EXPECT_EQ(NO_ERROR_DETECTED, manager->valid_manifest_error());
  EXPECT_EQ(NO_ERROR_DETECTED, manager->worker_error());
  EXPECT_TRUE(!manager->task_queue_.HasCurrent());
}

IN_PROC_BROWSER_TEST_F(InstallableManagerBrowserTest, ManagerInIncognito) {
  // Ensure that the InstallableManager returns an error if called in an
  // incognito profile.
  Browser* incognito_browser =
      OpenURLOffTheRecord(browser()->profile(), GURL("about:blank"));
  InstallableManager* manager = GetManager(incognito_browser);

  base::RunLoop run_loop;
  std::unique_ptr<CallbackTester> tester(
      new CallbackTester(run_loop.QuitClosure()));

  ui_test_utils::NavigateToURL(
      incognito_browser,
      embedded_test_server()->GetURL("/banners/manifest_test_page.html"));

  RunInstallableManager(incognito_browser, tester.get(), GetManifestParams());
  run_loop.Run();

  EXPECT_TRUE(manager->manifest().IsEmpty());
  EXPECT_TRUE(manager->manifest_url().is_empty());
  EXPECT_TRUE(manager->icons_.empty());
  EXPECT_FALSE(manager->valid_manifest());
  EXPECT_FALSE(manager->has_worker());

  EXPECT_EQ(IN_INCOGNITO, manager->eligibility_error());
  EXPECT_EQ(NO_ERROR_DETECTED, manager->manifest_error());
  EXPECT_EQ(NO_ERROR_DETECTED, manager->valid_manifest_error());
  EXPECT_EQ(NO_ERROR_DETECTED, manager->worker_error());
  EXPECT_TRUE(!manager->task_queue_.HasCurrent());
}

IN_PROC_BROWSER_TEST_F(InstallableManagerBrowserTest, CheckNoManifest) {
  // Ensure that a page with no manifest returns the appropriate error and with
  // null fields for everything.
  base::HistogramTester histograms;
  base::RunLoop run_loop;
  std::unique_ptr<CallbackTester> tester(
      new CallbackTester(run_loop.QuitClosure()));

  // Navigating resets histogram state, so do it before recording a histogram.
  ui_test_utils::NavigateToURL(
      browser(),
      embedded_test_server()->GetURL("/banners/no_manifest_test_page.html"));
  GetManager(browser())->RecordMenuOpenHistogram();
  RunInstallableManager(browser(), tester.get(), GetManifestParams());
  run_loop.Run();

  // If there is no manifest, everything should be empty.
  EXPECT_TRUE(tester->manifest().IsEmpty());
  EXPECT_TRUE(tester->manifest_url().is_empty());
  EXPECT_TRUE(tester->primary_icon_url().is_empty());
  EXPECT_EQ(nullptr, tester->primary_icon());
  EXPECT_FALSE(tester->valid_manifest());
  EXPECT_FALSE(tester->has_worker());
  EXPECT_TRUE(tester->badge_icon_url().is_empty());
  EXPECT_EQ(nullptr, tester->badge_icon());
  EXPECT_EQ(NO_MANIFEST, tester->error_code());

  histograms.ExpectUniqueSample(
      "Webapp.InstallabilityCheckStatus.MenuOpen",
      static_cast<int>(InstallabilityCheckStatus::NOT_STARTED), 1);

  GetManager(browser())->RecordMenuItemAddToHomescreenHistogram();
  histograms.ExpectUniqueSample(
      "Webapp.InstallabilityCheckStatus.MenuItemAddToHomescreen",
      static_cast<int>(
          InstallabilityCheckStatus::COMPLETE_NON_PROGRESSIVE_WEB_APP),
      1);

  GetManager(browser())->RecordAddToHomescreenNoTimeout();
  histograms.ExpectUniqueSample(
      "Webapp.InstallabilityCheckStatus.AddToHomescreenTimeout",
      static_cast<int>(
          AddToHomescreenTimeoutStatus::NO_TIMEOUT_NON_PROGRESSIVE_WEB_APP),
      1);
}

IN_PROC_BROWSER_TEST_F(InstallableManagerBrowserTest, CheckManifest404) {
  base::RunLoop run_loop;
  std::unique_ptr<CallbackTester> tester(
      new CallbackTester(run_loop.QuitClosure()));

  NavigateAndRunInstallableManager(browser(), tester.get(), GetManifestParams(),
                                   GetURLOfPageWithServiceWorkerAndManifest(
                                       "/banners/manifest_missing.json"));
  run_loop.Run();

  // The installable manager should return a manifest URL even if it 404s.
  // However, the check should fail with a ManifestEmpty error.
  EXPECT_TRUE(tester->manifest().IsEmpty());

  EXPECT_FALSE(tester->manifest_url().is_empty());
  EXPECT_TRUE(tester->primary_icon_url().is_empty());
  EXPECT_EQ(nullptr, tester->primary_icon());
  EXPECT_FALSE(tester->valid_manifest());
  EXPECT_FALSE(tester->has_worker());
  EXPECT_TRUE(tester->badge_icon_url().is_empty());
  EXPECT_EQ(nullptr, tester->badge_icon());
  EXPECT_EQ(MANIFEST_EMPTY, tester->error_code());
}

IN_PROC_BROWSER_TEST_F(InstallableManagerBrowserTest, CheckManifestOnly) {
  // Verify that asking for just the manifest works as expected.
  base::RunLoop run_loop;
  std::unique_ptr<CallbackTester> tester(
      new CallbackTester(run_loop.QuitClosure()));

  NavigateAndRunInstallableManager(browser(), tester.get(), GetManifestParams(),
                                   "/banners/manifest_test_page.html");
  run_loop.Run();

  EXPECT_FALSE(tester->manifest().IsEmpty());
  EXPECT_FALSE(tester->manifest_url().is_empty());

  EXPECT_TRUE(tester->primary_icon_url().is_empty());
  EXPECT_EQ(nullptr, tester->primary_icon());
  EXPECT_FALSE(tester->valid_manifest());
  EXPECT_FALSE(tester->has_worker());
  EXPECT_TRUE(tester->badge_icon_url().is_empty());
  EXPECT_EQ(nullptr, tester->badge_icon());
  EXPECT_EQ(NO_ERROR_DETECTED, tester->error_code());
}

IN_PROC_BROWSER_TEST_F(InstallableManagerBrowserTest,
                       CheckInstallableParamsDefaultConstructor) {
  // Verify that using InstallableParams' default constructor is equivalent to
  // just asking for the manifest alone.
  base::RunLoop run_loop;
  std::unique_ptr<CallbackTester> tester(
      new CallbackTester(run_loop.QuitClosure()));

  InstallableParams params;
  NavigateAndRunInstallableManager(browser(), tester.get(), params,
                                   "/banners/manifest_test_page.html");
  run_loop.Run();

  EXPECT_FALSE(tester->manifest().IsEmpty());
  EXPECT_FALSE(tester->manifest_url().is_empty());

  EXPECT_TRUE(tester->primary_icon_url().is_empty());
  EXPECT_EQ(nullptr, tester->primary_icon());
  EXPECT_FALSE(tester->valid_manifest());
  EXPECT_FALSE(tester->has_worker());
  EXPECT_TRUE(tester->badge_icon_url().is_empty());
  EXPECT_EQ(nullptr, tester->badge_icon());
  EXPECT_EQ(NO_ERROR_DETECTED, tester->error_code());
}

IN_PROC_BROWSER_TEST_F(InstallableManagerBrowserTest,
                       CheckManifestWithIconThatIsTooSmall) {
  // This page has a manifest with only a 48x48 icon which is too small to be
  // installable. Asking for a primary icon should fail with NO_ACCEPTABLE_ICON.
  {
    base::RunLoop run_loop;
    std::unique_ptr<CallbackTester> tester(
        new CallbackTester(run_loop.QuitClosure()));

    NavigateAndRunInstallableManager(
        browser(), tester.get(), GetPrimaryIconParams(),
        GetURLOfPageWithServiceWorkerAndManifest(
            "/banners/manifest_too_small_icon.json"));
    run_loop.Run();

    EXPECT_FALSE(tester->manifest().IsEmpty());
    EXPECT_FALSE(tester->manifest_url().is_empty());

    EXPECT_TRUE(tester->primary_icon_url().is_empty());
    EXPECT_EQ(nullptr, tester->primary_icon());
    EXPECT_FALSE(tester->valid_manifest());
    EXPECT_FALSE(tester->has_worker());
    EXPECT_TRUE(tester->badge_icon_url().is_empty());
    EXPECT_EQ(nullptr, tester->badge_icon());
    EXPECT_EQ(NO_ACCEPTABLE_ICON, tester->error_code());
  }

  // Ask for everything except badge icon. This should fail with
  // NO_ACCEPTABLE_ICON - the primary icon fetch has already failed, so that
  // cached error stops the installable check from being performed.
  {
    base::RunLoop run_loop;
    std::unique_ptr<CallbackTester> tester(
        new CallbackTester(run_loop.QuitClosure()));

    RunInstallableManager(browser(), tester.get(), GetWebAppParams());
    run_loop.Run();

    EXPECT_FALSE(tester->manifest().IsEmpty());
    EXPECT_FALSE(tester->manifest_url().is_empty());

    EXPECT_TRUE(tester->primary_icon_url().is_empty());
    EXPECT_EQ(nullptr, tester->primary_icon());
    EXPECT_FALSE(tester->valid_manifest());
    EXPECT_FALSE(tester->has_worker());
    EXPECT_TRUE(tester->badge_icon_url().is_empty());
    EXPECT_EQ(nullptr, tester->badge_icon());
    EXPECT_EQ(NO_ACCEPTABLE_ICON, tester->error_code());
  }

  // Ask for a badge icon. This should fail to get a badge icon but not record
  // an error.
  {
    base::RunLoop run_loop;
    std::unique_ptr<CallbackTester> tester(
        new CallbackTester(run_loop.QuitClosure()));

    InstallableParams params = GetPrimaryIconAndBadgeIconParams();
    params.valid_primary_icon = false;
    RunInstallableManager(browser(), tester.get(), params);
    run_loop.Run();

    EXPECT_FALSE(tester->manifest().IsEmpty());
    EXPECT_FALSE(tester->manifest_url().is_empty());

    EXPECT_TRUE(tester->primary_icon_url().is_empty());
    EXPECT_EQ(nullptr, tester->primary_icon());
    EXPECT_TRUE(tester->badge_icon_url().is_empty());
    EXPECT_EQ(nullptr, tester->badge_icon());
    EXPECT_FALSE(tester->valid_manifest());
    EXPECT_FALSE(tester->has_worker());
    EXPECT_EQ(NO_ERROR_DETECTED, tester->error_code());
  }
}

IN_PROC_BROWSER_TEST_F(InstallableManagerBrowserTest,
                       CheckManifestWithOnlyRelatedApplications) {
  // This page has a manifest with only related applications specified. Asking
  // for just the manifest should succeed.
  {
    base::RunLoop run_loop;
    std::unique_ptr<CallbackTester> tester(
        new CallbackTester(run_loop.QuitClosure()));

    NavigateAndRunInstallableManager(browser(), tester.get(),
                                     GetManifestParams(),
                                     GetURLOfPageWithServiceWorkerAndManifest(
                                         "/banners/play_app_manifest.json"));
    run_loop.Run();

    EXPECT_FALSE(tester->manifest().IsEmpty());
    EXPECT_FALSE(tester->manifest_url().is_empty());
    EXPECT_TRUE(tester->manifest().prefer_related_applications);

    EXPECT_TRUE(tester->primary_icon_url().is_empty());
    EXPECT_EQ(nullptr, tester->primary_icon());
    EXPECT_FALSE(tester->valid_manifest());
    EXPECT_FALSE(tester->has_worker());
    EXPECT_TRUE(tester->badge_icon_url().is_empty());
    EXPECT_EQ(nullptr, tester->badge_icon());
    EXPECT_EQ(NO_ERROR_DETECTED, tester->error_code());
  }

  // Ask for a primary icon (but don't navigate). This should fail with
  // NO_ACCEPTABLE_ICON.
  {
    base::RunLoop run_loop;
    std::unique_ptr<CallbackTester> tester(
        new CallbackTester(run_loop.QuitClosure()));

    RunInstallableManager(browser(), tester.get(), GetPrimaryIconParams());
    run_loop.Run();

    EXPECT_FALSE(tester->manifest().IsEmpty());
    EXPECT_FALSE(tester->manifest_url().is_empty());
    EXPECT_TRUE(tester->manifest().prefer_related_applications);

    EXPECT_TRUE(tester->primary_icon_url().is_empty());
    EXPECT_EQ(nullptr, tester->primary_icon());
    EXPECT_FALSE(tester->valid_manifest());
    EXPECT_FALSE(tester->has_worker());
    EXPECT_TRUE(tester->badge_icon_url().is_empty());
    EXPECT_EQ(nullptr, tester->badge_icon());
    EXPECT_EQ(NO_ACCEPTABLE_ICON, tester->error_code());
  }

  // Ask for everything except badge icon. This should fail with
  // NO_ACCEPTABLE_ICON - the primary icon fetch has already failed, so that
  // cached error stops the installable check from being performed.
  {
    base::RunLoop run_loop;
    std::unique_ptr<CallbackTester> tester(
        new CallbackTester(run_loop.QuitClosure()));

    RunInstallableManager(browser(), tester.get(), GetWebAppParams());
    run_loop.Run();

    EXPECT_FALSE(tester->manifest().IsEmpty());
    EXPECT_FALSE(tester->manifest_url().is_empty());
    EXPECT_TRUE(tester->manifest().prefer_related_applications);

    EXPECT_TRUE(tester->primary_icon_url().is_empty());
    EXPECT_EQ(nullptr, tester->primary_icon());
    EXPECT_FALSE(tester->valid_manifest());
    EXPECT_FALSE(tester->has_worker());
    EXPECT_TRUE(tester->badge_icon_url().is_empty());
    EXPECT_EQ(nullptr, tester->badge_icon());
    EXPECT_EQ(NO_ACCEPTABLE_ICON, tester->error_code());
  }

  // Do not ask for primary icon. This should fail with START_URL_NOT_VALID.
  {
    base::RunLoop run_loop;
    std::unique_ptr<CallbackTester> tester(
        new CallbackTester(run_loop.QuitClosure()));

    InstallableParams params = GetWebAppParams();
    params.valid_primary_icon = false;
    RunInstallableManager(browser(), tester.get(), params);
    run_loop.Run();

    EXPECT_FALSE(tester->manifest().IsEmpty());
    EXPECT_FALSE(tester->manifest_url().is_empty());
    EXPECT_TRUE(tester->manifest().prefer_related_applications);

    EXPECT_TRUE(tester->primary_icon_url().is_empty());
    EXPECT_EQ(nullptr, tester->primary_icon());
    EXPECT_TRUE(tester->badge_icon_url().is_empty());
    EXPECT_EQ(nullptr, tester->badge_icon());
    EXPECT_FALSE(tester->valid_manifest());
    EXPECT_FALSE(tester->has_worker());
    EXPECT_EQ(START_URL_NOT_VALID, tester->error_code());
  }
}

IN_PROC_BROWSER_TEST_F(InstallableManagerBrowserTest, CheckManifestAndIcon) {
  // Add to homescreen checks for manifest + primary icon.
  {
    base::RunLoop run_loop;
    std::unique_ptr<CallbackTester> tester(
        new CallbackTester(run_loop.QuitClosure()));

    NavigateAndRunInstallableManager(browser(), tester.get(),
                                     GetPrimaryIconParams(),
                                     "/banners/manifest_test_page.html");
    run_loop.Run();

    EXPECT_FALSE(tester->manifest().IsEmpty());
    EXPECT_FALSE(tester->manifest_url().is_empty());

    EXPECT_FALSE(tester->primary_icon_url().is_empty());
    EXPECT_NE(nullptr, tester->primary_icon());
    EXPECT_FALSE(tester->valid_manifest());
    EXPECT_FALSE(tester->has_worker());
    EXPECT_TRUE(tester->badge_icon_url().is_empty());
    EXPECT_EQ(nullptr, tester->badge_icon());
    EXPECT_EQ(NO_ERROR_DETECTED, tester->error_code());
  }

  // Add to homescreen checks for manifest + primary icon + badge icon.
  {
    base::RunLoop run_loop;
    std::unique_ptr<CallbackTester> tester(
        new CallbackTester(run_loop.QuitClosure()));

    RunInstallableManager(browser(), tester.get(),
                          GetPrimaryIconAndBadgeIconParams());
    run_loop.Run();

    EXPECT_FALSE(tester->manifest().IsEmpty());
    EXPECT_FALSE(tester->manifest_url().is_empty());

    EXPECT_FALSE(tester->primary_icon_url().is_empty());
    EXPECT_NE(nullptr, tester->primary_icon());
    EXPECT_FALSE(tester->valid_manifest());
    EXPECT_FALSE(tester->has_worker());
    EXPECT_FALSE(tester->badge_icon_url().is_empty());
    EXPECT_NE(nullptr, tester->badge_icon());
    EXPECT_EQ(NO_ERROR_DETECTED, tester->error_code());
  }

  // Navigate to a page with a bad badge icon. This should now fail with
  // NO_ICON_AVAILABLE, but still have the manifest and primary icon.
  {
    base::RunLoop run_loop;
    std::unique_ptr<CallbackTester> tester(
        new CallbackTester(run_loop.QuitClosure()));

    NavigateAndRunInstallableManager(browser(), tester.get(),
                                     GetPrimaryIconAndBadgeIconParams(),
                                     GetURLOfPageWithServiceWorkerAndManifest(
                                         "/banners/manifest_bad_badge.json"));
    run_loop.Run();

    EXPECT_FALSE(tester->manifest().IsEmpty());
    EXPECT_FALSE(tester->manifest_url().is_empty());

    EXPECT_FALSE(tester->primary_icon_url().is_empty());
    EXPECT_NE(nullptr, tester->primary_icon());
    EXPECT_TRUE(tester->badge_icon_url().is_empty());
    EXPECT_EQ(nullptr, tester->badge_icon());
    EXPECT_FALSE(tester->valid_manifest());
    EXPECT_FALSE(tester->has_worker());
    EXPECT_EQ(NO_ICON_AVAILABLE, tester->error_code());
  }
}

IN_PROC_BROWSER_TEST_F(InstallableManagerBrowserTest, CheckWebapp) {
  // Request everything except badge icon.
  {
    base::HistogramTester histograms;
    base::RunLoop run_loop;
    std::unique_ptr<CallbackTester> tester(
        new CallbackTester(run_loop.QuitClosure()));

    // Navigating resets histogram state, so do it before recording a histogram.
    ui_test_utils::NavigateToURL(
        browser(),
        embedded_test_server()->GetURL("/banners/manifest_test_page.html"));
    GetManager(browser())->RecordMenuItemAddToHomescreenHistogram();
    RunInstallableManager(browser(), tester.get(), GetWebAppParams());
    run_loop.Run();

    EXPECT_FALSE(tester->manifest().IsEmpty());
    EXPECT_FALSE(tester->manifest_url().is_empty());
    EXPECT_FALSE(tester->primary_icon_url().is_empty());
    EXPECT_NE(nullptr, tester->primary_icon());
    EXPECT_TRUE(tester->valid_manifest());
    EXPECT_TRUE(tester->has_worker());
    EXPECT_TRUE(tester->badge_icon_url().is_empty());
    EXPECT_EQ(nullptr, tester->badge_icon());
    EXPECT_EQ(NO_ERROR_DETECTED, tester->error_code());

    // Verify that the returned state matches manager internal state.
    InstallableManager* manager = GetManager(browser());

    EXPECT_FALSE(manager->manifest().IsEmpty());
    EXPECT_FALSE(manager->manifest_url().is_empty());
    EXPECT_TRUE(manager->valid_manifest());
    EXPECT_TRUE(manager->has_worker());
    EXPECT_EQ(1u, manager->icons_.size());
    EXPECT_FALSE((manager->icon_url(IconPurpose::ANY).is_empty()));
    EXPECT_NE(nullptr, (manager->icon(IconPurpose::ANY)));
    EXPECT_EQ(NO_ERROR_DETECTED, manager->manifest_error());
    EXPECT_EQ(NO_ERROR_DETECTED, manager->valid_manifest_error());
    EXPECT_EQ(NO_ERROR_DETECTED, manager->worker_error());
    EXPECT_EQ(NO_ERROR_DETECTED, (manager->icon_error(IconPurpose::ANY)));
    EXPECT_TRUE(!manager->task_queue_.HasCurrent());

    histograms.ExpectUniqueSample(
        "Webapp.InstallabilityCheckStatus.MenuItemAddToHomescreen",
        static_cast<int>(InstallabilityCheckStatus::NOT_STARTED), 1);

    GetManager(browser())->RecordMenuOpenHistogram();
    histograms.ExpectUniqueSample(
        "Webapp.InstallabilityCheckStatus.MenuOpen",
        static_cast<int>(
            InstallabilityCheckStatus::COMPLETE_PROGRESSIVE_WEB_APP),
        1);

    GetManager(browser())->RecordAddToHomescreenNoTimeout();
    histograms.ExpectUniqueSample(
        "Webapp.InstallabilityCheckStatus.AddToHomescreenTimeout",
        static_cast<int>(
            AddToHomescreenTimeoutStatus::NO_TIMEOUT_PROGRESSIVE_WEB_APP),
        1);
  }

  // Request everything except badge icon again without navigating away. This
  // should work fine.
  {
    base::RunLoop run_loop;
    std::unique_ptr<CallbackTester> tester(
        new CallbackTester(run_loop.QuitClosure()));

    RunInstallableManager(browser(), tester.get(), GetWebAppParams());
    run_loop.Run();

    EXPECT_FALSE(tester->manifest().IsEmpty());
    EXPECT_FALSE(tester->manifest_url().is_empty());
    EXPECT_FALSE(tester->primary_icon_url().is_empty());
    EXPECT_NE(nullptr, tester->primary_icon());
    EXPECT_TRUE(tester->valid_manifest());
    EXPECT_TRUE(tester->has_worker());
    EXPECT_TRUE(tester->badge_icon_url().is_empty());
    EXPECT_EQ(nullptr, tester->badge_icon());
    EXPECT_EQ(NO_ERROR_DETECTED, tester->error_code());

    // Verify that the returned state matches manager internal state.
    InstallableManager* manager = GetManager(browser());

    EXPECT_FALSE(manager->manifest().IsEmpty());
    EXPECT_FALSE(manager->manifest_url().is_empty());
    EXPECT_TRUE(manager->valid_manifest());
    EXPECT_TRUE(manager->has_worker());
    EXPECT_EQ(1u, manager->icons_.size());
    EXPECT_FALSE((manager->icon_url(IconPurpose::ANY).is_empty()));
    EXPECT_NE(nullptr, (manager->icon(IconPurpose::ANY)));
    EXPECT_EQ(NO_ERROR_DETECTED, manager->manifest_error());
    EXPECT_EQ(NO_ERROR_DETECTED, manager->valid_manifest_error());
    EXPECT_EQ(NO_ERROR_DETECTED, manager->worker_error());
    EXPECT_EQ(NO_ERROR_DETECTED, (manager->icon_error(IconPurpose::ANY)));
    EXPECT_TRUE(!manager->task_queue_.HasCurrent());
  }

  {
    // Check that a subsequent navigation resets state.
    ui_test_utils::NavigateToURL(browser(), GURL("about:blank"));
    InstallableManager* manager = GetManager(browser());

    EXPECT_TRUE(manager->manifest().IsEmpty());
    EXPECT_TRUE(manager->manifest_url().is_empty());
    EXPECT_FALSE(manager->valid_manifest());
    EXPECT_FALSE(manager->has_worker());
    EXPECT_TRUE(manager->icons_.empty());
    EXPECT_EQ(NO_ERROR_DETECTED, manager->manifest_error());
    EXPECT_EQ(NO_ERROR_DETECTED, manager->valid_manifest_error());
    EXPECT_EQ(NO_ERROR_DETECTED, manager->worker_error());
    EXPECT_TRUE(!manager->task_queue_.HasCurrent());
  }
}

IN_PROC_BROWSER_TEST_F(InstallableManagerBrowserTest,
                       CheckNavigationWithoutRunning) {
  // Verify that we record "not started" metrics if we don't run the installable
  // manager and navigate away.
  {
    base::HistogramTester histograms;
    ui_test_utils::NavigateToURL(
        browser(),
        embedded_test_server()->GetURL("/banners/no_manifest_test_page.html"));

    InstallableManager* manager = GetManager(browser());
    manager->RecordMenuOpenHistogram();
    manager->RecordMenuOpenHistogram();
    manager->RecordMenuItemAddToHomescreenHistogram();
    manager->RecordMenuItemAddToHomescreenHistogram();
    manager->RecordAddToHomescreenInstallabilityTimeout();
    manager->RecordAddToHomescreenInstallabilityTimeout();

    ui_test_utils::NavigateToURL(browser(), GURL("about:blank"));

    histograms.ExpectUniqueSample(
        "Webapp.InstallabilityCheckStatus.MenuOpen",
        static_cast<int>(InstallabilityCheckStatus::NOT_STARTED), 2);

    histograms.ExpectUniqueSample(
        "Webapp.InstallabilityCheckStatus.MenuItemAddToHomescreen",
        static_cast<int>(InstallabilityCheckStatus::NOT_STARTED), 2);

    histograms.ExpectBucketCount(
        "Webapp.InstallabilityCheckStatus.AddToHomescreenTimeout",
        static_cast<int>(
            AddToHomescreenTimeoutStatus::TIMEOUT_INSTALLABILITY_CHECK_UNKNOWN),
        2);

    histograms.ExpectTotalCount(
        "Webapp.InstallabilityCheckStatus.AddToHomescreenTimeout", 2);
  }

  {
    // Expect the call to ManifestAndIconTimeout to kick off an installable
    // check and fail it on a not installable page.
    base::HistogramTester histograms;
    ui_test_utils::NavigateToURL(
        browser(),
        embedded_test_server()->GetURL("/banners/no_manifest_test_page.html"));

    InstallableManager* manager = GetManager(browser());
    manager->RecordAddToHomescreenManifestAndIconTimeout();

    base::RunLoop run_loop;
    std::unique_ptr<CallbackTester> tester(
        new CallbackTester(run_loop.QuitClosure()));

    // Set up a GetData call which will not record an installable metric to
    // ensure we wait until the previous check has finished.
    manager->GetData(GetManifestParams(),
                     base::Bind(&CallbackTester::OnDidFinishInstallableCheck,
                                base::Unretained(tester.get())));
    run_loop.Run();

    ui_test_utils::NavigateToURL(browser(), GURL("about:blank"));

    histograms.ExpectUniqueSample(
        "Webapp.InstallabilityCheckStatus.AddToHomescreenTimeout",
        static_cast<int>(AddToHomescreenTimeoutStatus::
                             TIMEOUT_MANIFEST_FETCH_NON_PROGRESSIVE_WEB_APP),
        1);
  }

  {
    // Expect the call to ManifestAndIconTimeout to kick off an installable
    // check and pass it on an installable page.
    base::HistogramTester histograms;
    ui_test_utils::NavigateToURL(
        browser(),
        embedded_test_server()->GetURL("/banners/manifest_test_page.html"));

    InstallableManager* manager = GetManager(browser());
    manager->RecordAddToHomescreenManifestAndIconTimeout();

    base::RunLoop run_loop;
    std::unique_ptr<CallbackTester> tester(
        new CallbackTester(run_loop.QuitClosure()));

    // Set up a GetData call which will not record an installable metric to
    // ensure we wait until the previous check has finished.
    manager->GetData(GetManifestParams(),
                     base::Bind(&CallbackTester::OnDidFinishInstallableCheck,
                                base::Unretained(tester.get())));
    run_loop.Run();

    ui_test_utils::NavigateToURL(browser(), GURL("about:blank"));

    histograms.ExpectUniqueSample(
        "Webapp.InstallabilityCheckStatus.AddToHomescreenTimeout",
        static_cast<int>(AddToHomescreenTimeoutStatus::
                             TIMEOUT_MANIFEST_FETCH_PROGRESSIVE_WEB_APP),
        1);
  }
}

IN_PROC_BROWSER_TEST_F(InstallableManagerBrowserTest, CheckWebappInIframe) {
  base::RunLoop run_loop;
  std::unique_ptr<CallbackTester> tester(
      new CallbackTester(run_loop.QuitClosure()));

  NavigateAndRunInstallableManager(browser(), tester.get(), GetWebAppParams(),
                                   "/banners/iframe_test_page.html");
  run_loop.Run();

  // The installable manager should only retrieve items in the main frame;
  // everything should be empty here.
  EXPECT_TRUE(tester->manifest().IsEmpty());
  EXPECT_TRUE(tester->manifest_url().is_empty());
  EXPECT_TRUE(tester->primary_icon_url().is_empty());
  EXPECT_EQ(nullptr, tester->primary_icon());
  EXPECT_FALSE(tester->valid_manifest());
  EXPECT_FALSE(tester->has_worker());
  EXPECT_TRUE(tester->badge_icon_url().is_empty());
  EXPECT_EQ(nullptr, tester->badge_icon());
  EXPECT_EQ(NO_MANIFEST, tester->error_code());
}

IN_PROC_BROWSER_TEST_F(InstallableManagerBrowserTest,
                       CheckPageWithManifestAndNoServiceWorker) {
  // Just fetch the manifest. This should have no error.
  {
    base::RunLoop run_loop;
    std::unique_ptr<CallbackTester> tester(
        new CallbackTester(run_loop.QuitClosure()));

    NavigateAndRunInstallableManager(
        browser(), tester.get(), GetManifestParams(),
        "/banners/manifest_no_service_worker.html");
    run_loop.Run();

    EXPECT_FALSE(tester->manifest().IsEmpty());
    EXPECT_FALSE(tester->manifest_url().is_empty());

    EXPECT_TRUE(tester->primary_icon_url().is_empty());
    EXPECT_EQ(nullptr, tester->primary_icon());
    EXPECT_FALSE(tester->valid_manifest());
    EXPECT_FALSE(tester->has_worker());
    EXPECT_TRUE(tester->badge_icon_url().is_empty());
    EXPECT_EQ(nullptr, tester->badge_icon());
    EXPECT_EQ(NO_ERROR_DETECTED, tester->error_code());
  }

  // Fetching the full criteria should fail if we don't wait for the worker.
  {
    base::RunLoop run_loop;
    std::unique_ptr<CallbackTester> tester(
        new CallbackTester(run_loop.QuitClosure()));

    InstallableParams params = GetWebAppParams();
    params.wait_for_worker = false;
    RunInstallableManager(browser(), tester.get(), params);
    run_loop.Run();

    EXPECT_FALSE(tester->manifest().IsEmpty());
    EXPECT_FALSE(tester->manifest_url().is_empty());

    EXPECT_FALSE(tester->primary_icon_url().is_empty());
    EXPECT_NE(nullptr, tester->primary_icon());
    EXPECT_TRUE(tester->valid_manifest());
    EXPECT_FALSE(tester->has_worker());
    EXPECT_TRUE(tester->badge_icon_url().is_empty());
    EXPECT_EQ(nullptr, tester->badge_icon());
    EXPECT_EQ(NO_MATCHING_SERVICE_WORKER, tester->error_code());
  }
}

IN_PROC_BROWSER_TEST_F(InstallableManagerBrowserTest,
                       CheckLazyServiceWorkerPassesWhenWaiting) {
  base::RunLoop tester_run_loop, sw_run_loop;
  std::unique_ptr<CallbackTester> tester(
      new CallbackTester(tester_run_loop.QuitClosure()));

  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  auto manager = std::make_unique<LazyWorkerInstallableManager>(
      web_contents, sw_run_loop.QuitClosure());

  {
    // Load a URL with no service worker.
    GURL test_url = embedded_test_server()->GetURL(
        "/banners/manifest_no_service_worker.html");
    ui_test_utils::NavigateToURL(browser(), test_url);

    // Kick off fetching the data. This should block on waiting for a worker.
    manager->GetData(GetWebAppParams(),
                     base::Bind(&CallbackTester::OnDidFinishInstallableCheck,
                                base::Unretained(tester.get())));
    sw_run_loop.Run();
  }

  // We should now be waiting for the service worker.
  EXPECT_TRUE(tester->manifest().IsEmpty());
  EXPECT_FALSE(manager->manifest().IsEmpty());
  EXPECT_FALSE(manager->manifest_url().is_empty());
  EXPECT_TRUE(manager->valid_manifest());
  EXPECT_FALSE(manager->has_worker());
  EXPECT_EQ(1u, manager->icons_.size());
  EXPECT_FALSE((manager->icon_url(IconPurpose::ANY).is_empty()));
  EXPECT_NE(nullptr, (manager->icon(IconPurpose::ANY)));
  EXPECT_EQ(NO_ERROR_DETECTED, manager->manifest_error());
  EXPECT_EQ(NO_ERROR_DETECTED, manager->valid_manifest_error());
  EXPECT_EQ(NO_ERROR_DETECTED, manager->worker_error());
  EXPECT_EQ(NO_ERROR_DETECTED, (manager->icon_error(IconPurpose::ANY)));
  EXPECT_TRUE(!manager->task_queue_.HasCurrent());
  EXPECT_TRUE(!manager->task_queue_.paused_tasks_.empty());

  {
    // Fetching just the manifest and icons should not hang while the other call
    // is waiting for a worker.
    base::RunLoop run_loop;
    std::unique_ptr<CallbackTester> nested_tester(
        new CallbackTester(run_loop.QuitClosure()));
    manager->GetData(GetPrimaryIconParams(),
                     base::Bind(&CallbackTester::OnDidFinishInstallableCheck,
                                base::Unretained(nested_tester.get())));
    run_loop.Run();

    EXPECT_FALSE(nested_tester->manifest().IsEmpty());
    EXPECT_FALSE(nested_tester->manifest_url().is_empty());
    EXPECT_FALSE(nested_tester->primary_icon_url().is_empty());
    EXPECT_NE(nullptr, nested_tester->primary_icon());
    EXPECT_TRUE(nested_tester->valid_manifest());
    EXPECT_FALSE(nested_tester->has_worker());
    EXPECT_TRUE(nested_tester->badge_icon_url().is_empty());
    EXPECT_EQ(nullptr, nested_tester->badge_icon());
    EXPECT_EQ(NO_ERROR_DETECTED, nested_tester->error_code());
  }

  // Load the service worker.
  EXPECT_TRUE(content::ExecuteScript(
      web_contents, "navigator.serviceWorker.register('service_worker.js');"));
  tester_run_loop.Run();

  // We should have passed now.
  EXPECT_FALSE(tester->manifest().IsEmpty());
  EXPECT_FALSE(tester->manifest_url().is_empty());
  EXPECT_FALSE(tester->primary_icon_url().is_empty());
  EXPECT_NE(nullptr, tester->primary_icon());
  EXPECT_TRUE(tester->valid_manifest());
  EXPECT_TRUE(tester->has_worker());
  EXPECT_TRUE(tester->badge_icon_url().is_empty());
  EXPECT_EQ(nullptr, tester->badge_icon());
  EXPECT_EQ(NO_ERROR_DETECTED, tester->error_code());

  // Verify internal state.
  EXPECT_FALSE(manager->manifest().IsEmpty());
  EXPECT_FALSE(manager->manifest_url().is_empty());
  EXPECT_TRUE(manager->valid_manifest());
  EXPECT_TRUE(manager->has_worker());
  EXPECT_EQ(1u, manager->icons_.size());
  EXPECT_FALSE((manager->icon_url(IconPurpose::ANY).is_empty()));
  EXPECT_NE(nullptr, (manager->icon(IconPurpose::ANY)));
  EXPECT_EQ(NO_ERROR_DETECTED, manager->manifest_error());
  EXPECT_EQ(NO_ERROR_DETECTED, manager->valid_manifest_error());
  EXPECT_EQ(NO_ERROR_DETECTED, manager->worker_error());
  EXPECT_EQ(NO_ERROR_DETECTED, (manager->icon_error(IconPurpose::ANY)));
  EXPECT_TRUE(!manager->task_queue_.HasCurrent());
  EXPECT_FALSE(!manager->task_queue_.paused_tasks_.empty());
}

IN_PROC_BROWSER_TEST_F(InstallableManagerBrowserTest,
                       CheckLazyServiceWorkerNoFetchHandlerFails) {
  base::RunLoop tester_run_loop, sw_run_loop;
  std::unique_ptr<CallbackTester> tester(
      new CallbackTester(tester_run_loop.QuitClosure()));

  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  auto manager = std::make_unique<LazyWorkerInstallableManager>(
      web_contents, sw_run_loop.QuitClosure());

  // Load a URL with no service worker.
  GURL test_url = embedded_test_server()->GetURL(
      "/banners/manifest_no_service_worker.html");
  ui_test_utils::NavigateToURL(browser(), test_url);

  // Kick off fetching the data. This should block on waiting for a worker.
  manager->GetData(GetWebAppParams(),
                   base::Bind(&CallbackTester::OnDidFinishInstallableCheck,
                              base::Unretained(tester.get())));
  sw_run_loop.Run();

  // We should now be waiting for the service worker.
  EXPECT_TRUE(!manager->task_queue_.HasCurrent());
  EXPECT_TRUE(!manager->task_queue_.paused_tasks_.empty());

  // Load the service worker with no fetch handler.
  EXPECT_TRUE(content::ExecuteScript(web_contents,
                                     "navigator.serviceWorker.register('"
                                     "service_worker_no_fetch_handler.js');"));
  tester_run_loop.Run();

  // We should fail the check.
  EXPECT_FALSE(tester->manifest().IsEmpty());
  EXPECT_FALSE(tester->manifest_url().is_empty());
  EXPECT_FALSE(tester->primary_icon_url().is_empty());
  EXPECT_NE(nullptr, tester->primary_icon());
  EXPECT_TRUE(tester->valid_manifest());
  EXPECT_FALSE(tester->has_worker());
  EXPECT_TRUE(tester->badge_icon_url().is_empty());
  EXPECT_EQ(nullptr, tester->badge_icon());
  EXPECT_EQ(NOT_OFFLINE_CAPABLE, tester->error_code());
}

IN_PROC_BROWSER_TEST_F(InstallableManagerBrowserTest,
                       WaitingForServiceWorkerRecordsNonPwa) {
  base::RunLoop tester_run_loop, sw_run_loop;
  base::HistogramTester histograms;
  std::unique_ptr<CallbackTester> tester(
      new CallbackTester(tester_run_loop.QuitClosure()));

  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  auto manager = std::make_unique<LazyWorkerInstallableManager>(
      web_contents, sw_run_loop.QuitClosure());

  manager->RecordMenuOpenHistogram();
  manager->RecordMenuItemAddToHomescreenHistogram();

  {
    // Load a URL with no service worker.
    GURL test_url = embedded_test_server()->GetURL(
        "/banners/manifest_no_service_worker.html");
    ui_test_utils::NavigateToURL(browser(), test_url);

    // Kick off fetching the data. This should block on waiting for a worker.
    manager->GetData(GetWebAppParams(),
                     base::Bind(&CallbackTester::OnDidFinishInstallableCheck,
                                base::Unretained(tester.get())));
    sw_run_loop.Run();
  }

  manager->RecordMenuOpenHistogram();
  manager->RecordMenuOpenHistogram();
  manager->RecordMenuItemAddToHomescreenHistogram();

  // Navigate to force metrics recording.
  ui_test_utils::NavigateToURL(browser(), GURL("about:blank"));

  // Expect to record that we completed the check and found a non-PWA since we
  // waited until navigation and didn't get a service worker.
  histograms.ExpectBucketCount(
      "Webapp.InstallabilityCheckStatus.MenuOpen",
      static_cast<int>(InstallabilityCheckStatus::NOT_STARTED), 1);
  histograms.ExpectBucketCount(
      "Webapp.InstallabilityCheckStatus.MenuOpen",
      static_cast<int>(
          InstallabilityCheckStatus::COMPLETE_NON_PROGRESSIVE_WEB_APP),
      2);
  histograms.ExpectTotalCount("Webapp.InstallabilityCheckStatus.MenuOpen", 3);
  histograms.ExpectBucketCount(
      "Webapp.InstallabilityCheckStatus.MenuItemAddToHomescreen",
      static_cast<int>(InstallabilityCheckStatus::NOT_STARTED), 1);
  histograms.ExpectBucketCount(
      "Webapp.InstallabilityCheckStatus.MenuItemAddToHomescreen",
      static_cast<int>(
          InstallabilityCheckStatus::COMPLETE_NON_PROGRESSIVE_WEB_APP),
      1);
  histograms.ExpectTotalCount(
      "Webapp.InstallabilityCheckStatus.MenuItemAddToHomescreen", 2);
}

IN_PROC_BROWSER_TEST_F(InstallableManagerBrowserTest,
                       CheckServiceWorkerErrorIsNotCached) {
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  base::RunLoop sw_run_loop;
  auto manager = std::make_unique<LazyWorkerInstallableManager>(
      web_contents, sw_run_loop.QuitClosure());

  // Load a URL with no service worker.
  GURL test_url = embedded_test_server()->GetURL(
      "/banners/manifest_no_service_worker.html");
  ui_test_utils::NavigateToURL(browser(), test_url);

  {
    base::RunLoop tester_run_loop;
    std::unique_ptr<CallbackTester> tester(
        new CallbackTester(tester_run_loop.QuitClosure()));

    InstallableParams params = GetWebAppParams();
    params.wait_for_worker = false;
    manager->GetData(params,
                     base::Bind(&CallbackTester::OnDidFinishInstallableCheck,
                                base::Unretained(tester.get())));
    tester_run_loop.Run();

    // We should have returned with an error.
    EXPECT_FALSE(tester->manifest().IsEmpty());
    EXPECT_TRUE(tester->valid_manifest());
    EXPECT_FALSE(tester->has_worker());
    EXPECT_EQ(NO_MATCHING_SERVICE_WORKER, tester->error_code());
  }

  {
    base::RunLoop tester_run_loop;
    std::unique_ptr<CallbackTester> tester(
        new CallbackTester(tester_run_loop.QuitClosure()));

    InstallableParams params = GetWebAppParams();
    manager->GetData(params,
                     base::Bind(&CallbackTester::OnDidFinishInstallableCheck,
                                base::Unretained(tester.get())));
    sw_run_loop.Run();

    EXPECT_TRUE(content::ExecuteScript(
        web_contents,
        "navigator.serviceWorker.register('service_worker.js');"));
    tester_run_loop.Run();

    // The callback should tell us that the page is installable
    EXPECT_FALSE(tester->manifest().IsEmpty());
    EXPECT_TRUE(tester->valid_manifest());
    EXPECT_TRUE(tester->has_worker());
    EXPECT_EQ(NO_ERROR_DETECTED, tester->error_code());
  }
}

IN_PROC_BROWSER_TEST_F(InstallableManagerBrowserTest,
                       CheckPageWithNoServiceWorkerFetchHandler) {
  base::RunLoop run_loop;
  std::unique_ptr<CallbackTester> tester(
      new CallbackTester(run_loop.QuitClosure()));

  NavigateAndRunInstallableManager(
      browser(), tester.get(), GetWebAppParams(),
      "/banners/no_sw_fetch_handler_test_page.html");

  RunInstallableManager(browser(), tester.get(), GetWebAppParams());
  run_loop.Run();

  EXPECT_FALSE(tester->manifest().IsEmpty());
  EXPECT_FALSE(tester->manifest_url().is_empty());

  EXPECT_FALSE(tester->primary_icon_url().is_empty());
  EXPECT_NE(nullptr, tester->primary_icon());
  EXPECT_TRUE(tester->valid_manifest());
  EXPECT_FALSE(tester->has_worker());
  EXPECT_TRUE(tester->badge_icon_url().is_empty());
  EXPECT_EQ(nullptr, tester->badge_icon());
  EXPECT_EQ(NOT_OFFLINE_CAPABLE, tester->error_code());
}

IN_PROC_BROWSER_TEST_F(InstallableManagerBrowserTest, CheckDataUrlIcon) {
  // Verify that InstallableManager can handle data URL icons.
  base::RunLoop run_loop;
  std::unique_ptr<CallbackTester> tester(
      new CallbackTester(run_loop.QuitClosure()));

  NavigateAndRunInstallableManager(browser(), tester.get(), GetWebAppParams(),
                                   GetURLOfPageWithServiceWorkerAndManifest(
                                       "/banners/manifest_data_url_icon.json"));
  run_loop.Run();

  EXPECT_FALSE(tester->manifest().IsEmpty());
  EXPECT_FALSE(tester->manifest_url().is_empty());

  EXPECT_FALSE(tester->primary_icon_url().is_empty());
  EXPECT_NE(nullptr, tester->primary_icon());
  EXPECT_EQ(144, tester->primary_icon()->width());
  EXPECT_TRUE(tester->valid_manifest());
  EXPECT_TRUE(tester->has_worker());
  EXPECT_TRUE(tester->badge_icon_url().is_empty());
  EXPECT_EQ(nullptr, tester->badge_icon());
  EXPECT_EQ(NO_ERROR_DETECTED, tester->error_code());
}

IN_PROC_BROWSER_TEST_F(InstallableManagerBrowserTest,
                       CheckManifestCorruptedIcon) {
  // Verify that the returned InstallableData::primary_icon is null if the web
  // manifest points to a corrupt primary icon.
  base::RunLoop run_loop;
  std::unique_ptr<CallbackTester> tester(
      new CallbackTester(run_loop.QuitClosure()));

  NavigateAndRunInstallableManager(browser(), tester.get(),
                                   GetPrimaryIconParams(),
                                   GetURLOfPageWithServiceWorkerAndManifest(
                                       "/banners/manifest_bad_icon.json"));
  run_loop.Run();

  EXPECT_FALSE(tester->manifest().IsEmpty());
  EXPECT_FALSE(tester->manifest_url().is_empty());
  EXPECT_TRUE(tester->primary_icon_url().is_empty());
  EXPECT_EQ(nullptr, tester->primary_icon());
  EXPECT_FALSE(tester->valid_manifest());
  EXPECT_FALSE(tester->has_worker());
  EXPECT_TRUE(tester->badge_icon_url().is_empty());
  EXPECT_EQ(nullptr, tester->badge_icon());
  EXPECT_EQ(NO_ICON_AVAILABLE, tester->error_code());
}

IN_PROC_BROWSER_TEST_F(InstallableManagerBrowserTest,
                       CheckChangeInIconDimensions) {
  // Verify that a follow-up request for a primary icon with a different size
  // works.
  {
    base::RunLoop run_loop;
    std::unique_ptr<CallbackTester> tester(
        new CallbackTester(run_loop.QuitClosure()));

    NavigateAndRunInstallableManager(browser(), tester.get(), GetWebAppParams(),
                                     "/banners/manifest_test_page.html");
    run_loop.Run();

    EXPECT_FALSE(tester->manifest_url().is_empty());
    EXPECT_FALSE(tester->manifest().IsEmpty());
    EXPECT_FALSE(tester->primary_icon_url().is_empty());
    EXPECT_NE(nullptr, tester->primary_icon());
    EXPECT_TRUE(tester->valid_manifest());
    EXPECT_TRUE(tester->has_worker());
    EXPECT_TRUE(tester->badge_icon_url().is_empty());
    EXPECT_EQ(nullptr, tester->badge_icon());
    EXPECT_EQ(NO_ERROR_DETECTED, tester->error_code());
  }

  {
    base::RunLoop run_loop;
    std::unique_ptr<CallbackTester> tester(
        new CallbackTester(run_loop.QuitClosure()));
    RunInstallableManager(browser(), tester.get(), GetWebAppParams());

    run_loop.Run();

    // The smaller primary icon requirements should allow this to pass.
    EXPECT_FALSE(tester->manifest_url().is_empty());
    EXPECT_FALSE(tester->manifest().IsEmpty());
    EXPECT_FALSE(tester->primary_icon_url().is_empty());
    EXPECT_NE(nullptr, tester->primary_icon());
    EXPECT_TRUE(tester->valid_manifest());
    EXPECT_TRUE(tester->has_worker());
    EXPECT_TRUE(tester->badge_icon_url().is_empty());
    EXPECT_EQ(nullptr, tester->badge_icon());
    EXPECT_EQ(NO_ERROR_DETECTED, tester->error_code());
  }
}

IN_PROC_BROWSER_TEST_F(InstallableManagerBrowserTest,
                       CheckNestedCallsToGetData) {
  // Verify that we can call GetData while in a callback from GetData.
  base::RunLoop run_loop;
  InstallableParams params = GetWebAppParams();
  std::unique_ptr<NestedCallbackTester> tester(new NestedCallbackTester(
      GetManager(browser()), params, run_loop.QuitClosure()));

  tester->Run();
  run_loop.Run();
}

IN_PROC_BROWSER_TEST_F(InstallableManagerBrowserTest,
                       ManifestUrlChangeFlushesState) {
  content::WebContents* web_contents =
      browser()->tab_strip_model()->GetActiveWebContents();
  auto manager = std::make_unique<ResetDataInstallableManager>(web_contents);

  // Start on a page with no manifest.
  ui_test_utils::NavigateToURL(
      browser(),
      embedded_test_server()->GetURL("/banners/no_manifest_test_page.html"));

  {
    // Fetch the data. This should return an empty manifest.
    base::RunLoop run_loop;
    std::unique_ptr<CallbackTester> tester(
        new CallbackTester(run_loop.QuitClosure()));

    manager->GetData(GetWebAppParams(),
                     base::Bind(&CallbackTester::OnDidFinishInstallableCheck,
                                base::Unretained(tester.get())));
    run_loop.Run();

    EXPECT_TRUE(tester->manifest().IsEmpty());
    EXPECT_EQ(NO_MANIFEST, manager->manifest_error());
    EXPECT_EQ(NO_MANIFEST, tester->error_code());
  }

  {
    // Injecting a manifest URL but not navigating should flush the state.
    base::RunLoop run_loop;
    manager->SetQuitClosure(run_loop.QuitClosure());
    EXPECT_TRUE(content::ExecuteScript(web_contents, "addManifestLinkTag()"));
    run_loop.Run();

    EXPECT_TRUE(manager->manifest().IsEmpty());
    EXPECT_EQ(NO_ERROR_DETECTED, manager->manifest_error());
  }

  {
    // Fetch the data again. This should succeed.
    base::RunLoop run_loop;
    std::unique_ptr<CallbackTester> tester(
        new CallbackTester(run_loop.QuitClosure()));

    manager->GetData(GetWebAppParams(),
                     base::Bind(&CallbackTester::OnDidFinishInstallableCheck,
                                base::Unretained(tester.get())));
    run_loop.Run();

    EXPECT_FALSE(tester->manifest().IsEmpty());
    EXPECT_EQ(NO_ERROR_DETECTED, tester->error_code());
    EXPECT_EQ(base::ASCIIToUTF16("Manifest test app"),
              tester->manifest().name.string());
    EXPECT_EQ(base::string16(), tester->manifest().short_name.string());
  }

  {
    // Flush the state again by changing the manifest URL.
    base::RunLoop run_loop;
    manager->SetQuitClosure(run_loop.QuitClosure());

    GURL manifest_url = embedded_test_server()->GetURL(
        "/banners/manifest_short_name_only.json");
    EXPECT_TRUE(content::ExecuteScript(
        web_contents, "changeManifestUrl('" + manifest_url.spec() + "');"));
    run_loop.Run();

    EXPECT_TRUE(manager->manifest().IsEmpty());
  }

  {
    // Fetch again. This should return the data from the new manifest.
    base::RunLoop run_loop;
    std::unique_ptr<CallbackTester> tester(
        new CallbackTester(run_loop.QuitClosure()));

    manager->GetData(GetWebAppParams(),
                     base::Bind(&CallbackTester::OnDidFinishInstallableCheck,
                                base::Unretained(tester.get())));
    run_loop.Run();

    EXPECT_FALSE(tester->manifest().IsEmpty());
    EXPECT_EQ(base::string16(), tester->manifest().name.string());
    EXPECT_EQ(base::ASCIIToUTF16("Manifest"),
              tester->manifest().short_name.string());
    EXPECT_EQ(NO_ERROR_DETECTED, tester->error_code());
  }
}
