// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <algorithm>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/strings/stringprintf.h"
#include "base/test/test_file_util.h"
#include "base/timer/elapsed_timer.h"
#include "components/visitedlink/browser/visitedlink_master.h"
#include "content/public/test/test_browser_thread_bundle.h"
#include "content/public/test/test_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/perf/perf_test.h"
#include "url/gurl.h"

using base::TimeDelta;

namespace visitedlink {

namespace {

// Designed like base/test/perf_time_logger but uses testing/perf instead of
// base/test/perf* to report timings.
class TimeLogger {
 public:
  explicit TimeLogger(std::string test_name);
  ~TimeLogger();
  void Done();

 private:
  bool logged_;
  std::string test_name_;
  base::ElapsedTimer timer_;

  DISALLOW_COPY_AND_ASSIGN(TimeLogger);
};

TimeLogger::TimeLogger(std::string test_name)
    : logged_(false), test_name_(std::move(test_name)) {}

TimeLogger::~TimeLogger() {
  if (!logged_)
    Done();
}

void TimeLogger::Done() {
  // We use a floating-point millisecond value because it is more
  // intuitive than microseconds and we want more precision than
  // integer milliseconds.
  perf_test::PrintResult(test_name_, std::string(), std::string(),
                         timer_.Elapsed().InMillisecondsF(), "ms", true);
  logged_ = true;
}

// how we generate URLs, note that the two strings should be the same length
const int add_count = 10000;
const int load_test_add_count = 250000;
const char added_prefix[] = "http://www.google.com/stuff/something/foo?session=85025602345625&id=1345142319023&seq=";
const char unadded_prefix[] = "http://www.google.org/stuff/something/foo?session=39586739476365&id=2347624314402&seq=";

// Returns a URL with the given prefix and index
GURL TestURL(const char* prefix, int i) {
  return GURL(base::StringPrintf("%s%d", prefix, i));
}

// We have no slaves, so all methods on this listener are a no-ops.
class DummyVisitedLinkEventListener : public VisitedLinkMaster::Listener {
 public:
  DummyVisitedLinkEventListener() {}
  void NewTable(base::ReadOnlySharedMemoryRegion*) override {}
  void Add(VisitedLinkCommon::Fingerprint) override {}
  void Reset(bool invalidate_hashes) override {}
};


// this checks IsVisited for the URLs starting with the given prefix and
// within the given range
void CheckVisited(VisitedLinkMaster& master, const char* prefix,
                  int begin, int end) {
  for (int i = begin; i < end; i++)
    master.IsVisited(TestURL(prefix, i));
}

// Fills that master's table with URLs starting with the given prefix and
// within the given range
void FillTable(VisitedLinkMaster& master, const char* prefix,
               int begin, int end) {
  for (int i = begin; i < end; i++)
    master.AddURL(TestURL(prefix, i));
}

class VisitedLink : public testing::Test {
 protected:
  base::FilePath db_path_;
  void SetUp() override { ASSERT_TRUE(base::CreateTemporaryFile(&db_path_)); }
  void TearDown() override { base::DeleteFile(db_path_, false); }

 private:
  content::TestBrowserThreadBundle thread_bundle_;
};

} // namespace

// This test tests adding many things to a database, and how long it takes
// to query the database with different numbers of things in it. The time
// is the total time to do all the operations, and as such, it is only
// useful for a regression test. If there is a regression, it might be
// useful to make another set of tests to test these things in isolation.
TEST_F(VisitedLink, TestAddAndQuery) {
  // init
  VisitedLinkMaster master(new DummyVisitedLinkEventListener(), nullptr, true,
                           true, db_path_, 0);
  ASSERT_TRUE(master.Init());
  content::RunAllTasksUntilIdle();

  TimeLogger timer("Visited_link_add_and_query");

  // first check without anything in the table
  CheckVisited(master, added_prefix, 0, add_count);

  // now fill half the table
  const int half_size = add_count / 2;
  FillTable(master, added_prefix, 0, half_size);

  // check the table again, half of these URLs will be visited, the other half
  // will not
  CheckVisited(master, added_prefix, 0, add_count);

  // fill the rest of the table
  FillTable(master, added_prefix, half_size, add_count);

  // check URLs, doing half visited, half unvisited
  CheckVisited(master, added_prefix, 0, add_count);
  CheckVisited(master, unadded_prefix, 0, add_count);
}

// Tests how long it takes to write and read a large database to and from disk.
// Flaky, see crbug.com/822308.
TEST_F(VisitedLink, DISABLED_TestLoad) {
  // create a big DB
  {
    TimeLogger table_initialization_timer("Table_initialization");

    VisitedLinkMaster master(new DummyVisitedLinkEventListener(), nullptr, true,
                             true, db_path_, 0);

    // time init with empty table
    TimeLogger initTimer("Empty_visited_link_init");
    bool success = master.Init();
    content::RunAllTasksUntilIdle();
    initTimer.Done();
    ASSERT_TRUE(success);

    // add a bunch of stuff
    // TODO(maruel): This is very inefficient because the file gets rewritten
    // many time and this is the actual bottleneck of this test. The file should
    // only get written that the end of the FillTable call, not 4169(!) times.
    FillTable(master, added_prefix, 0, load_test_add_count);

    // time writing the file out out
    TimeLogger flushTimer("Visited_link_database_flush");
    master.RewriteFile();
    // TODO(maruel): Without calling FlushFileBuffers(master.file_); you don't
    // know really how much time it took to write the file.
    flushTimer.Done();

    table_initialization_timer.Done();
  }

  // test loading the DB back, we do this several times since the flushing is
  // not very reliable.
  const int load_count = 5;
  std::vector<double> cold_load_times;
  std::vector<double> hot_load_times;
  for (int i = 0; i < load_count; i++) {
    // make sure the file has to be re-loaded
    base::EvictFileFromSystemCache(db_path_);

    // cold load (no OS cache, hopefully)
    {
      base::ElapsedTimer cold_timer;

      VisitedLinkMaster master(new DummyVisitedLinkEventListener(), nullptr,
                               true, true, db_path_, 0);
      bool success = master.Init();
      content::RunAllTasksUntilIdle();
      TimeDelta elapsed = cold_timer.Elapsed();
      ASSERT_TRUE(success);

      cold_load_times.push_back(elapsed.InMillisecondsF());
    }

    // hot load (with OS caching the file in memory)
    {
      base::ElapsedTimer hot_timer;

      VisitedLinkMaster master(new DummyVisitedLinkEventListener(), nullptr,
                               true, true, db_path_, 0);
      bool success = master.Init();
      content::RunAllTasksUntilIdle();
      TimeDelta elapsed = hot_timer.Elapsed();
      ASSERT_TRUE(success);

      hot_load_times.push_back(elapsed.InMillisecondsF());
    }
  }

  // We discard the max and return the average time.
  cold_load_times.erase(std::max_element(cold_load_times.begin(),
                                         cold_load_times.end()));
  hot_load_times.erase(std::max_element(hot_load_times.begin(),
                                        hot_load_times.end()));

  double cold_sum = 0, hot_sum = 0;
  for (int i = 0; i < static_cast<int>(cold_load_times.size()); i++) {
    cold_sum += cold_load_times[i];
    hot_sum += hot_load_times[i];
  }

  perf_test::PrintResult("Visited_link_cold_load_time", std::string(),
                         std::string(), cold_sum / cold_load_times.size(), "ms",
                         true);
  perf_test::PrintResult("Visited_link_hot_load_time", std::string(),
                         std::string(), hot_sum / hot_load_times.size(), "ms",
                         true);
}

}  // namespace visitedlink
