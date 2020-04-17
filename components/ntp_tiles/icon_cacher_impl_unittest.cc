// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/ntp_tiles/icon_cacher_impl.h"

#include <memory>
#include <utility>

#include "base/containers/flat_set.h"
#include "base/files/scoped_temp_dir.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/test/metrics/histogram_tester.h"
#include "base/test/mock_callback.h"
#include "base/test/scoped_task_environment.h"
#include "base/test/test_simple_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "components/favicon/core/favicon_client.h"
#include "components/favicon/core/favicon_service_impl.h"
#include "components/favicon/core/favicon_util.h"
#include "components/favicon/core/large_icon_service.h"
#include "components/favicon_base/favicon_types.h"
#include "components/history/core/browser/history_database_params.h"
#include "components/history/core/browser/history_service.h"
#include "components/image_fetcher/core/image_decoder.h"
#include "components/image_fetcher/core/image_fetcher.h"
#include "components/image_fetcher/core/mock_image_decoder.h"
#include "components/image_fetcher/core/mock_image_fetcher.h"
#include "components/image_fetcher/core/request_metadata.h"
#include "net/traffic_annotation/network_traffic_annotation_test_helper.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/ui_base_paths.h"
#include "ui/gfx/image/image_unittest_util.h"

using base::Bucket;
using ::image_fetcher::MockImageFetcher;
using ::testing::_;
using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::Invoke;
using ::testing::InSequence;
using ::testing::IsEmpty;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnArg;

namespace ntp_tiles {
namespace {
using MockImageDecoder = image_fetcher::MockImageDecoder;

// This class provides methods to inject an image resource where a real resource
// would be necessary otherwise. All other methods have return values that allow
// the normal implementation to proceed.
class MockResourceDelegate : public ui::ResourceBundle::Delegate {
 public:
  ~MockResourceDelegate() override {}

  MOCK_METHOD1(GetImageNamed, gfx::Image(int resource_id));
  MOCK_METHOD1(GetNativeImageNamed, gfx::Image(int resource_id));

  MOCK_METHOD2(GetPathForResourcePack,
               base::FilePath(const base::FilePath& pack_path,
                              ui::ScaleFactor scale_factor));

  MOCK_METHOD2(GetPathForLocalePack,
               base::FilePath(const base::FilePath& pack_path,
                              const std::string& locale));

  MOCK_METHOD2(LoadDataResourceBytes,
               base::RefCountedMemory*(int resource_id,
                                       ui::ScaleFactor scale_factor));

  MOCK_METHOD3(GetRawDataResource,
               bool(int resource_id,
                    ui::ScaleFactor scale_factor,
                    base::StringPiece* value));

  MOCK_METHOD2(GetLocalizedString, bool(int message_id, base::string16* value));
};

ACTION(FailFetch) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(*arg3), arg0, gfx::Image(),
                                image_fetcher::RequestMetadata()));
}

ACTION_P2(DecodeSuccessfully, width, height) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::Bind(arg2, gfx::test::CreateImage(width, height)));
}

ACTION_P2(PassFetch, width, height) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(std::move(*arg3), arg0,
                                gfx::test::CreateImage(width, height),
                                image_fetcher::RequestMetadata()));
}

ACTION_P(Quit, run_loop) {
  run_loop->Quit();
}

// TODO(jkrcal): Split off large_icon_service.h and large_icon_service_impl.h.
// Use then mocks of FaviconService and LargeIconService instead of the real
// things.
class IconCacherTestBase : public ::testing::Test {
 protected:
  IconCacherTestBase()
      : favicon_service_(/*favicon_client=*/nullptr, &history_service_) {
    CHECK(history_dir_.CreateUniqueTempDir());
    CHECK(history_service_.Init(
        history::HistoryDatabaseParams(history_dir_.GetPath(), 0, 0)));
  }

  void PreloadIcon(const GURL& url,
                   const GURL& icon_url,
                   favicon_base::IconType icon_type,
                   int width,
                   int height) {
    favicon_service_.SetFavicons({url}, icon_url, icon_type,
                                 gfx::test::CreateImage(width, height));
  }

  bool IconIsCachedFor(const GURL& url, favicon_base::IconType icon_type) {
    return !GetCachedIconFor(url, icon_type).IsEmpty();
  }

  gfx::Image GetCachedIconFor(const GURL& url,
                              favicon_base::IconType icon_type) {
    base::CancelableTaskTracker tracker;
    gfx::Image image;
    base::RunLoop loop;
    favicon::GetFaviconImageForPageURL(
        &favicon_service_, url, icon_type,
        base::Bind(
            [](gfx::Image* image, base::RunLoop* loop,
               const favicon_base::FaviconImageResult& result) {
              *image = result.image;
              loop->Quit();
            },
            &image, &loop),
        &tracker);
    loop.Run();
    return image;
  }

  void WaitForHistoryThreadTasksToFinish() {
    base::RunLoop loop;
    base::MockCallback<base::Closure> done;
    EXPECT_CALL(done, Run()).WillOnce(Quit(&loop));
    history_service_.FlushForTest(done.Get());
    loop.Run();
  }

  void WaitForMainThreadTasksToFinish() {
    base::RunLoop loop;
    loop.RunUntilIdle();
  }

  base::test::ScopedTaskEnvironment scoped_task_environment_;
  base::ScopedTempDir history_dir_;
  history::HistoryService history_service_;
  favicon::FaviconServiceImpl favicon_service_;
};

class IconCacherTestPopularSites : public IconCacherTestBase {
 protected:
  IconCacherTestPopularSites()
      : site_(base::string16(),  // title, unused
              GURL("http://url.google/"),
              GURL("http://url.google/icon.png"),
              GURL("http://url.google/favicon.ico"),
              GURL(),                     // thumbnail, unused
              TileTitleSource::UNKNOWN),  // title_source, unused
        image_fetcher_(new ::testing::StrictMock<MockImageFetcher>),
        image_decoder_(new ::testing::StrictMock<MockImageDecoder>) {}

  void SetUp() override {
    if (ui::ResourceBundle::HasSharedInstance()) {
      ui::ResourceBundle::CleanupSharedInstance();
    }
    ON_CALL(mock_resource_delegate_, GetPathForResourcePack(_, _))
        .WillByDefault(ReturnArg<0>());
    ON_CALL(mock_resource_delegate_, GetPathForLocalePack(_, _))
        .WillByDefault(ReturnArg<0>());
    ui::ResourceBundle::InitSharedInstanceWithLocale(
        "en-US", &mock_resource_delegate_,
        ui::ResourceBundle::LOAD_COMMON_RESOURCES);
  }

  void TearDown() override {
    if (ui::ResourceBundle::HasSharedInstance()) {
      ui::ResourceBundle::CleanupSharedInstance();
    }
    base::FilePath pak_path;
#if defined(OS_ANDROID)
    base::PathService::Get(ui::DIR_RESOURCE_PAKS_ANDROID, &pak_path);
#else
    base::PathService::Get(base::DIR_MODULE, &pak_path);
#endif

    base::FilePath ui_test_pak_path;
    ASSERT_TRUE(base::PathService::Get(ui::UI_TEST_PAK, &ui_test_pak_path));
    ui::ResourceBundle::InitSharedInstanceWithPakPath(ui_test_pak_path);

    ui::ResourceBundle::GetSharedInstance().AddDataPackFromPath(
        pak_path.AppendASCII("components_tests_resources.pak"),
        ui::SCALE_FACTOR_NONE);
  }

  PopularSites::Site site_;
  std::unique_ptr<MockImageFetcher> image_fetcher_;
  std::unique_ptr<MockImageDecoder> image_decoder_;
  NiceMock<MockResourceDelegate> mock_resource_delegate_;
};

TEST_F(IconCacherTestPopularSites, LargeCached) {
  base::HistogramTester histogram_tester;
  base::MockCallback<base::Closure> done;
  EXPECT_CALL(done, Run()).Times(0);
  base::RunLoop loop;
  {
    InSequence s;
    EXPECT_CALL(*image_fetcher_,
                SetDataUseServiceName(
                    data_use_measurement::DataUseUserData::NTP_TILES));
    EXPECT_CALL(*image_fetcher_, SetDesiredImageFrameSize(gfx::Size(128, 128)));
  }
  PreloadIcon(site_.url, site_.large_icon_url,
              favicon_base::IconType::kTouchIcon, 128, 128);
  IconCacherImpl cacher(&favicon_service_, nullptr, std::move(image_fetcher_));
  cacher.StartFetchPopularSites(site_, done.Get(), done.Get());
  WaitForMainThreadTasksToFinish();
  EXPECT_FALSE(IconIsCachedFor(site_.url, favicon_base::IconType::kFavicon));
  EXPECT_TRUE(IconIsCachedFor(site_.url, favicon_base::IconType::kTouchIcon));
  EXPECT_THAT(histogram_tester.GetAllSamples(
                  "NewTabPage.TileFaviconFetchSuccess.Popular"),
              IsEmpty());
}

TEST_F(IconCacherTestPopularSites, LargeNotCachedAndFetchSucceeded) {
  base::HistogramTester histogram_tester;
  base::MockCallback<base::Closure> done;
  base::RunLoop loop;
  {
    InSequence s;
    EXPECT_CALL(*image_fetcher_,
                SetDataUseServiceName(
                    data_use_measurement::DataUseUserData::NTP_TILES));
    EXPECT_CALL(*image_fetcher_, SetDesiredImageFrameSize(gfx::Size(128, 128)));
    EXPECT_CALL(*image_fetcher_,
                FetchImageAndData_(_, site_.large_icon_url, _, _, _))
        .WillOnce(PassFetch(128, 128));
    EXPECT_CALL(done, Run()).WillOnce(Quit(&loop));
  }

  IconCacherImpl cacher(&favicon_service_, nullptr, std::move(image_fetcher_));
  cacher.StartFetchPopularSites(site_, done.Get(), done.Get());
  loop.Run();
  EXPECT_FALSE(IconIsCachedFor(site_.url, favicon_base::IconType::kFavicon));
  EXPECT_TRUE(IconIsCachedFor(site_.url, favicon_base::IconType::kTouchIcon));
  EXPECT_THAT(histogram_tester.GetAllSamples(
                  "NewTabPage.TileFaviconFetchSuccess.Popular"),
              ElementsAre(Bucket(/*bucket=*/true, /*count=*/1)));
}

TEST_F(IconCacherTestPopularSites, SmallNotCachedAndFetchSucceeded) {
  site_.large_icon_url = GURL();

  base::MockCallback<base::Closure> done;
  base::RunLoop loop;
  {
    InSequence s;
    EXPECT_CALL(*image_fetcher_,
                SetDataUseServiceName(
                    data_use_measurement::DataUseUserData::NTP_TILES));
    EXPECT_CALL(*image_fetcher_, SetDesiredImageFrameSize(gfx::Size(128, 128)));
    EXPECT_CALL(*image_fetcher_,
                FetchImageAndData_(_, site_.favicon_url, _, _, _))
        .WillOnce(PassFetch(128, 128));
    EXPECT_CALL(done, Run()).WillOnce(Quit(&loop));
  }

  IconCacherImpl cacher(&favicon_service_, nullptr, std::move(image_fetcher_));
  cacher.StartFetchPopularSites(site_, done.Get(), done.Get());
  loop.Run();
  EXPECT_TRUE(IconIsCachedFor(site_.url, favicon_base::IconType::kFavicon));
  EXPECT_FALSE(IconIsCachedFor(site_.url, favicon_base::IconType::kTouchIcon));
}

TEST_F(IconCacherTestPopularSites, LargeNotCachedAndFetchFailed) {
  base::HistogramTester histogram_tester;
  base::MockCallback<base::Closure> done;
  EXPECT_CALL(done, Run()).Times(0);
  {
    InSequence s;
    EXPECT_CALL(*image_fetcher_,
                SetDataUseServiceName(
                    data_use_measurement::DataUseUserData::NTP_TILES));
    EXPECT_CALL(*image_fetcher_, SetDesiredImageFrameSize(gfx::Size(128, 128)));
    EXPECT_CALL(*image_fetcher_,
                FetchImageAndData_(_, site_.large_icon_url, _, _, _))
        .WillOnce(FailFetch());
  }

  IconCacherImpl cacher(&favicon_service_, nullptr, std::move(image_fetcher_));
  cacher.StartFetchPopularSites(site_, done.Get(), done.Get());
  WaitForMainThreadTasksToFinish();
  EXPECT_FALSE(IconIsCachedFor(site_.url, favicon_base::IconType::kFavicon));
  EXPECT_FALSE(IconIsCachedFor(site_.url, favicon_base::IconType::kTouchIcon));
  EXPECT_THAT(histogram_tester.GetAllSamples(
                  "NewTabPage.TileFaviconFetchSuccess.Popular"),
              ElementsAre(Bucket(/*bucket=*/false, /*count=*/1)));
}

TEST_F(IconCacherTestPopularSites, HandlesEmptyCallbacksNicely) {
  base::HistogramTester histogram_tester;
  EXPECT_CALL(*image_fetcher_, SetDataUseServiceName(_));
  EXPECT_CALL(*image_fetcher_, SetDesiredImageFrameSize(_));
  EXPECT_CALL(*image_fetcher_, FetchImageAndData_(_, _, _, _, _))
      .WillOnce(PassFetch(128, 128));
  IconCacherImpl cacher(&favicon_service_, nullptr, std::move(image_fetcher_));
  cacher.StartFetchPopularSites(site_, base::Closure(), base::Closure());
  WaitForHistoryThreadTasksToFinish();  // Writing the icon into the DB.
  WaitForMainThreadTasksToFinish();     // Finishing tasks after the DB write.
  // Even though the callbacks are not called, the icon gets written out.
  EXPECT_FALSE(IconIsCachedFor(site_.url, favicon_base::IconType::kFavicon));
  EXPECT_TRUE(IconIsCachedFor(site_.url, favicon_base::IconType::kTouchIcon));
  // The histogram gets reported despite empty callbacks.
  EXPECT_THAT(histogram_tester.GetAllSamples(
                  "NewTabPage.TileFaviconFetchSuccess.Popular"),
              ElementsAre(Bucket(/*bucket=*/true, /*count=*/1)));
}

TEST_F(IconCacherTestPopularSites, ProvidesDefaultIconAndSucceedsWithFetching) {
  base::HistogramTester histogram_tester;
  // The returned data string is not used by the mocked decoder.
  ON_CALL(mock_resource_delegate_, GetRawDataResource(12345, _, _))
      .WillByDefault(Return(""));
  // It's not important when the image_fetcher's decoder is used to decode the
  // image but it must happen at some point.
  EXPECT_CALL(*image_fetcher_, GetImageDecoder())
      .WillOnce(Return(image_decoder_.get()));
  EXPECT_CALL(*image_decoder_, DecodeImage(_, gfx::Size(128, 128), _))
      .WillOnce(DecodeSuccessfully(64, 64));
  base::MockCallback<base::Closure> preliminary_icon_available;
  base::MockCallback<base::Closure> icon_available;
  base::RunLoop default_loop;
  base::RunLoop fetch_loop;
  {
    InSequence s;
    EXPECT_CALL(*image_fetcher_,
                SetDataUseServiceName(
                    data_use_measurement::DataUseUserData::NTP_TILES));
    EXPECT_CALL(*image_fetcher_, SetDesiredImageFrameSize(gfx::Size(128, 128)));
    EXPECT_CALL(*image_fetcher_,
                FetchImageAndData_(_, site_.large_icon_url, _, _, _))
        .WillOnce(PassFetch(128, 128));

    // Both callback are called async after the request but preliminary has to
    // preceed icon_available.
    EXPECT_CALL(preliminary_icon_available, Run())
        .WillOnce(Quit(&default_loop));
    EXPECT_CALL(icon_available, Run()).WillOnce(Quit(&fetch_loop));
  }

  IconCacherImpl cacher(&favicon_service_, nullptr, std::move(image_fetcher_));
  site_.default_icon_resource = 12345;
  cacher.StartFetchPopularSites(site_, icon_available.Get(),
                                preliminary_icon_available.Get());

  default_loop.Run();  // Wait for the default image.
  EXPECT_THAT(
      GetCachedIconFor(site_.url, favicon_base::IconType::kTouchIcon).Size(),
      Eq(gfx::Size(64, 64)));  // Compares dimensions, not objects.

  // Let the fetcher continue and wait for the second call of the callback.
  fetch_loop.Run();  // Wait for the updated image.
  EXPECT_THAT(
      GetCachedIconFor(site_.url, favicon_base::IconType::kTouchIcon).Size(),
      Eq(gfx::Size(128, 128)));  // Compares dimensions, not objects.
  // The histogram gets reported only once (for the downloaded icon, not for the
  // default one).
  EXPECT_THAT(histogram_tester.GetAllSamples(
                  "NewTabPage.TileFaviconFetchSuccess.Popular"),
              ElementsAre(Bucket(/*bucket=*/true, /*count=*/1)));
}

TEST_F(IconCacherTestPopularSites, LargeNotCachedAndFetchPerformedOnlyOnce) {
  base::MockCallback<base::Closure> done;
  base::RunLoop loop;
  {
    InSequence s;
    // Image fetcher is used only once.
    EXPECT_CALL(*image_fetcher_,
                SetDataUseServiceName(
                    data_use_measurement::DataUseUserData::NTP_TILES));
    EXPECT_CALL(*image_fetcher_, SetDesiredImageFrameSize(gfx::Size(128, 128)));
    EXPECT_CALL(*image_fetcher_,
                FetchImageAndData_(_, site_.large_icon_url, _, _, _))
        .WillOnce(PassFetch(128, 128));
    // Success will be notified to both requests.
    EXPECT_CALL(done, Run()).WillOnce(Return()).WillOnce(Quit(&loop));
  }

  IconCacherImpl cacher(&favicon_service_, nullptr, std::move(image_fetcher_));
  cacher.StartFetchPopularSites(site_, done.Get(), done.Get());
  cacher.StartFetchPopularSites(site_, done.Get(), done.Get());
  loop.Run();
  EXPECT_FALSE(IconIsCachedFor(site_.url, favicon_base::IconType::kFavicon));
  EXPECT_TRUE(IconIsCachedFor(site_.url, favicon_base::IconType::kTouchIcon));
}

class IconCacherTestMostLikely : public IconCacherTestBase {
 protected:
  IconCacherTestMostLikely()
      : fetcher_for_large_icon_service_(
            std::make_unique<::testing::StrictMock<MockImageFetcher>>()),
        fetcher_for_icon_cacher_(
            std::make_unique<::testing::StrictMock<MockImageFetcher>>()) {
    // Expect uninteresting calls here, |fetcher_for_icon_cacher_| is not
    // related to these tests. Keep it strict to make sure we do not use it in
    // any other way.
    EXPECT_CALL(*fetcher_for_icon_cacher_,
                SetDataUseServiceName(
                    data_use_measurement::DataUseUserData::NTP_TILES));
    EXPECT_CALL(*fetcher_for_icon_cacher_,
                SetDesiredImageFrameSize(gfx::Size(128, 128)));
  }

  std::unique_ptr<MockImageFetcher> fetcher_for_large_icon_service_;
  std::unique_ptr<MockImageFetcher> fetcher_for_icon_cacher_;
};

TEST_F(IconCacherTestMostLikely, Cached) {
  GURL page_url("http://www.site.com");
  base::HistogramTester histogram_tester;

  GURL icon_url("http://www.site.com/favicon.png");
  PreloadIcon(page_url, icon_url, favicon_base::IconType::kTouchIcon, 128, 128);

  favicon::LargeIconService large_icon_service(
      &favicon_service_, std::move(fetcher_for_large_icon_service_));
  IconCacherImpl cacher(&favicon_service_, &large_icon_service,
                        std::move(fetcher_for_icon_cacher_));

  base::MockCallback<base::Closure> done;
  EXPECT_CALL(done, Run()).Times(0);
  cacher.StartFetchMostLikely(page_url, done.Get());
  scoped_task_environment_.RunUntilIdle();

  EXPECT_FALSE(IconIsCachedFor(page_url, favicon_base::IconType::kFavicon));
  EXPECT_TRUE(IconIsCachedFor(page_url, favicon_base::IconType::kTouchIcon));
  EXPECT_THAT(histogram_tester.GetAllSamples(
                  "NewTabPage.TileFaviconFetchStatus.Server"),
              IsEmpty());
}

TEST_F(IconCacherTestMostLikely, NotCachedAndFetchSucceeded) {
  GURL page_url("http://www.site.com");
  base::HistogramTester histogram_tester;

  base::MockCallback<base::Closure> done;
  base::RunLoop loop;
  {
    InSequence s;
    EXPECT_CALL(*fetcher_for_large_icon_service_,
                SetDataUseServiceName(
                    data_use_measurement::DataUseUserData::LARGE_ICON_SERVICE));
    EXPECT_CALL(*fetcher_for_large_icon_service_,
                FetchImageAndData_(_, _, _, _, _))
        .WillOnce(PassFetch(128, 128));
    EXPECT_CALL(done, Run()).WillOnce(Quit(&loop));
  }

  favicon::LargeIconService large_icon_service(
      &favicon_service_, std::move(fetcher_for_large_icon_service_));
  IconCacherImpl cacher(&favicon_service_, &large_icon_service,
                        std::move(fetcher_for_icon_cacher_));

  cacher.StartFetchMostLikely(page_url, done.Get());
  // Both these task runners need to be flushed in order to get |done| called by
  // running the main loop.
  WaitForHistoryThreadTasksToFinish();
  scoped_task_environment_.RunUntilIdle();

  loop.Run();
  EXPECT_FALSE(IconIsCachedFor(page_url, favicon_base::IconType::kFavicon));
  EXPECT_TRUE(IconIsCachedFor(page_url, favicon_base::IconType::kTouchIcon));
  EXPECT_THAT(histogram_tester.GetAllSamples(
                  "NewTabPage.TileFaviconFetchStatus.Server"),
              ElementsAre(Bucket(
                  /*bucket=*/static_cast<int>(
                      favicon_base::GoogleFaviconServerRequestStatus::SUCCESS),
                  /*count=*/1)));
}

TEST_F(IconCacherTestMostLikely, NotCachedAndFetchFailed) {
  GURL page_url("http://www.site.com");
  base::HistogramTester histogram_tester;

  base::MockCallback<base::Closure> done;
  {
    InSequence s;
    EXPECT_CALL(*fetcher_for_large_icon_service_,
                SetDataUseServiceName(
                    data_use_measurement::DataUseUserData::LARGE_ICON_SERVICE));
    EXPECT_CALL(*fetcher_for_large_icon_service_,
                FetchImageAndData_(_, _, _, _, _))
        .WillOnce(FailFetch());
    EXPECT_CALL(done, Run()).Times(0);
  }

  favicon::LargeIconService large_icon_service(
      &favicon_service_, std::move(fetcher_for_large_icon_service_));
  IconCacherImpl cacher(&favicon_service_, &large_icon_service,
                        std::move(fetcher_for_icon_cacher_));

  cacher.StartFetchMostLikely(page_url, done.Get());
  // Both these task runners need to be flushed before flushing the main thread
  // queue in order to finish the work.
  WaitForHistoryThreadTasksToFinish();
  scoped_task_environment_.RunUntilIdle();

  EXPECT_FALSE(IconIsCachedFor(page_url, favicon_base::IconType::kFavicon));
  EXPECT_FALSE(IconIsCachedFor(page_url, favicon_base::IconType::kTouchIcon));
  EXPECT_THAT(histogram_tester.GetAllSamples(
                  "NewTabPage.TileFaviconFetchStatus.Server"),
              ElementsAre(Bucket(
                  /*bucket=*/static_cast<int>(
                      favicon_base::GoogleFaviconServerRequestStatus::
                          FAILURE_CONNECTION_ERROR),
                  /*count=*/1)));
}

TEST_F(IconCacherTestMostLikely, HandlesEmptyCallbacksNicely) {
  GURL page_url("http://www.site.com");

  EXPECT_CALL(*fetcher_for_large_icon_service_, SetDataUseServiceName(_));
  EXPECT_CALL(*fetcher_for_large_icon_service_,
              FetchImageAndData_(_, _, _, _, _))
      .WillOnce(PassFetch(128, 128));

  favicon::LargeIconService large_icon_service(
      &favicon_service_, std::move(fetcher_for_large_icon_service_));
  IconCacherImpl cacher(&favicon_service_, &large_icon_service,
                        std::move(fetcher_for_icon_cacher_));

  cacher.StartFetchMostLikely(page_url, base::Closure());

  // Finish the posted tasks on the main and the history thread to find out if
  // we can write the icon.
  scoped_task_environment_.RunUntilIdle();
  WaitForHistoryThreadTasksToFinish();
  // Continue with the work in large icon service - fetch and decode the data.
  scoped_task_environment_.RunUntilIdle();
  // Do the work on the history thread to write down the icon
  WaitForHistoryThreadTasksToFinish();
  // Finish the tasks on the main thread.
  scoped_task_environment_.RunUntilIdle();

  // Even though the callbacks are not called, the icon gets written out.
  EXPECT_FALSE(IconIsCachedFor(page_url, favicon_base::IconType::kFavicon));
  EXPECT_TRUE(IconIsCachedFor(page_url, favicon_base::IconType::kTouchIcon));
}

TEST_F(IconCacherTestMostLikely, NotCachedAndFetchPerformedOnlyOnce) {
  GURL page_url("http://www.site.com");

  base::MockCallback<base::Closure> done;
  base::RunLoop loop;
  {
    InSequence s;
    // Image fetcher is used only once.
    EXPECT_CALL(*fetcher_for_large_icon_service_,
                SetDataUseServiceName(
                    data_use_measurement::DataUseUserData::LARGE_ICON_SERVICE));
    EXPECT_CALL(*fetcher_for_large_icon_service_,
                FetchImageAndData_(_, _, _, _, _))
        .WillOnce(PassFetch(128, 128));
    // Success will be notified to both requests.
    EXPECT_CALL(done, Run()).WillOnce(Return()).WillOnce(Quit(&loop));
  }

  favicon::LargeIconService large_icon_service(
      &favicon_service_, std::move(fetcher_for_large_icon_service_));
  IconCacherImpl cacher(&favicon_service_, &large_icon_service,
                        std::move(fetcher_for_icon_cacher_));

  cacher.StartFetchMostLikely(page_url, done.Get());
  cacher.StartFetchMostLikely(page_url, done.Get());

  // Finish the posted tasks on the main and the history thread to find out if
  // we can write the icon.
  scoped_task_environment_.RunUntilIdle();
  WaitForHistoryThreadTasksToFinish();
  // Continue with the work in large icon service - fetch and decode the data.
  scoped_task_environment_.RunUntilIdle();
  // Do the work on the history thread to write down the icon
  WaitForHistoryThreadTasksToFinish();
  // Finish the tasks on the main thread.
  loop.Run();

  EXPECT_FALSE(IconIsCachedFor(page_url, favicon_base::IconType::kFavicon));
  EXPECT_TRUE(IconIsCachedFor(page_url, favicon_base::IconType::kTouchIcon));
}

}  // namespace
}  // namespace ntp_tiles
