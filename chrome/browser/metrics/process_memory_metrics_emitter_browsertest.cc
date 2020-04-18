// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/metrics/process_memory_metrics_emitter.h"

#include "base/memory/ref_counted.h"
#include "base/run_loop.h"
#include "base/test/histogram_tester.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/trace_event_analyzer.h"
#include "base/trace_event/memory_dump_manager.h"
#include "base/trace_event/trace_config_memory_test_util.h"
#include "build/build_config.h"
#include "chrome/browser/extensions/extension_browsertest.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "chrome/test/base/tracing.h"
#include "chrome/test/base/ui_test_utils.h"
#include "components/ukm/test_ukm_recorder.h"
#include "components/ukm/ukm_source.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/test/test_utils.h"
#include "extensions/buildflags/buildflags.h"
#include "net/dns/mock_host_resolver.h"
#include "services/metrics/public/cpp/ukm_builders.h"
#include "services/resource_coordinator/public/cpp/memory_instrumentation/memory_instrumentation.h"
#include "url/gurl.h"
#include "url/url_constants.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "extensions/browser/process_manager.h"
#include "extensions/common/extension.h"
#include "extensions/test/background_page_watcher.h"
#include "extensions/test/test_extension_dir.h"
#endif

namespace {

using base::trace_event::MemoryDumpType;
using memory_instrumentation::GlobalMemoryDump;
using memory_instrumentation::mojom::ProcessType;

#if BUILDFLAG(ENABLE_EXTENSIONS)
using extensions::BackgroundPageWatcher;
using extensions::Extension;
using extensions::ProcessManager;
using extensions::TestExtensionDir;
#endif

using UkmEntry = ukm::builders::Memory_Experimental;

void RequestGlobalDumpCallback(base::Closure quit_closure,
                               bool success,
                               uint64_t) {
  base::ThreadTaskRunnerHandle::Get()->PostTask(FROM_HERE, quit_closure);
  ASSERT_TRUE(success);
}

void OnStartTracingDoneCallback(
    base::trace_event::MemoryDumpLevelOfDetail explicit_dump_type,
    base::Closure quit_closure) {
  memory_instrumentation::MemoryInstrumentation::GetInstance()
      ->RequestGlobalDumpAndAppendToTrace(
          MemoryDumpType::EXPLICITLY_TRIGGERED, explicit_dump_type,
          Bind(&RequestGlobalDumpCallback, quit_closure));
}

class ProcessMemoryMetricsEmitterFake : public ProcessMemoryMetricsEmitter {
 public:
  explicit ProcessMemoryMetricsEmitterFake(base::RunLoop* run_loop,
                                           ukm::TestUkmRecorder* recorder)
      : run_loop_(run_loop), recorder_(recorder) {}

 private:
  ~ProcessMemoryMetricsEmitterFake() override {}

  void ReceivedMemoryDump(bool success,
                          std::unique_ptr<GlobalMemoryDump> ptr) override {
    EXPECT_TRUE(success);
    ProcessMemoryMetricsEmitter::ReceivedMemoryDump(success, std::move(ptr));
    finished_memory_dump_ = true;
    QuitIfFinished();
  }

  void ReceivedProcessInfos(
      std::vector<resource_coordinator::mojom::ProcessInfoPtr> process_infos)
      override {
    ProcessMemoryMetricsEmitter::ReceivedProcessInfos(std::move(process_infos));
    finished_process_info_ = true;
    QuitIfFinished();
  }

  void QuitIfFinished() {
    if (!finished_memory_dump_ || !finished_process_info_)
      return;
    if (run_loop_)
      run_loop_->Quit();
  }

  ukm::UkmRecorder* GetUkmRecorder() override { return recorder_; }

  base::RunLoop* run_loop_;
  bool finished_memory_dump_ = false;
  bool finished_process_info_ = false;
  ukm::TestUkmRecorder* recorder_;

  DISALLOW_COPY_AND_ASSIGN(ProcessMemoryMetricsEmitterFake);
};

void CheckMemoryMetric(const std::string& name,
                       const base::HistogramTester& histogram_tester,
                       int count,
                       bool check_minimum,
                       int number_of_processes = 1u) {
  std::unique_ptr<base::HistogramSamples> samples(
      histogram_tester.GetHistogramSamplesSinceCreation(name));
  ASSERT_TRUE(samples);

  bool count_matches = samples->TotalCount() == count * number_of_processes;
  // The exact number of renderers present at the time the metrics are emitted
  // is not deterministic. Sometimes there is an extra renderer.
  if (name.find("Renderer") != std::string::npos) {
    count_matches = samples->TotalCount() >= (count * number_of_processes) &&
                    samples->TotalCount() <= (number_of_processes + 1) * count;
  }

  EXPECT_TRUE(count_matches);

  if (check_minimum)
    EXPECT_GT(samples->sum(), 0u) << name;

  // As a sanity check, no memory stat should exceed 4 GB.
  int64_t maximum_expected_size = 1ll << 32;
  EXPECT_LT(samples->sum(), maximum_expected_size) << name;
}

void CheckAllMemoryMetrics(const base::HistogramTester& histogram_tester,
                           int count,
                           int number_of_renderer_processes = 1u,
                           int number_of_extenstion_processes = 0u) {
#if !defined(OS_WIN)
  CheckMemoryMetric("Memory.Experimental.Browser2.Malloc", histogram_tester,
                    count, true);
#endif
#if !defined(OS_MACOSX)
  CheckMemoryMetric("Memory.Experimental.Browser2.Resident", histogram_tester,
                    count, true);
#endif
  CheckMemoryMetric("Memory.Experimental.Browser2.PrivateMemoryFootprint",
                    histogram_tester, count, true);
  if (number_of_renderer_processes) {
#if !defined(OS_WIN)
    CheckMemoryMetric("Memory.Experimental.Renderer2.Malloc", histogram_tester,
                      count, true, number_of_renderer_processes);
#endif
#if !defined(OS_MACOSX)
    CheckMemoryMetric("Memory.Experimental.Renderer2.Resident",
                      histogram_tester, count, true,
                      number_of_renderer_processes);
#endif
    CheckMemoryMetric("Memory.Experimental.Renderer2.BlinkGC", histogram_tester,
                      count, false, number_of_renderer_processes);
    CheckMemoryMetric("Memory.Experimental.Renderer2.PartitionAlloc",
                      histogram_tester, count, false,
                      number_of_renderer_processes);
    CheckMemoryMetric("Memory.Experimental.Renderer2.V8", histogram_tester,
                      count, true, number_of_renderer_processes);
    CheckMemoryMetric("Memory.Experimental.Renderer2.PrivateMemoryFootprint",
                      histogram_tester, count, true,
                      number_of_renderer_processes);
  }
  if (number_of_extenstion_processes) {
#if !defined(OS_WIN)
    CheckMemoryMetric("Memory.Experimental.Extension2.Malloc", histogram_tester,
                      count, true, number_of_extenstion_processes);
#endif
#if !defined(OS_MACOSX)
    CheckMemoryMetric("Memory.Experimental.Extension2.Resident",
                      histogram_tester, count, true,
                      number_of_extenstion_processes);
#endif
    CheckMemoryMetric("Memory.Experimental.Extension2.BlinkGC",
                      histogram_tester, count, false,
                      number_of_extenstion_processes);
    CheckMemoryMetric("Memory.Experimental.Extension2.PartitionAlloc",
                      histogram_tester, count, false,
                      number_of_extenstion_processes);
    CheckMemoryMetric("Memory.Experimental.Extension2.V8", histogram_tester,
                      count, true, number_of_extenstion_processes);
    CheckMemoryMetric("Memory.Experimental.Extension2.PrivateMemoryFootprint",
                      histogram_tester, count, true,
                      number_of_extenstion_processes);
  }
  CheckMemoryMetric("Memory.Experimental.Total2.PrivateMemoryFootprint",
                    histogram_tester, count, true);
}

}  // namespace

class ProcessMemoryMetricsEmitterTest
    : public extensions::ExtensionBrowserTest {
 public:
  ProcessMemoryMetricsEmitterTest() {
    scoped_feature_list_.InitAndEnableFeature(ukm::kUkmFeature);
  }

  ~ProcessMemoryMetricsEmitterTest() override {}

  void SetUpOnMainThread() override {
    extensions::ExtensionBrowserTest::SetUpOnMainThread();
    host_resolver()->AddRule("*", "127.0.0.1");
  }

  void PreRunTestOnMainThread() override {
    InProcessBrowserTest::PreRunTestOnMainThread();

    test_ukm_recorder_ = std::make_unique<ukm::TestAutoSetUkmRecorder>();
  }

 protected:
  std::unique_ptr<ukm::TestAutoSetUkmRecorder> test_ukm_recorder_;

  void CheckMetricWithName(const ukm::mojom::UkmEntry* entry,
                           const char* name,
                           std::function<bool(int64_t)> check) {
    const int64_t* value = test_ukm_recorder_->GetEntryMetric(entry, name);
    EXPECT_TRUE(value && check(*value)) << name;
  }

  void CheckExactMetricWithName(const ukm::mojom::UkmEntry* entry,
                                const char* name,
                                int64_t expected_value) {
    CheckMetricWithName(entry, name, [expected_value](int64_t value) -> bool {
      return value == expected_value;
    });
  }

  void CheckMemoryMetricWithName(const ukm::mojom::UkmEntry* entry,
                                 const char* name,
                                 bool can_be_zero) {
    CheckMetricWithName(entry, name, [can_be_zero](int64_t value) -> bool {
      return value >= (can_be_zero ? 0 : 1) && value <= 4000;
    });
  }

  void CheckTimeMetricWithName(const ukm::mojom::UkmEntry* entry,
                               const char* name) {
    CheckMetricWithName(entry, name, [](int64_t value) -> bool {
      return value >= 0 && value <= 10;
    });
  }

  void CheckAllUkmEntries(size_t entry_count = 1u) {
    const auto& entries =
        test_ukm_recorder_->GetEntriesByName(UkmEntry::kEntryName);
    size_t browser_entry_count = 0;
    size_t renderer_entry_count = 0;
    size_t total_entry_count = 0;

    for (const auto* entry : entries) {
      if (ProcessHasTypeForEntry(entry, ProcessType::BROWSER)) {
        browser_entry_count++;
        CheckUkmBrowserEntry(entry);
      } else if (ProcessHasTypeForEntry(entry, ProcessType::RENDERER)) {
        renderer_entry_count++;
        CheckUkmRendererEntry(entry);
      } else if (ProcessHasTypeForEntry(entry, ProcessType::GPU)) {
        CheckUkmGPUEntry(entry);
      } else {
        // This must be Total2.
        total_entry_count++;
        CheckMemoryMetricWithName(
            entry, UkmEntry::kTotal2_PrivateMemoryFootprintName, false);
      }
    }
    EXPECT_EQ(entry_count, browser_entry_count);
    EXPECT_EQ(entry_count, total_entry_count);

    EXPECT_GE(renderer_entry_count, entry_count);
  }

  void CheckUkmRendererEntry(const ukm::mojom::UkmEntry* entry) {
#if !defined(OS_WIN)
    CheckMemoryMetricWithName(entry, UkmEntry::kMallocName, false);
#endif
#if !defined(OS_MACOSX)
    CheckMemoryMetricWithName(entry, UkmEntry::kResidentName, false);
#endif
    CheckMemoryMetricWithName(entry, UkmEntry::kPrivateMemoryFootprintName,
                              false);
    CheckMemoryMetricWithName(entry, UkmEntry::kBlinkGCName, true);
    CheckMemoryMetricWithName(entry, UkmEntry::kPartitionAllocName, true);
    CheckMemoryMetricWithName(entry, UkmEntry::kV8Name, true);
    CheckMemoryMetricWithName(entry, UkmEntry::kNumberOfExtensionsName, true);
    CheckTimeMetricWithName(entry, UkmEntry::kUptimeName);

    CheckMemoryMetricWithName(entry, UkmEntry::kNumberOfDocumentsName, true);
    CheckMemoryMetricWithName(entry, UkmEntry::kNumberOfFramesName, true);
    CheckMemoryMetricWithName(entry, UkmEntry::kNumberOfLayoutObjectsName,
                              true);
    CheckMemoryMetricWithName(entry, UkmEntry::kNumberOfNodesName, true);
  }

  void CheckUkmBrowserEntry(const ukm::mojom::UkmEntry* entry) {
#if !defined(OS_WIN)
    CheckMemoryMetricWithName(entry, UkmEntry::kMallocName, false);
#endif
#if !defined(OS_MACOSX)
    CheckMemoryMetricWithName(entry, UkmEntry::kResidentName, false);
#endif
    CheckMemoryMetricWithName(entry, UkmEntry::kPrivateMemoryFootprintName,
                              false);

    CheckTimeMetricWithName(entry, UkmEntry::kUptimeName);
  }

  void CheckUkmGPUEntry(const ukm::mojom::UkmEntry* entry) {
    CheckTimeMetricWithName(entry, UkmEntry::kUptimeName);
  }

  bool ProcessHasTypeForEntry(const ukm::mojom::UkmEntry* entry,
                              ProcessType process_type) {
    const int64_t* value =
        test_ukm_recorder_->GetEntryMetric(entry, UkmEntry::kProcessTypeName);
    return value && *value == static_cast<int64_t>(process_type);
  }

  void CheckPageInfoUkmMetrics(GURL url,
                               bool is_visible,
                               size_t entry_count = 1u) {
    const auto& entries =
        test_ukm_recorder_->GetEntriesByName(UkmEntry::kEntryName);
    size_t found_count = false;
    const ukm::mojom::UkmEntry* last_entry = nullptr;
    for (const auto* entry : entries) {
      const ukm::UkmSource* source =
          test_ukm_recorder_->GetSourceForSourceId(entry->source_id);
      if (!source || source->url() != url)
        continue;
      if (!test_ukm_recorder_->EntryHasMetric(entry, UkmEntry::kIsVisibleName))
        continue;
      found_count++;
      last_entry = entry;
      EXPECT_TRUE(ProcessHasTypeForEntry(entry, ProcessType::RENDERER));
      CheckTimeMetricWithName(entry, UkmEntry::kTimeSinceLastNavigationName);
      CheckTimeMetricWithName(entry,
                              UkmEntry::kTimeSinceLastVisibilityChangeName);
    }
    CheckExactMetricWithName(last_entry, UkmEntry::kIsVisibleName, is_visible);
    EXPECT_EQ(entry_count, found_count);
  }

#if BUILDFLAG(ENABLE_EXTENSIONS)
  // Create an barebones extension with a background page for the given name.
  const Extension* CreateExtension(const std::string& name) {
    auto dir = std::make_unique<TestExtensionDir>();
    dir->WriteManifestWithSingleQuotes(
        base::StringPrintf("{"
                           "'name': '%s',"
                           "'version': '1',"
                           "'manifest_version': 2,"
                           "'background': {'page': 'bg.html'}"
                           "}",
                           name.c_str()));
    dir->WriteFile(FILE_PATH_LITERAL("bg.html"), "");

    const Extension* extension = LoadExtension(dir->UnpackedPath());
    EXPECT_TRUE(extension);
    temp_dirs_.push_back(std::move(dir));
    return extension;
  }

  const Extension* CreateHostedApp(const std::string& name,
                                   const GURL& app_url) {
    std::unique_ptr<TestExtensionDir> dir(new TestExtensionDir);
    dir->WriteManifestWithSingleQuotes(base::StringPrintf(
        "{"
        "'name': '%s',"
        "'version': '1',"
        "'manifest_version': 2,"
        "'app': {'urls': ['%s'], 'launch': {'web_url': '%s'}}"
        "}",
        name.c_str(), app_url.spec().c_str(), app_url.spec().c_str()));

    const Extension* extension = LoadExtension(dir->UnpackedPath());
    EXPECT_TRUE(extension);
    temp_dirs_.push_back(std::move(dir));
    return extension;
  }

#endif

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
#if BUILDFLAG(ENABLE_EXTENSIONS)
  std::vector<std::unique_ptr<TestExtensionDir>> temp_dirs_;
#endif

  DISALLOW_COPY_AND_ASSIGN(ProcessMemoryMetricsEmitterTest);
};

#if defined(ADDRESS_SANITIZER) || defined(MEMORY_SANITIZER)
#define MAYBE_FetchAndEmitMetrics DISABLED_FetchAndEmitMetrics
#else
// TODO(michaelpg): Remove this unconditional disabling once new UKM testing
// style CLs land: crbug.com/761524.
#define MAYBE_FetchAndEmitMetrics DISABLED_FetchAndEmitMetrics
#endif
IN_PROC_BROWSER_TEST_F(ProcessMemoryMetricsEmitterTest,
                       MAYBE_FetchAndEmitMetrics) {
  ASSERT_TRUE(embedded_test_server()->Start());
  const GURL url = embedded_test_server()->GetURL("foo.com", "/empty.html");
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);

  base::HistogramTester histogram_tester;
  base::RunLoop run_loop;

  // Intentionally let emitter leave scope to check that it correctly keeps
  // itself alive.
  {
    scoped_refptr<ProcessMemoryMetricsEmitterFake> emitter(
        new ProcessMemoryMetricsEmitterFake(&run_loop,
                                            test_ukm_recorder_.get()));
    emitter->FetchAndEmitProcessMemoryMetrics();
  }

  run_loop.Run();

  CheckAllMemoryMetrics(histogram_tester, 1);
  CheckAllUkmEntries();
  CheckPageInfoUkmMetrics(url, true);
}

#if BUILDFLAG(ENABLE_EXTENSIONS)
#if defined(ADDRESS_SANITIZER) || defined(MEMORY_SANITIZER)
#define MAYBE_FetchAndEmitMetricsWithExtensions \
  DISABLED_FetchAndEmitMetricsWithExtensions
#else
// TODO(michaelpg): Remove this unconditional disabling once new UKM testing
// style CLs land: crbug.com/761524.
#define MAYBE_FetchAndEmitMetricsWithExtensions \
  DISABLED_FetchAndEmitMetricsWithExtensions
#endif
IN_PROC_BROWSER_TEST_F(ProcessMemoryMetricsEmitterTest,
                       MAYBE_FetchAndEmitMetricsWithExtensions) {
  const Extension* extension1 = CreateExtension("Extension 1");
  const Extension* extension2 = CreateExtension("Extension 2");
  ProcessManager* pm = ProcessManager::Get(profile());

  // Verify that the extensions has loaded.
  BackgroundPageWatcher(pm, extension1).WaitForOpen();
  BackgroundPageWatcher(pm, extension2).WaitForOpen();
  EXPECT_EQ(1u, pm->GetRenderFrameHostsForExtension(extension1->id()).size());
  EXPECT_EQ(1u, pm->GetRenderFrameHostsForExtension(extension2->id()).size());

  ASSERT_TRUE(embedded_test_server()->Start());
  const GURL url = embedded_test_server()->GetURL("foo.com", "/empty.html");
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);

  base::HistogramTester histogram_tester;
  base::RunLoop run_loop;

  // Intentionally let emitter leave scope to check that it correctly keeps
  // itself alive.
  {
    scoped_refptr<ProcessMemoryMetricsEmitterFake> emitter(
        new ProcessMemoryMetricsEmitterFake(&run_loop,
                                            test_ukm_recorder_.get()));
    emitter->FetchAndEmitProcessMemoryMetrics();
  }

  run_loop.Run();

  CheckAllMemoryMetrics(histogram_tester, 1, 1, 2);
  // Extension processes do not have page_info.
  CheckAllUkmEntries();
  CheckPageInfoUkmMetrics(url, true);
}

#if defined(ADDRESS_SANITIZER) || defined(MEMORY_SANITIZER)
#define MAYBE_FetchAndEmitMetricsWithHostedApps \
  DISABLED_FetchAndEmitMetricsWithHostedApps
#else
// TODO(michaelpg): Remove this unconditional disabling once new UKM testing
// style CLs land: crbug.com/761524.
#define MAYBE_FetchAndEmitMetricsWithHostedApps \
  DISABLED_FetchAndEmitMetricsWithHostedApps
#endif
IN_PROC_BROWSER_TEST_F(ProcessMemoryMetricsEmitterTest,
                       MAYBE_FetchAndEmitMetricsWithHostedApps) {
  ASSERT_TRUE(embedded_test_server()->Start());
  const GURL app_url = embedded_test_server()->GetURL("app.org", "/empty.html");
  const Extension* app = CreateHostedApp("App", GURL("http://app.org"));
  ui_test_utils::NavigateToURL(browser(), app_url);

  // Verify that the hosted app has loaded.
  ProcessManager* pm = ProcessManager::Get(profile());
  EXPECT_EQ(1u, pm->GetRenderFrameHostsForExtension(app->id()).size());

  const GURL url = embedded_test_server()->GetURL("foo.com", "/empty.html");
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);

  base::HistogramTester histogram_tester;
  base::RunLoop run_loop;

  // Intentionally let emitter leave scope to check that it correctly keeps
  // itself alive.
  {
    scoped_refptr<ProcessMemoryMetricsEmitterFake> emitter(
        new ProcessMemoryMetricsEmitterFake(&run_loop,
                                            test_ukm_recorder_.get()));
    emitter->FetchAndEmitProcessMemoryMetrics();
  }

  run_loop.Run();

  // No extensions should be observed
  CheckAllMemoryMetrics(histogram_tester, 1, 1, 0);
  CheckAllUkmEntries();
  CheckPageInfoUkmMetrics(url, true);
}

// Breaks when attempting to add tests for new UKMs: crbug.com/761524
// Re-enable with crrev.com/c/774120.
#define MAYBE_FetchAndEmitMetricsWithExtensionsAndHostReuse \
  DISABLED_FetchAndEmitMetricsWithExtensionsAndHostReuse
IN_PROC_BROWSER_TEST_F(ProcessMemoryMetricsEmitterTest,
                       MAYBE_FetchAndEmitMetricsWithExtensionsAndHostReuse) {
  // This test does not work with --site-per-process flag since this test
  // combines multiple extensions in the same process.
  if (content::AreAllSitesIsolatedForTesting())
    return;
  // Limit the number of renderer processes to force reuse.
  content::RenderProcessHost::SetMaxRendererProcessCount(1);
  const Extension* extension1 = CreateExtension("Extension 1");
  const Extension* extension2 = CreateExtension("Extension 2");
  ProcessManager* pm = ProcessManager::Get(profile());

  // Verify that the extensions has loaded.
  BackgroundPageWatcher(pm, extension1).WaitForOpen();
  BackgroundPageWatcher(pm, extension2).WaitForOpen();
  EXPECT_EQ(1u, pm->GetRenderFrameHostsForExtension(extension1->id()).size());
  EXPECT_EQ(1u, pm->GetRenderFrameHostsForExtension(extension2->id()).size());

  ASSERT_TRUE(embedded_test_server()->Start());
  const GURL url = embedded_test_server()->GetURL("foo.com", "/empty.html");
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);

  base::HistogramTester histogram_tester;
  base::RunLoop run_loop;

  // Intentionally let emitter leave scope to check that it correctly keeps
  // itself alive.
  {
    scoped_refptr<ProcessMemoryMetricsEmitterFake> emitter(
        new ProcessMemoryMetricsEmitterFake(&run_loop,
                                            test_ukm_recorder_.get()));
    emitter->FetchAndEmitProcessMemoryMetrics();
  }

  run_loop.Run();

  CheckAllMemoryMetrics(histogram_tester, 1, 1, 1);
  CheckAllUkmEntries();
  // When hosts share a process, no unique URL is identified, therefore no page
  // info.
  const auto& entries =
      test_ukm_recorder_->GetEntriesByName(UkmEntry::kEntryName);
  for (const auto* entry : entries) {
    EXPECT_EQ(nullptr,
              test_ukm_recorder_->GetSourceForSourceId(entry->source_id));
  }
}
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)

#if defined(ADDRESS_SANITIZER) || defined(MEMORY_SANITIZER)
#define MAYBE_FetchDuringTrace DISABLED_FetchDuringTrace
#else
// TODO(michaelpg): Remove this unconditional disabling once new UKM testing
// style CLs land: crbug.com/761524.
#define MAYBE_FetchDuringTrace DISABLED_FetchDuringTrace
#endif
IN_PROC_BROWSER_TEST_F(ProcessMemoryMetricsEmitterTest,
                       MAYBE_FetchDuringTrace) {
  ASSERT_TRUE(embedded_test_server()->Start());
  const GURL url = embedded_test_server()->GetURL("foo.com", "/empty.html");
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);

  base::HistogramTester histogram_tester;

  {
    base::RunLoop run_loop;

    base::trace_event::TraceConfig trace_config(
        base::trace_event::TraceConfigMemoryTestUtil::
            GetTraceConfig_EmptyTriggers());
    ASSERT_TRUE(tracing::BeginTracingWithTraceConfig(
        trace_config, Bind(&OnStartTracingDoneCallback,
                           base::trace_event::MemoryDumpLevelOfDetail::DETAILED,
                           run_loop.QuitClosure())));
    run_loop.Run();
  }

  {
    base::RunLoop run_loop;
    scoped_refptr<ProcessMemoryMetricsEmitterFake> emitter(
        new ProcessMemoryMetricsEmitterFake(&run_loop,
                                            test_ukm_recorder_.get()));
    emitter->FetchAndEmitProcessMemoryMetrics();

    run_loop.Run();
  }

  std::string json_events;
  ASSERT_TRUE(tracing::EndTracing(&json_events));

  trace_analyzer::TraceEventVector events;
  std::unique_ptr<trace_analyzer::TraceAnalyzer> analyzer(
      trace_analyzer::TraceAnalyzer::Create(json_events));
  analyzer->FindEvents(
      trace_analyzer::Query::EventPhaseIs(TRACE_EVENT_PHASE_MEMORY_DUMP),
      &events);

  ASSERT_GT(events.size(), 1u);
  ASSERT_TRUE(trace_analyzer::CountMatches(
      events, trace_analyzer::Query::EventNameIs(MemoryDumpTypeToString(
                  MemoryDumpType::EXPLICITLY_TRIGGERED))));

  CheckAllMemoryMetrics(histogram_tester, 1);
  CheckAllUkmEntries();
  CheckPageInfoUkmMetrics(url, true);
}

#if defined(ADDRESS_SANITIZER) || defined(MEMORY_SANITIZER)
#define MAYBE_FetchThreeTimes DISABLED_FetchThreeTimes
#else
// TODO(michaelpg): Remove this unconditional disabling once new UKM testing
// style CLs land: crbug.com/761524.
#define MAYBE_FetchThreeTimes DISABLED_FetchThreeTimes
#endif
IN_PROC_BROWSER_TEST_F(ProcessMemoryMetricsEmitterTest, MAYBE_FetchThreeTimes) {
  ASSERT_TRUE(embedded_test_server()->Start());
  const GURL url = embedded_test_server()->GetURL("foo.com", "/empty.html");
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), url, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);

  base::HistogramTester histogram_tester;
  base::RunLoop run_loop;

  int count = 3;
  for (int i = 0; i < count; ++i) {
    // Only the last emitter should stop the run loop.
    auto emitter = base::MakeRefCounted<ProcessMemoryMetricsEmitterFake>(
        (i == count - 1) ? &run_loop : nullptr, test_ukm_recorder_.get());
    emitter->FetchAndEmitProcessMemoryMetrics();
  }

  run_loop.Run();

  CheckAllMemoryMetrics(histogram_tester, count);
  CheckAllUkmEntries(count);
  CheckPageInfoUkmMetrics(url, true, count);
}

#if defined(ADDRESS_SANITIZER) || defined(MEMORY_SANITIZER)
#define MAYBE_ForegroundAndBackgroundPages DISABLED_ForegroundAndBackgroundPages
#else
// TODO(michaelpg): Remove this unconditional disabling once new UKM testing
// style CLs land: crbug.com/761524.
#define MAYBE_ForegroundAndBackgroundPages DISABLED_ForegroundAndBackgroundPages
#endif
IN_PROC_BROWSER_TEST_F(ProcessMemoryMetricsEmitterTest,
                       MAYBE_ForegroundAndBackgroundPages) {
  ui_test_utils::WindowedTabAddedNotificationObserver tab_observer(
      content::NotificationService::AllSources());
  ASSERT_TRUE(embedded_test_server()->Start());
  const GURL url1 = embedded_test_server()->GetURL("a.com", "/empty.html");
  const GURL url2 = embedded_test_server()->GetURL("b.com", "/empty.html");
  ui_test_utils::NavigateToURLWithDisposition(
      browser(), url1, WindowOpenDisposition::NEW_FOREGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);
  tab_observer.Wait();
  content::WebContents* tab1 = tab_observer.GetTab();

  ui_test_utils::NavigateToURLWithDisposition(
      browser(), url2, WindowOpenDisposition::NEW_BACKGROUND_TAB,
      ui_test_utils::BROWSER_TEST_WAIT_FOR_NAVIGATION);
  tab_observer.Wait();
  content::WebContents* tab2 = tab_observer.GetTab();

  base::HistogramTester histogram_tester;
  {
    base::RunLoop run_loop;
    scoped_refptr<ProcessMemoryMetricsEmitterFake> emitter(
        new ProcessMemoryMetricsEmitterFake(&run_loop,
                                            test_ukm_recorder_.get()));
    emitter->FetchAndEmitProcessMemoryMetrics();
    run_loop.Run();
  }

  CheckAllMemoryMetrics(histogram_tester, 1, 2);
  CheckAllUkmEntries();
  CheckPageInfoUkmMetrics(url1, true /* is_visible */);
  CheckPageInfoUkmMetrics(url2, false /* is_visible */);

  tab1->WasHidden();
  tab2->WasShown();
  {
    base::RunLoop run_loop;
    scoped_refptr<ProcessMemoryMetricsEmitterFake> emitter(
        new ProcessMemoryMetricsEmitterFake(&run_loop,
                                            test_ukm_recorder_.get()));
    emitter->FetchAndEmitProcessMemoryMetrics();
    run_loop.Run();
  }
  CheckAllMemoryMetrics(histogram_tester, 2, 2);
  CheckAllUkmEntries(2);
  CheckPageInfoUkmMetrics(url1, false /* is_visible */, 2);
  CheckPageInfoUkmMetrics(url2, true /* is_visible */, 2);
}
