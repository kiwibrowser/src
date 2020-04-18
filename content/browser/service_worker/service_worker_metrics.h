// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_METRICS_H_
#define CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_METRICS_H_

#include <stddef.h>
#include <map>
#include <set>

#include "base/macros.h"
#include "base/time/time.h"
#include "content/browser/service_worker/service_worker_context_request_handler.h"
#include "content/browser/service_worker/service_worker_database.h"
#include "content/browser/service_worker/service_worker_installed_script_reader.h"
#include "content/common/service_worker/embedded_worker.mojom.h"
#include "content/common/service_worker/service_worker_types.h"
#include "content/public/browser/service_worker_context.h"
#include "services/network/public/mojom/fetch_api.mojom.h"
#include "ui/base/page_transition_types.h"

class GURL;

namespace content {

enum class EmbeddedWorkerStatus;

class ServiceWorkerMetrics {
 public:
  // Used for UMA. Append-only.
  enum ReadResponseResult {
    READ_OK,
    READ_HEADERS_ERROR,
    READ_DATA_ERROR,
    NUM_READ_RESPONSE_RESULT_TYPES,
  };

  // Used for UMA. Append-only.
  enum WriteResponseResult {
    WRITE_OK,
    WRITE_HEADERS_ERROR,
    WRITE_DATA_ERROR,
    NUM_WRITE_RESPONSE_RESULT_TYPES,
  };

  // Used for UMA. Append-only.
  enum DeleteAndStartOverResult {
    DELETE_OK,
    DELETE_DATABASE_ERROR,
    DELETE_DISK_CACHE_ERROR,
    NUM_DELETE_AND_START_OVER_RESULT_TYPES,
  };

  // Used for UMA. Append-only.
  enum URLRequestJobResult {
    REQUEST_JOB_FALLBACK_RESPONSE,
    REQUEST_JOB_FALLBACK_FOR_CORS,
    REQUEST_JOB_HEADERS_ONLY_RESPONSE,
    REQUEST_JOB_STREAM_RESPONSE,
    REQUEST_JOB_BLOB_RESPONSE,
    REQUEST_JOB_ERROR_RESPONSE_STATUS_ZERO,
    REQUEST_JOB_ERROR_BAD_BLOB,
    REQUEST_JOB_ERROR_NO_PROVIDER_HOST,
    REQUEST_JOB_ERROR_NO_ACTIVE_VERSION,
    REQUEST_JOB_ERROR_NO_REQUEST,
    REQUEST_JOB_ERROR_FETCH_EVENT_DISPATCH,
    REQUEST_JOB_ERROR_BLOB_READ,
    REQUEST_JOB_ERROR_STREAM_ABORTED,
    REQUEST_JOB_ERROR_KILLED,
    REQUEST_JOB_ERROR_KILLED_WITH_BLOB,
    REQUEST_JOB_ERROR_KILLED_WITH_STREAM,
    REQUEST_JOB_ERROR_DESTROYED,
    REQUEST_JOB_ERROR_DESTROYED_WITH_BLOB,
    REQUEST_JOB_ERROR_DESTROYED_WITH_STREAM,
    REQUEST_JOB_ERROR_BAD_DELEGATE,
    REQUEST_JOB_ERROR_REQUEST_BODY_BLOB_FAILED,
    NUM_REQUEST_JOB_RESULT_TYPES,
  };

  // Used for UMA. Append-only.
  enum class StopStatus {
    NORMAL,
    DETACH_BY_REGISTRY,
    TIMEOUT,
    // Add new types here.
    NUM_TYPES
  };

  // Used for UMA. Append-only.
  // This class is used to indicate which event is fired/finished. Most events
  // have only one request that starts the event and one response that finishes
  // the event, but the fetch and the foreign fetch event have two responses, so
  // there are two types of EventType to break down the measurement into two:
  // FETCH/FOREIGN_FETCH and FETCH_WAITUNTIL/FOREIGN_FETCH_WAITUNTIL.
  // Moreover, FETCH is separated into the four: MAIN_FRAME, SUB_FRAME,
  // SHARED_WORKER and SUB_RESOURCE for more detailed UMA.
  enum class EventType {
    ACTIVATE = 0,
    INSTALL = 1,
    // FETCH = 2,  // Obsolete
    SYNC = 3,
    NOTIFICATION_CLICK = 4,
    PUSH = 5,
    // GEOFENCING = 6,  // Obsolete
    // SERVICE_PORT_CONNECT = 7,  // Obsolete
    MESSAGE = 8,
    NOTIFICATION_CLOSE = 9,
    FETCH_MAIN_FRAME = 10,
    FETCH_SUB_FRAME = 11,
    FETCH_SHARED_WORKER = 12,
    FETCH_SUB_RESOURCE = 13,
    UNKNOWN = 14,  // Used when event type is not known.
    FOREIGN_FETCH = 15,
    FETCH_WAITUNTIL = 16,
    FOREIGN_FETCH_WAITUNTIL = 17,
    // NAVIGATION_HINT_LINK_MOUSE_DOWN = 18,  // Obsolete
    // NAVIGATION_HINT_LINK_TAP_UNCONFIRMED = 19,  // Obsolete
    // NAVIGATION_HINT_LINK_TAP_DOWN = 20,  // Obsolete
    // Used when external consumers want to add a request to
    // ServiceWorkerVersion to keep it alive.
    EXTERNAL_REQUEST = 21,
    PAYMENT_REQUEST = 22,
    BACKGROUND_FETCH_ABORT = 23,
    BACKGROUND_FETCH_CLICK = 24,
    BACKGROUND_FETCH_FAIL = 25,
    BACKGROUND_FETCHED = 26,
    NAVIGATION_HINT = 27,
    CAN_MAKE_PAYMENT = 28,
    ABORT_PAYMENT = 29,
    COOKIE_CHANGE = 30,
    // Add new events to record here.
    NUM_TYPES
  };

  // Used for UMA. Append only.
  enum class Site {
    OTHER,  // Obsolete for UMA. Use WITH_FETCH_HANDLER or
            // WITHOUT_FETCH_HANDLER.
    NEW_TAB_PAGE,
    WITH_FETCH_HANDLER,
    WITHOUT_FETCH_HANDLER,
    PLUS,
    INBOX,
    DOCS,
    NUM_TYPES
  };

  // Not used for UMA.
  enum class StartSituation {
    // Failed to allocate a process.
    UNKNOWN,
    // The service worker started up during browser startup.
    DURING_STARTUP,
    // The service worker started up in a new process.
    NEW_PROCESS,
    // The service worker started up in an existing unready process. (Ex: The
    // process was created for the navigation by PlzNavigate but the IPC
    // connection is not established yet.)
    EXISTING_UNREADY_PROCESS,
    // The service worker started up in an existing ready process.
    EXISTING_READY_PROCESS
  };

  // Used for UMA. Append only.
  // This enum describes how an activated worker was found and prepared (i.e.,
  // reached the RUNNING status) in order to dispatch a fetch event to.
  enum class WorkerPreparationType {
    UNKNOWN = 0,
    // The worker was already starting up. We waited for it to finish.
    STARTING = 1,
    // The worker was already running.
    RUNNING = 2,
    // The worker was stopping. We waited for it to stop, and then started it
    // up.
    STOPPING = 3,
    // The worker was in the stopped state. We started it up, and startup
    // required a new process to be created.
    START_IN_NEW_PROCESS = 4,
    // Deprecated 07/2017; replaced by START_IN_EXISTING_UNREADY_PROCESS and
    // START_IN_EXISTING_READY_PROCESS.
    //   START_IN_EXISTING_PROCESS = 5,
    // The worker was in the stopped state. We started it up, and this occurred
    // during browser startup.
    START_DURING_STARTUP = 6,
    // The worker was in the stopped state. We started it up, and it used an
    // existing unready process.
    START_IN_EXISTING_UNREADY_PROCESS = 7,
    // The worker was in the stopped state. We started it up, and it used an
    // existing ready process.
    START_IN_EXISTING_READY_PROCESS = 8,
    // Add new types here.
    NUM_TYPES
  };

  // Used for UMA. Append only.
  // Describes the outcome of a time measurement taken between processes.
  enum class CrossProcessTimeDelta {
    NORMAL,
    NEGATIVE,
    INACCURATE_CLOCK,
    // Add new types here.
    NUM_TYPES
  };

  // Not used for UMA.
  enum class LoadSource { NETWORK, HTTP_CACHE, SERVICE_WORKER_STORAGE };

  class ScopedEventRecorder {
   public:
    explicit ScopedEventRecorder(EventType start_worker_purpose);
    ~ScopedEventRecorder();

    void RecordEventHandledStatus(EventType event, bool handled);

   private:
    struct EventStat {
      size_t fired_events = 0;
      size_t handled_events = 0;
    };

    // Records how much of dispatched events are handled.
    static void RecordEventHandledRatio(EventType event,
                                        size_t handled_events,
                                        size_t fired_events);

    std::map<EventType, EventStat> event_stats_;
    const EventType start_worker_purpose_;

    DISALLOW_COPY_AND_ASSIGN(ScopedEventRecorder);
  };

  // Converts an event type to a string. Used for tracing.
  static const char* EventTypeToString(EventType event_type);

  // Converts a start situation to a string. Used for tracing.
  static const char* StartSituationToString(StartSituation start_situation);

  // If the |url| is not a special site, returns Site::OTHER.
  static Site SiteFromURL(const GURL& url);

  // Excludes NTP scope from UMA for now as it tends to dominate the stats and
  // makes the results largely skewed. Some metrics don't follow this policy
  // and hence don't call this function.
  static bool ShouldExcludeSiteFromHistogram(Site site);

  // Used for ServiceWorkerDiskCache.
  static void CountInitDiskCacheResult(bool result);
  static void CountReadResponseResult(ReadResponseResult result);
  static void CountWriteResponseResult(WriteResponseResult result);

  // Used for ServiceWorkerDatabase.
  static void CountOpenDatabaseResult(ServiceWorkerDatabase::Status status);
  static void CountReadDatabaseResult(ServiceWorkerDatabase::Status status);
  static void CountWriteDatabaseResult(ServiceWorkerDatabase::Status status);
  static void RecordDestroyDatabaseResult(ServiceWorkerDatabase::Status status);

  // Used for ServiceWorkerStorage.
  static void RecordPurgeResourceResult(int net_error);
  static void RecordDeleteAndStartOverResult(DeleteAndStartOverResult result);

  // Counts the number of page loads controlled by a Service Worker.
  static void CountControlledPageLoad(Site site,
                                      const GURL& url,
                                      bool is_main_frame_load);

  // Records the result of trying to start a worker. |is_installed| indicates
  // whether the version has been installed.
  static void RecordStartWorkerStatus(ServiceWorkerStatusCode status,
                                      EventType purpose,
                                      bool is_installed);

  // Records the result of sending installed scripts to the renderer.
  static void RecordInstalledScriptsSenderStatus(
      ServiceWorkerInstalledScriptReader::FinishedReason reason);

  // Records the time taken to successfully start a worker. |is_installed|
  // indicates whether the version has been installed.
  static void RecordStartWorkerTime(base::TimeDelta time,
                                    bool is_installed,
                                    StartSituation start_situation,
                                    EventType purpose);

  // Records metrics for the preparation of an activated Service Worker for a
  // main frame navigation.
  CONTENT_EXPORT static void RecordActivatedWorkerPreparationForMainFrame(
      base::TimeDelta time,
      EmbeddedWorkerStatus initial_worker_status,
      StartSituation start_situation,
      bool did_navigation_preload,
      const GURL& url);

  // Records the result of trying to stop a worker.
  static void RecordWorkerStopped(StopStatus status);

  // Records the time taken to successfully stop a worker.
  static void RecordStopWorkerTime(base::TimeDelta time);

  static void RecordActivateEventStatus(ServiceWorkerStatusCode status,
                                        bool is_shutdown);
  static void RecordInstallEventStatus(ServiceWorkerStatusCode status);

  // Records how often a dispatched event times out.
  static void RecordEventTimeout(EventType event);

  // Records the amount of time spent handling an event.
  static void RecordEventDuration(EventType event,
                                  base::TimeDelta time,
                                  bool was_handled);

  // Records the time taken between sending an event IPC from the browser
  // process to a Service Worker and executing the event handler in the Service
  // Worker.
  static void RecordEventDispatchingDelay(EventType event,
                                          base::TimeDelta time,
                                          Site site_for_metrics);

  // Records the result of dispatching a fetch event to a service worker.
  static void RecordFetchEventStatus(bool is_main_resource,
                                     ServiceWorkerStatusCode status);

  // Records result of a ServiceWorkerURLRequestJob that was forwarded to
  // the service worker.
  static void RecordURLRequestJobResult(bool is_main_resource,
                                        URLRequestJobResult result);

  // Records the error code provided when the renderer returns a response with
  // status zero to a fetch request.
  static void RecordStatusZeroResponseError(
      bool is_main_resource,
      blink::mojom::ServiceWorkerResponseError error);

  // Records the mode of request that was fallbacked to the network.
  static void RecordFallbackedRequestMode(
      network::mojom::FetchRequestMode mode);

  // Called at the beginning of each ServiceWorkerVersion::Dispatch*Event
  // function. Records the time elapsed since idle (generally the time since the
  // previous event ended).
  static void RecordTimeBetweenEvents(base::TimeDelta time);

  // The following record steps of EmbeddedWorkerInstance's start sequence.
  static void RecordProcessCreated(bool is_new_process);
  static void RecordTimeToSendStartWorker(base::TimeDelta duration,
                                          StartSituation start_situation);
  static void RecordTimeToURLJob(base::TimeDelta duration,
                                 StartSituation start_situation);
  static void RecordTimeToLoad(base::TimeDelta duration,
                               LoadSource source,
                               StartSituation start_situation);
  static void RecordTimeToStartThread(base::TimeDelta duration,
                                      StartSituation start_situation);
  static void RecordTimeToEvaluateScript(base::TimeDelta duration,
                                         StartSituation start_situation);
  static void RecordStartMessageLatencyType(CrossProcessTimeDelta type);
  static void RecordWaitedForRendererSetup(bool waited);
  CONTENT_EXPORT static void RecordEmbeddedWorkerStartTiming(
      mojom::EmbeddedWorkerStartTimingPtr start_timing,
      base::TimeTicks start_worker_sent_time,
      StartSituation start_situation);

  static const char* LoadSourceToString(LoadSource source);

  // Records the result of a start attempt that occurred after the worker had
  // failed |failure_count| consecutive times.
  static void RecordStartStatusAfterFailure(int failure_count,
                                            ServiceWorkerStatusCode status);

  // Records the size of Service-Worker-Navigation-Preload header when the
  // navigation preload request is to be sent.
  static void RecordNavigationPreloadRequestHeaderSize(size_t size);

  // Records timings for the navigation preload response and how
  // it compares to starting the worker.
  // |worker_start| is the time it took to prepare an activated and running
  // worker to receive the fetch event. |initial_worker_status| and
  // |start_situation| describe the preparation needed.
  // |response_start| is the time it took until the navigation preload response
  // started.
  // |resource_type| must be RESOURCE_TYPE_MAIN_FRAME or
  // RESOURCE_TYPE_SUB_FRAME.
  CONTENT_EXPORT static void RecordNavigationPreloadResponse(
      base::TimeDelta worker_start,
      base::TimeDelta response_start,
      EmbeddedWorkerStatus initial_worker_status,
      StartSituation start_situation,
      ResourceType resource_type);

  // Records the result of trying to handle a request for a service worker
  // script.
  static void RecordContextRequestHandlerStatus(
      ServiceWorkerContextRequestHandler::CreateJobStatus status,
      bool is_installed,
      bool is_main_script);

  static void RecordRuntime(base::TimeDelta time);

  // Records when an installed service worker imports a script that was not
  // previously installed.
  // TODO(falken): Remove after this is deprecated. https://crbug.com/737044
  static void RecordUninstalledScriptImport(const GURL& url);

  // Records the result of starting service worker for a navigation hint.
  static void RecordStartServiceWorkerForNavigationHintResult(
      StartServiceWorkerForNavigationHintResult result);

  // Records the number of origins with a registered service worker.
  static void RecordRegisteredOriginCount(size_t origin_count);

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(ServiceWorkerMetrics);
};

}  // namespace content

#endif  // CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_METRICS_H_
