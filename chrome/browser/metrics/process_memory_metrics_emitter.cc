// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/metrics/process_memory_metrics_emitter.h"

#include "base/compiler_specific.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/trace_event/memory_dump_request_args.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "content/public/browser/audio_service_info.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/service_manager_connection.h"
#include "content/public/common/service_names.mojom.h"
#include "extensions/buildflags/buildflags.h"
#include "services/metrics/public/cpp/ukm_builders.h"
#include "services/metrics/public/cpp/ukm_recorder.h"
#include "services/resource_coordinator/public/cpp/memory_instrumentation/memory_instrumentation.h"
#include "services/resource_coordinator/public/cpp/resource_coordinator_features.h"
#include "services/resource_coordinator/public/mojom/service_constants.mojom.h"
#include "services/service_manager/public/cpp/connector.h"
#include "url/gurl.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/process_map.h"
#include "extensions/common/extension.h"
#endif

using base::trace_event::MemoryAllocatorDump;
using memory_instrumentation::GlobalMemoryDump;
using ukm::builders::Memory_Experimental;

namespace {

const char kEffectiveSize[] = "effective_size";
const bool kLargeMetric = true;

struct Metric {
  // The root dump name that represents the required metric.
  const char* const dump_name;
  // The name of the metric to be recorded in UMA.
  const char* const uma_name;
  // Should the UMA use large memory range (1MB - 64GB) or small memory range
  // (10KB - 500MB). Only relevant if the |metric| is a size metric.
  const bool is_large_metric;
  // The type of metric that is measured, usually size in bytes or object count.
  const char* const metric;
  // The setter method for the metric in UKM recorder.
  Memory_Experimental& (Memory_Experimental::*setter)(int64_t);
};
const Metric kAllocatorDumpNamesForMetrics[] = {
    {"blink_gc", "BlinkGC", kLargeMetric, kEffectiveSize,
     &Memory_Experimental::SetBlinkGC},
    {"blink_gc/allocated_objects", "BlinkGC.AllocatedObjects", kLargeMetric,
     kEffectiveSize, &Memory_Experimental::SetBlinkGC_AllocatedObjects},
    {"blink_objects/Document", "NumberOfDocuments", !kLargeMetric,
     MemoryAllocatorDump::kNameObjectCount,
     &Memory_Experimental::SetNumberOfDocuments},
    {"blink_objects/AdSubframe", "NumberOfAdSubframes", !kLargeMetric,
     MemoryAllocatorDump::kNameObjectCount,
     &Memory_Experimental::SetNumberOfAdSubframes},
    {"blink_objects/DetachedScriptState", "NumberOfDetachedScriptStates",
     !kLargeMetric, MemoryAllocatorDump::kNameObjectCount,
     &Memory_Experimental::SetNumberOfDetachedScriptStates},
    {"blink_objects/Frame", "NumberOfFrames", !kLargeMetric,
     MemoryAllocatorDump::kNameObjectCount,
     &Memory_Experimental::SetNumberOfFrames},
    {"blink_objects/LayoutObject", "NumberOfLayoutObjects", !kLargeMetric,
     MemoryAllocatorDump::kNameObjectCount,
     &Memory_Experimental::SetNumberOfLayoutObjects},
    {"blink_objects/Node", "NumberOfNodes", !kLargeMetric,
     MemoryAllocatorDump::kNameObjectCount,
     &Memory_Experimental::SetNumberOfNodes},
    {"components/download", "DownloadService", !kLargeMetric, kEffectiveSize,
     &Memory_Experimental::SetDownloadService},
    {"discardable", "Discardable", kLargeMetric, kEffectiveSize,
     &Memory_Experimental::SetDiscardable},
    {"extensions/value_store", "Extensions.ValueStore", kLargeMetric,
     kEffectiveSize, &Memory_Experimental::SetExtensions_ValueStore},
    {"font_caches", "FontCaches", !kLargeMetric, kEffectiveSize,
     &Memory_Experimental::SetFontCaches},
    {"gpu/gl", "CommandBuffer", kLargeMetric, kEffectiveSize,
     &Memory_Experimental::SetCommandBuffer},
    {"history", "History", !kLargeMetric, kEffectiveSize,
     &Memory_Experimental::SetHistory},
    {"java_heap", "JavaHeap", kLargeMetric, kEffectiveSize,
     &Memory_Experimental::SetJavaHeap},
    {"leveldatabase", "LevelDatabase", !kLargeMetric, kEffectiveSize,
     &Memory_Experimental::SetLevelDatabase},
    {"malloc", "Malloc", kLargeMetric, kEffectiveSize,
     &Memory_Experimental::SetMalloc},
    {"mojo", "NumberOfMojoHandles", !kLargeMetric,
     MemoryAllocatorDump::kNameObjectCount,
     &Memory_Experimental::SetNumberOfMojoHandles},
    {"net", "Net", !kLargeMetric, kEffectiveSize, &Memory_Experimental::SetNet},
    {"net/url_request_context", "Net.UrlRequestContext", !kLargeMetric,
     kEffectiveSize, &Memory_Experimental::SetNet_UrlRequestContext},
    {"omnibox", "OmniboxSuggestions", !kLargeMetric, kEffectiveSize,
     &Memory_Experimental::SetOmniboxSuggestions},
    {"partition_alloc", "PartitionAlloc", kLargeMetric, kEffectiveSize,
     &Memory_Experimental::SetPartitionAlloc},
    {"partition_alloc/allocated_objects", "PartitionAlloc.AllocatedObjects",
     kLargeMetric, kEffectiveSize,
     &Memory_Experimental::SetPartitionAlloc_AllocatedObjects},
    {"partition_alloc/partitions/array_buffer",
     "PartitionAlloc.Partitions.ArrayBuffer", kLargeMetric, kEffectiveSize,
     &Memory_Experimental::SetPartitionAlloc_Partitions_ArrayBuffer},
    {"partition_alloc/partitions/buffer", "PartitionAlloc.Partitions.Buffer",
     kLargeMetric, kEffectiveSize,
     &Memory_Experimental::SetPartitionAlloc_Partitions_Buffer},
    {"partition_alloc/partitions/fast_malloc",
     "PartitionAlloc.Partitions.FastMalloc", kLargeMetric, kEffectiveSize,
     &Memory_Experimental::SetPartitionAlloc_Partitions_FastMalloc},
    {"partition_alloc/partitions/layout", "PartitionAlloc.Partitions.Layout",
     kLargeMetric, kEffectiveSize,
     &Memory_Experimental::SetPartitionAlloc_Partitions_Layout},
    {"site_storage", "SiteStorage", kLargeMetric, kEffectiveSize,
     &Memory_Experimental::SetSiteStorage},
    {"site_storage/blob_storage", "SiteStorage.BlobStorage", kLargeMetric,
     kEffectiveSize, &Memory_Experimental::SetSiteStorage_BlobStorage},
    {"site_storage/index_db", "SiteStorage.IndexDB", !kLargeMetric,
     kEffectiveSize, &Memory_Experimental::SetSiteStorage_IndexDB},
    {"site_storage/localstorage", "SiteStorage.LocalStorage", !kLargeMetric,
     kEffectiveSize, &Memory_Experimental::SetSiteStorage_LocalStorage},
    {"site_storage/session_storage", "SiteStorage.SessionStorage",
     !kLargeMetric, kEffectiveSize,
     &Memory_Experimental::SetSiteStorage_SessionStorage},
    {"skia", "Skia", kLargeMetric, kEffectiveSize,
     &Memory_Experimental::SetSkia},
    {"skia/sk_glyph_cache", "Skia.SkGlyphCache", kLargeMetric, kEffectiveSize,
     &Memory_Experimental::SetSkia_SkGlyphCache},
    {"skia/sk_resource_cache", "Skia.SkResourceCache", kLargeMetric,
     kEffectiveSize, &Memory_Experimental::SetSkia_SkResourceCache},
    {"sqlite", "Sqlite", !kLargeMetric, kEffectiveSize,
     &Memory_Experimental::SetSqlite},
    {"sync", "Sync", kLargeMetric, kEffectiveSize,
     &Memory_Experimental::SetSync},
    {"tab_restore", "TabRestore", !kLargeMetric, kEffectiveSize,
     &Memory_Experimental::SetTabRestore},
    {"ui", "UI", !kLargeMetric, kEffectiveSize, &Memory_Experimental::SetUI},
    {"v8", "V8", kLargeMetric, kEffectiveSize, &Memory_Experimental::SetV8},
    {"web_cache", "WebCache", !kLargeMetric, kEffectiveSize,
     &Memory_Experimental::SetWebCache},
    {"web_cache/Image_resources", "WebCache.ImageResources", !kLargeMetric,
     kEffectiveSize, &Memory_Experimental::SetWebCache_ImageResources},
    {"web_cache/CSS stylesheet_resources", "WebCache.CSSStylesheetResources",
     !kLargeMetric, kEffectiveSize,
     &Memory_Experimental::SetWebCache_CSSStylesheetResources},
    {"web_cache/Script_resources", "WebCache.ScriptResources", !kLargeMetric,
     kEffectiveSize, &Memory_Experimental::SetWebCache_ScriptResources},
    {"web_cache/XSL stylesheet_resources", "WebCache.XSLStylesheetResources",
     !kLargeMetric, kEffectiveSize,
     &Memory_Experimental::SetWebCache_XSLStylesheetResources},
    {"web_cache/Font_resources", "WebCache.FontResources", !kLargeMetric,
     kEffectiveSize, &Memory_Experimental::SetWebCache_FontResources},
    {"web_cache/Other_resources", "WebCache.OtherResources", !kLargeMetric,
     kEffectiveSize, &Memory_Experimental::SetWebCache_OtherResources},
};

#define UMA_PREFIX "Memory."
#define EXPERIMENTAL_UMA_PREFIX "Memory.Experimental."
#define VERSION_SUFFIX_NORMAL "2."
#define VERSION_SUFFIX_SMALL "2.Small."

// Use the values from UMA_HISTOGRAM_MEMORY_LARGE_MB.
#define MEMORY_METRICS_HISTOGRAM_MB(name, value) \
  base::UmaHistogramCustomCounts(name, value, 1, 64000, 100)

// Used to measure KB-granularity memory stats. Range is from 1KB to 500,000KB
// (500MB).
#define MEMORY_METRICS_HISTOGRAM_KB(name, value) \
  base::UmaHistogramCustomCounts(name, value, 10, 500000, 100)

void EmitProcessUkm(const GlobalMemoryDump::ProcessDump& pmd,
                    const char* process_name,
                    const base::Optional<base::TimeDelta>& uptime,
                    bool record_uma,
                    Memory_Experimental* builder) {
  for (const auto& item : kAllocatorDumpNamesForMetrics) {
    base::Optional<uint64_t> value = pmd.GetMetric(item.dump_name, item.metric);
    if (value) {
      // Effective size is the size of the memory dump after discounting all
      // suballocations from the dump.
      if (base::StringPiece(item.metric) == kEffectiveSize) {
        // For each effective size metric, emit both an UMA in MB or KB, and an
        // UKM in MB.
        ((*builder).*(item.setter))(value.value() / 1024 / 1024);
        if (!record_uma)
          continue;

        std::string uma_name;

        // Always use "Gpu" in process name for command buffers to be
        // consistent even in single process mode.
        if (base::StringPiece(item.uma_name) == "CommandBuffer") {
          uma_name = EXPERIMENTAL_UMA_PREFIX "Gpu" VERSION_SUFFIX_NORMAL
                                             "CommandBuffer";
          DCHECK(item.is_large_metric);
        } else {
          const char* version_suffix = item.is_large_metric
                                           ? VERSION_SUFFIX_NORMAL
                                           : VERSION_SUFFIX_SMALL;
          uma_name = std::string(EXPERIMENTAL_UMA_PREFIX) + process_name +
                     version_suffix + item.uma_name;
        }

        if (item.is_large_metric) {
          MEMORY_METRICS_HISTOGRAM_MB(uma_name, value.value() / 1024 / 1024);
        } else {
          MEMORY_METRICS_HISTOGRAM_KB(uma_name, value.value() / 1024);
        }
      } else {
        // For all non-size metrics emit only an UKM, with the metric value as
        // is.
        ((*builder).*(item.setter))(value.value());
      }
    }
  }
  builder->SetResident(pmd.os_dump().resident_set_kb / 1024);
  builder->SetPrivateMemoryFootprint(pmd.os_dump().private_footprint_kb / 1024);
  builder->SetSharedMemoryFootprint(pmd.os_dump().shared_footprint_kb / 1024);
#if defined(OS_LINUX) || defined(OS_ANDROID)
  builder->SetPrivateSwapFootprint(pmd.os_dump().private_footprint_swap_kb /
                                   1024);
#endif
  if (uptime)
    builder->SetUptime(uptime.value().InSeconds());
  if (!record_uma)
    return;

  MEMORY_METRICS_HISTOGRAM_MB(
      std::string(UMA_PREFIX) + process_name + ".PrivateMemoryFootprint",
      pmd.os_dump().private_footprint_kb / 1024);
  MEMORY_METRICS_HISTOGRAM_MB(
      std::string(UMA_PREFIX) + process_name + ".SharedMemoryFootprint",
      pmd.os_dump().shared_footprint_kb / 1024);
#if defined(OS_LINUX) || defined(OS_ANDROID)
  MEMORY_METRICS_HISTOGRAM_MB(
      std::string(UMA_PREFIX) + process_name + ".PrivateSwapFootprint",
      pmd.os_dump().private_footprint_swap_kb / 1024);
#endif
}

void EmitBrowserMemoryMetrics(const GlobalMemoryDump::ProcessDump& pmd,
                              ukm::SourceId ukm_source_id,
                              ukm::UkmRecorder* ukm_recorder,
                              const base::Optional<base::TimeDelta>& uptime,
                              bool record_uma) {
  Memory_Experimental builder(ukm_source_id);
  builder.SetProcessType(static_cast<int64_t>(
      memory_instrumentation::mojom::ProcessType::BROWSER));
  EmitProcessUkm(pmd, "Browser", uptime, record_uma, &builder);

  builder.Record(ukm_recorder);
}

void EmitRendererMemoryMetrics(
    const GlobalMemoryDump::ProcessDump& pmd,
    const resource_coordinator::mojom::PageInfoPtr& page_info,
    ukm::UkmRecorder* ukm_recorder,
    int number_of_extensions,
    const base::Optional<base::TimeDelta>& uptime,
    bool record_uma) {
  ukm::SourceId ukm_source_id = page_info.is_null()
                                    ? ukm::UkmRecorder::GetNewSourceID()
                                    : page_info->ukm_source_id;
  Memory_Experimental builder(ukm_source_id);
  builder.SetProcessType(static_cast<int64_t>(
      memory_instrumentation::mojom::ProcessType::RENDERER));
  builder.SetNumberOfExtensions(number_of_extensions);

  const char* process = number_of_extensions == 0 ? "Renderer" : "Extension";
  EmitProcessUkm(pmd, process, uptime, record_uma, &builder);

  if (!page_info.is_null()) {
    builder.SetIsVisible(page_info->is_visible);
    builder.SetTimeSinceLastVisibilityChange(
        page_info->time_since_last_visibility_change.InSeconds());
    builder.SetTimeSinceLastNavigation(
        page_info->time_since_last_navigation.InSeconds());
  }

  builder.Record(ukm_recorder);
}

void EmitGpuMemoryMetrics(const GlobalMemoryDump::ProcessDump& pmd,
                          ukm::SourceId ukm_source_id,
                          ukm::UkmRecorder* ukm_recorder,
                          const base::Optional<base::TimeDelta>& uptime,
                          bool record_uma) {
  Memory_Experimental builder(ukm_source_id);
  builder.SetProcessType(
      static_cast<int64_t>(memory_instrumentation::mojom::ProcessType::GPU));
  EmitProcessUkm(pmd, "Gpu", uptime, record_uma, &builder);

  builder.Record(ukm_recorder);
}

void EmitAudioServiceMemoryMetrics(
    const GlobalMemoryDump::ProcessDump& pmd,
    ukm::SourceId ukm_source_id,
    ukm::UkmRecorder* ukm_recorder,
    const base::Optional<base::TimeDelta>& uptime,
    bool record_uma) {
  Memory_Experimental builder(ukm_source_id);
  builder.SetProcessType(static_cast<int64_t>(
      memory_instrumentation::mojom::ProcessType::UTILITY));
  EmitProcessUkm(pmd, "AudioService", uptime, record_uma, &builder);

  builder.Record(ukm_recorder);
}

}  // namespace

ProcessMemoryMetricsEmitter::ProcessMemoryMetricsEmitter()
    : pid_scope_(base::kNullProcessId) {}

ProcessMemoryMetricsEmitter::ProcessMemoryMetricsEmitter(
    base::ProcessId pid_scope)
    : pid_scope_(pid_scope) {}

void ProcessMemoryMetricsEmitter::FetchAndEmitProcessMemoryMetrics() {
  MarkServiceRequestsInProgress();

  // The callback keeps this object alive until the callback is invoked.
  auto callback =
      base::Bind(&ProcessMemoryMetricsEmitter::ReceivedMemoryDump, this);
  std::vector<std::string> mad_list;
  for (const auto& metric : kAllocatorDumpNamesForMetrics)
    mad_list.push_back(metric.dump_name);
  if (pid_scope_ != base::kNullProcessId) {
    memory_instrumentation::MemoryInstrumentation::GetInstance()
        ->RequestGlobalDumpForPid(pid_scope_, mad_list, callback);
  } else {
    memory_instrumentation::MemoryInstrumentation::GetInstance()
        ->RequestGlobalDump(mad_list, callback);
  }

  // The callback keeps this object alive until the callback is invoked.
  if (IsResourceCoordinatorEnabled()) {
    service_manager::Connector* connector =
        content::ServiceManagerConnection::GetForProcess()->GetConnector();
    connector->BindInterface(resource_coordinator::mojom::kServiceName,
                             mojo::MakeRequest(&introspector_));
    auto callback2 =
        base::Bind(&ProcessMemoryMetricsEmitter::ReceivedProcessInfos, this);
    introspector_->GetProcessToURLMap(callback2);
  }
}

void ProcessMemoryMetricsEmitter::MarkServiceRequestsInProgress() {
  memory_dump_in_progress_ = true;
  if (IsResourceCoordinatorEnabled())
    get_process_urls_in_progress_ = true;
}

ProcessMemoryMetricsEmitter::~ProcessMemoryMetricsEmitter() {}

void ProcessMemoryMetricsEmitter::ReceivedMemoryDump(
    bool success,
    std::unique_ptr<GlobalMemoryDump> dump) {
  memory_dump_in_progress_ = false;
  if (!success)
    return;
  global_dump_ = std::move(dump);
  CollateResults();
}

void ProcessMemoryMetricsEmitter::ReceivedProcessInfos(
    std::vector<resource_coordinator::mojom::ProcessInfoPtr> process_infos) {
  get_process_urls_in_progress_ = false;
  process_infos_.clear();
  process_infos_.reserve(process_infos.size());

  // If there are duplicate pids, keep the latest ProcessInfoPtr.
  for (resource_coordinator::mojom::ProcessInfoPtr& process_info :
       process_infos) {
    base::ProcessId pid = process_info->pid;
    process_infos_[pid] = std::move(process_info);
  }
  CollateResults();
}

bool ProcessMemoryMetricsEmitter::IsResourceCoordinatorEnabled() {
  return resource_coordinator::IsResourceCoordinatorEnabled();
}

ukm::UkmRecorder* ProcessMemoryMetricsEmitter::GetUkmRecorder() {
  return ukm::UkmRecorder::Get();
}

int ProcessMemoryMetricsEmitter::GetNumberOfExtensions(base::ProcessId pid) {
  int number_of_extensions = 0;
#if BUILDFLAG(ENABLE_EXTENSIONS)
  // Retrieve the renderer process host for the given pid.
  int rph_id = -1;
  bool found = false;
  for (auto iter = content::RenderProcessHost::AllHostsIterator();
       !iter.IsAtEnd(); iter.Advance()) {
    if (!iter.GetCurrentValue()->GetProcess().IsValid())
      continue;

    if (iter.GetCurrentValue()->GetProcess().Pid() == pid) {
      found = true;
      rph_id = iter.GetCurrentValue()->GetID();
      break;
    }
  }
  if (!found)
    return 0;

  // Count the number of extensions associated with that renderer process host
  // in all profiles.
  for (Profile* profile :
       g_browser_process->profile_manager()->GetLoadedProfiles()) {
    extensions::ProcessMap* process_map = extensions::ProcessMap::Get(profile);
    extensions::ExtensionRegistry* registry =
        extensions::ExtensionRegistry::Get(profile);
    std::set<std::string> extension_ids =
        process_map->GetExtensionsInProcess(rph_id);
    for (const std::string& extension_id : extension_ids) {
      // Only count non hosted apps extensions.
      const extensions::Extension* extension =
          registry->enabled_extensions().GetByID(extension_id);
      if (extension && !extension->is_hosted_app())
        number_of_extensions++;
    }
  }
#endif
  return number_of_extensions;
}

base::Optional<base::TimeDelta> ProcessMemoryMetricsEmitter::GetProcessUptime(
    const base::Time& now,
    base::ProcessId pid) {
  auto process_info = process_infos_.find(pid);
  if (process_info != process_infos_.end()) {
    if (process_info->second->launch_time)
      return now - process_info->second->launch_time.value();
  }
  return base::Optional<base::TimeDelta>();
}

void ProcessMemoryMetricsEmitter::CollateResults() {
  if (memory_dump_in_progress_ || get_process_urls_in_progress_)
    return;
  if (!global_dump_)
    return;

  uint32_t private_footprint_total_kb = 0;
  uint32_t shared_footprint_total_kb = 0;
  bool record_uma = pid_scope_ == base::kNullProcessId;

  base::Time now = base::Time::Now();
  for (const auto& pmd : global_dump_->process_dumps()) {
    private_footprint_total_kb += pmd.os_dump().private_footprint_kb;
    shared_footprint_total_kb += pmd.os_dump().shared_footprint_kb;

    if (!record_uma && pid_scope_ != pmd.pid())
      continue;

    switch (pmd.process_type()) {
      case memory_instrumentation::mojom::ProcessType::BROWSER: {
        EmitBrowserMemoryMetrics(pmd, ukm::UkmRecorder::GetNewSourceID(),
                                 GetUkmRecorder(),
                                 GetProcessUptime(now, pmd.pid()), record_uma);
        break;
      }
      case memory_instrumentation::mojom::ProcessType::RENDERER: {
        resource_coordinator::mojom::PageInfoPtr page_info;
        // If there is more than one frame being hosted in a renderer, don't
        // emit any URLs. This is not ideal, but UKM does not support
        // multiple-URLs per entry, and we must have one entry per process.
        if (process_infos_.find(pmd.pid()) != process_infos_.end()) {
          const resource_coordinator::mojom::ProcessInfoPtr& process_info =
              process_infos_[pmd.pid()];
          if (process_info->page_infos.size() == 1) {
            page_info = std::move(process_info->page_infos[0]);
          }
        }

        int number_of_extensions = GetNumberOfExtensions(pmd.pid());
        EmitRendererMemoryMetrics(pmd, page_info, GetUkmRecorder(),
                                  number_of_extensions,
                                  GetProcessUptime(now, pmd.pid()), record_uma);
        break;
      }
      case memory_instrumentation::mojom::ProcessType::GPU: {
        EmitGpuMemoryMetrics(pmd, ukm::UkmRecorder::GetNewSourceID(),
                             GetUkmRecorder(), GetProcessUptime(now, pmd.pid()),
                             record_uma);
        break;
      }
      case memory_instrumentation::mojom::ProcessType::UTILITY: {
        if (pmd.pid() == content::GetProcessIdForAudioService())
          EmitAudioServiceMemoryMetrics(
              pmd, ukm::UkmRecorder::GetNewSourceID(), GetUkmRecorder(),
              GetProcessUptime(now, pmd.pid()), record_uma);
        break;
      }
      case memory_instrumentation::mojom::ProcessType::PLUGIN:
        FALLTHROUGH;
      case memory_instrumentation::mojom::ProcessType::OTHER:
        break;
    }
  }
  if (record_uma) {
    UMA_HISTOGRAM_MEMORY_LARGE_MB(
        "Memory.Experimental.Total2.PrivateMemoryFootprint",
        private_footprint_total_kb / 1024);
    UMA_HISTOGRAM_MEMORY_LARGE_MB("Memory.Total.PrivateMemoryFootprint",
                                  private_footprint_total_kb / 1024);
    UMA_HISTOGRAM_MEMORY_LARGE_MB("Memory.Total.SharedMemoryFootprint",
                                  shared_footprint_total_kb / 1024);
  }

  Memory_Experimental(ukm::UkmRecorder::GetNewSourceID())
      .SetTotal2_PrivateMemoryFootprint(private_footprint_total_kb / 1024)
      .SetTotal2_SharedMemoryFootprint(shared_footprint_total_kb / 1024)
      .Record(GetUkmRecorder());
}
