// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/download/content/factory/download_service_factory.h"

#include "base/files/file_path.h"
#include "build/build_config.h"
#include "components/download/content/factory/navigation_monitor_factory.h"
#include "components/download/content/internal/download_driver_impl.h"
#include "components/download/internal/background_service/client_set.h"
#include "components/download/internal/background_service/config.h"
#include "components/download/internal/background_service/controller_impl.h"
#include "components/download/internal/background_service/download_service_impl.h"
#include "components/download/internal/background_service/download_store.h"
#include "components/download/internal/background_service/empty_file_monitor.h"
#include "components/download/internal/background_service/empty_task_scheduler.h"
#include "components/download/internal/background_service/file_monitor_impl.h"
#include "components/download/internal/background_service/in_memory_download_driver.h"
#include "components/download/internal/background_service/logger_impl.h"
#include "components/download/internal/background_service/model_impl.h"
#include "components/download/internal/background_service/noop_store.h"
#include "components/download/internal/background_service/proto/entry.pb.h"
#include "components/download/internal/background_service/scheduler/scheduler_impl.h"
#include "components/leveldb_proto/proto_database_impl.h"
#include "content/public/browser/storage_partition.h"

#if defined(OS_ANDROID)
#include "components/download/internal/background_service/android/battery_status_listener_android.h"
#endif

namespace download {
namespace {
const base::FilePath::CharType kEntryDBStorageDir[] =
    FILE_PATH_LITERAL("EntryDB");
const base::FilePath::CharType kFilesStorageDir[] = FILE_PATH_LITERAL("Files");
}  // namespace

// Helper function to create download service with different implementation
// details.
DownloadService* CreateDownloadServiceInternal(
    content::BrowserContext* browser_context,
    std::unique_ptr<DownloadClientMap> clients,
    std::unique_ptr<Configuration> config,
    std::unique_ptr<DownloadDriver> driver,
    std::unique_ptr<Store> store,
    std::unique_ptr<TaskScheduler> task_scheduler,
    std::unique_ptr<FileMonitor> file_monitor,
    const base::FilePath& files_storage_dir) {
  auto client_set = std::make_unique<ClientSet>(std::move(clients));
  auto model = std::make_unique<ModelImpl>(std::move(store));

#if defined(OS_ANDROID)
  auto battery_listener = std::make_unique<BatteryStatusListenerAndroid>(
      config->battery_query_interval);
#else
  auto battery_listener =
      std::make_unique<BatteryStatusListener>(config->battery_query_interval);
#endif

  auto device_status_listener = std::make_unique<DeviceStatusListener>(
      config->network_startup_delay, config->network_change_delay,
      std::move(battery_listener));
  NavigationMonitor* navigation_monitor =
      NavigationMonitorFactory::GetForBrowserContext(browser_context);
  auto scheduler = std::make_unique<SchedulerImpl>(
      task_scheduler.get(), config.get(), client_set.get());
  auto logger = std::make_unique<LoggerImpl>();
  auto controller = std::make_unique<ControllerImpl>(
      config.get(), logger.get(), std::move(client_set), std::move(driver),
      std::move(model), std::move(device_status_listener), navigation_monitor,
      std::move(scheduler), std::move(task_scheduler), std::move(file_monitor),
      files_storage_dir);
  logger->SetLogSource(controller.get());

  return new DownloadServiceImpl(std::move(config), std::move(logger),
                                 std::move(controller));
}

// Create download service for normal profile.
DownloadService* BuildDownloadService(
    content::BrowserContext* browser_context,
    std::unique_ptr<DownloadClientMap> clients,
    const base::FilePath& storage_dir,
    const scoped_refptr<base::SequencedTaskRunner>& background_task_runner,
    std::unique_ptr<TaskScheduler> task_scheduler) {
  auto config = Configuration::CreateFromFinch();

  auto driver = std::make_unique<DownloadDriverImpl>(
      content::BrowserContext::GetDownloadManager(browser_context));

  auto entry_db_storage_dir = storage_dir.Append(kEntryDBStorageDir);
  auto entry_db =
      std::make_unique<leveldb_proto::ProtoDatabaseImpl<protodb::Entry>>(
          background_task_runner);
  auto store = std::make_unique<DownloadStore>(entry_db_storage_dir,
                                               std::move(entry_db));

  auto files_storage_dir = storage_dir.Append(kFilesStorageDir);
  auto file_monitor = std::make_unique<FileMonitorImpl>(
      files_storage_dir, background_task_runner, config->file_keep_alive_time);

  return CreateDownloadServiceInternal(
      browser_context, std::move(clients), std::move(config), std::move(driver),
      std::move(store), std::move(task_scheduler), std::move(file_monitor),
      files_storage_dir);
}

// Create download service for incognito mode without any database or file IO.
DownloadService* BuildInMemoryDownloadService(
    content::BrowserContext* browser_context,
    std::unique_ptr<DownloadClientMap> clients,
    const base::FilePath& storage_dir,
    BlobTaskProxy::BlobContextGetter blob_context_getter,
    scoped_refptr<base::SingleThreadTaskRunner> io_task_runner) {
  auto config = Configuration::CreateFromFinch();
  auto* url_loader_factory =
      content::BrowserContext::GetDefaultStoragePartition(browser_context)
          ->GetURLLoaderFactoryForBrowserProcess()
          .get();
  DCHECK(url_loader_factory);
  auto download_factory = std::make_unique<InMemoryDownloadFactory>(
      url_loader_factory, blob_context_getter, io_task_runner);
  auto driver =
      std::make_unique<InMemoryDownloadDriver>(std::move(download_factory));
  auto store = std::make_unique<NoopStore>();
  auto task_scheduler = std::make_unique<EmptyTaskScheduler>();

  // TODO(xingliu): Remove |files_storage_dir| and |storage_dir| for incognito
  // mode. See https://crbug.com/810202.
  auto files_storage_dir = storage_dir.Append(kFilesStorageDir);
  auto file_monitor = std::make_unique<EmptyFileMonitor>();

  return CreateDownloadServiceInternal(
      browser_context, std::move(clients), std::move(config), std::move(driver),
      std::move(store), std::move(task_scheduler), std::move(file_monitor),
      files_storage_dir);
}

}  // namespace download
