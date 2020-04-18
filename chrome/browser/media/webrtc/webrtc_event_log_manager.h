// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_WEBRTC_WEBRTC_EVENT_LOG_MANAGER_H_
#define CHROME_BROWSER_MEDIA_WEBRTC_WEBRTC_EVENT_LOG_MANAGER_H_

#include <map>
#include <memory>
#include <type_traits>
#include <utility>

#include "base/callback.h"
#include "base/containers/flat_set.h"
#include "base/files/file_path.h"
#include "base/memory/scoped_refptr.h"
#include "base/sequenced_task_runner.h"
#include "base/time/clock.h"
#include "base/time/time.h"
#include "chrome/browser/media/webrtc/webrtc_event_log_manager_common.h"
#include "chrome/browser/media/webrtc/webrtc_event_log_manager_local.h"
#include "chrome/browser/media/webrtc/webrtc_event_log_manager_remote.h"
#include "content/public/browser/render_process_host_observer.h"
#include "content/public/browser/webrtc_event_logger.h"

namespace content {
class BrowserContext;
};

// This is a singleton class running in the browser UI thread (ownership of
// the only instance lies in BrowserContext). It is in charge of writing WebRTC
// event logs to temporary files, then uploading those files to remote servers,
// as well as of writing the logs to files which were manually indicated by the
// user from the WebRTCIntenals. (A log may simulatenously be written to both,
// either, or none.)
// The only instance of this class is owned by BrowserProcessImpl. It is
// destroyed from ~BrowserProcessImpl(), at which point any tasks posted to the
// internal SequencedTaskRunner, or coming from another thread, would no longer
// execute.
class WebRtcEventLogManager final : public content::RenderProcessHostObserver,
                                    public content::WebRtcEventLogger,
                                    public WebRtcLocalEventLogsObserver,
                                    public WebRtcRemoteEventLogsObserver {
 public:
  using BrowserContextId = WebRtcEventLogPeerConnectionKey::BrowserContextId;

  // To turn WebRTC on and off, we go through PeerConnectionTrackerProxy. In
  // order to make this toggling easily testable, PeerConnectionTrackerProxyImpl
  // will send real messages to PeerConnectionTracker, whereas
  // PeerConnectionTrackerProxyForTesting will be a mock that just makes sure
  // the correct messages were attempted to be sent.
  class PeerConnectionTrackerProxy {
   public:
    virtual ~PeerConnectionTrackerProxy() = default;
    virtual void SetWebRtcEventLoggingState(
        const WebRtcEventLogPeerConnectionKey& key,
        bool event_logging_enabled) = 0;
  };

  // Translate a BrowserContext into an ID, allowing associating PeerConnections
  // with it while making sure that its methods would never be called outside
  // of the UI thread.
  static BrowserContextId GetBrowserContextId(
      const content::BrowserContext* browser_context);

  // Fetches the BrowserContext associated with the render process ID, then
  // returns its BrowserContextId. (If the render process has already died,
  // it would have no BrowserContext associated, so kNullBrowserContextId will
  // be returned.)
  static BrowserContextId GetBrowserContextId(int render_process_id);

  // Ensures that no previous instantiation of the class was performed, then
  // instantiates the class and returns the object (ownership is transfered to
  // the caller). Subsequent calls to GetInstance() will return this object,
  // until it is destructed, at which pointer nullptr will be returned by
  // subsequent calls.
  static std::unique_ptr<WebRtcEventLogManager> CreateSingletonInstance();

  // Returns the object previously constructed using CreateSingletonInstance(),
  // if it was constructed and was not yet destroyed; nullptr otherwise.
  static WebRtcEventLogManager* GetInstance();

  ~WebRtcEventLogManager() override;

  void EnableForBrowserContext(
      const content::BrowserContext* browser_context,
      base::OnceClosure reply = base::OnceClosure()) override;

  void DisableForBrowserContext(
      const content::BrowserContext* browser_context,
      base::OnceClosure reply = base::OnceClosure()) override;

  void PeerConnectionAdded(int render_process_id,
                           int lid,  // Renderer-local PeerConnection ID.
                           const std::string& peer_connection_id,
                           base::OnceCallback<void(bool)> reply =
                               base::OnceCallback<void(bool)>()) override;

  void PeerConnectionRemoved(int render_process_id,
                             int lid,  // Renderer-local PeerConnection ID.
                             base::OnceCallback<void(bool)> reply =
                                 base::OnceCallback<void(bool)>()) override;

  // From the logger's perspective, we treat stopping a peer connection the
  // same as we do its removal. Should a stopped peer connection be later
  // removed, the removal callback will assume the value |false|.
  void PeerConnectionStopped(int render_process_id,
                             int lid,  // Renderer-local PeerConnection ID.
                             base::OnceCallback<void(bool)> reply =
                                 base::OnceCallback<void(bool)>()) override;

  // The file's actual path is derived from |base_path| by adding a timestamp,
  // the render process ID and the PeerConnection's local ID.
  void EnableLocalLogging(const base::FilePath& base_path,
                          base::OnceCallback<void(bool)> reply =
                              base::OnceCallback<void(bool)>()) override;
  void EnableLocalLogging(
      const base::FilePath& base_path,
      size_t max_file_size_bytes = kDefaultMaxLocalLogFileSizeBytes,
      base::OnceCallback<void(bool)> reply = base::OnceCallback<void(bool)>());

  void DisableLocalLogging(base::OnceCallback<void(bool)> reply =
                               base::OnceCallback<void(bool)>()) override;

  void OnWebRtcEventLogWrite(
      int render_process_id,
      int lid,  // Renderer-local PeerConnection ID.
      const std::string& message,
      base::OnceCallback<void(std::pair<bool, bool>)> reply =
          base::OnceCallback<void(std::pair<bool, bool>)>()) override;

  // Start logging a peer connection's WebRTC events to a file, which will
  // later be uploaded to a remote server. If a reply is provided, it will be
  // posted back to BrowserThread::UI with the return value provided by
  // WebRtcRemoteEventLogManager::StartRemoteLogging - see the comment there
  // for more details.
  // One may attach an arbitrary binary string |metadata|, which would be
  // prepended to the WebRTC event log. Its length is counted towards the
  // max file size. (I.e., if the metadata is of length 4 and the max size
  // is 10, only 6 bytes are left available for the actual WebRTC event log.)
  void StartRemoteLogging(
      int render_process_id,
      const std::string& peer_connection_id,
      size_t max_file_size_bytes,
      const std::string& metadata,
      base::OnceCallback<void(bool, const std::string&)> reply =
          base::OnceCallback<void(bool, const std::string&)>());

  // Clear WebRTC event logs associated with a given browser context, in a given
  // time range (|delete_begin| inclusive, |delete_end| exclusive), then
  // post |reply| back to the thread from which the method was originally
  // invoked (which can be any thread).
  void ClearCacheForBrowserContext(
      const content::BrowserContext* browser_context,
      const base::Time& delete_begin,
      const base::Time& delete_end,
      base::OnceClosure reply);

  // Set (or unset) an observer that will be informed whenever a local log file
  // is started/stopped. The observer needs to be able to either run from
  // anywhere. If you need the code to run on specific runners or queues, have
  // the observer post them there.
  // If a reply callback is given, it will be posted back to BrowserThread::UI
  // after the observer has been set.
  void SetLocalLogsObserver(WebRtcLocalEventLogsObserver* observer,
                            base::OnceClosure reply = base::OnceClosure());

  // Set (or unset) an observer that will be informed whenever a remote log file
  // is started/stopped. Note that this refers to writing these files to disk,
  // not for uploading them to the server.
  // The observer needs to be able to either run from anywhere. If you need the
  // code to run on specific runners or queues, have the observer post
  // them there.
  // If a reply callback is given, it will be posted back to BrowserThread::UI
  // after the observer has been set.
  void SetRemoteLogsObserver(WebRtcRemoteEventLogsObserver* observer,
                             base::OnceClosure reply = base::OnceClosure());

 private:
  friend class SigninManagerAndroidTest;       // Calls *ForTesting() methods.
  friend class WebRtcEventLogManagerTestBase;  // Calls *ForTesting() methods.

  using PeerConnectionKey = WebRtcEventLogPeerConnectionKey;

  // This bitmap allows us to track for which clients (local/remote logging)
  // we have turned WebRTC event logging on for a given peer connection, so that
  // we may turn it off only when the last client no longer needs it.
  enum LoggingTarget : unsigned int {
    kLocalLogging = 1 << 0,
    kRemoteLogging = 1 << 1
  };
  using LoggingTargetBitmap = std::underlying_type<LoggingTarget>::type;

  WebRtcEventLogManager();

  // Checks whether remote-bound logging is enabled.
  bool IsRemoteLoggingEnabled() const;

  // RenderProcessHostObserver implementation.
  void RenderProcessExited(
      content::RenderProcessHost* host,
      const content::ChildProcessTerminationInfo& info) override;
  void RenderProcessHostDestroyed(content::RenderProcessHost* host) override;

  // RenderProcessExited() and RenderProcessHostDestroyed() treated similarly
  // by this function.
  void RenderProcessHostExitedDestroyed(content::RenderProcessHost* host);

  // WebRtcLocalEventLogsObserver implementation:
  void OnLocalLogStarted(PeerConnectionKey peer_connection,
                         const base::FilePath& file_path) override;
  void OnLocalLogStopped(PeerConnectionKey peer_connection) override;

  // WebRtcRemoteEventLogsObserver implementation:
  void OnRemoteLogStarted(PeerConnectionKey key,
                          const base::FilePath& file_path) override;
  void OnRemoteLogStopped(PeerConnectionKey key) override;

  void OnLoggingTargetStarted(LoggingTarget target, PeerConnectionKey key);
  void OnLoggingTargetStopped(LoggingTarget target, PeerConnectionKey key);

  void EnableForBrowserContextInternal(
      BrowserContextId browser_context_id,
      const base::FilePath& browser_context_dir,
      base::OnceClosure reply);
  void DisableForBrowserContextInternal(BrowserContextId browser_context_id,
                                        base::OnceClosure reply);

  void PeerConnectionAddedInternal(PeerConnectionKey key,
                                   const std::string& peer_connection_id,
                                   base::OnceCallback<void(bool)> reply);
  void PeerConnectionRemovedInternal(PeerConnectionKey key,
                                     base::OnceCallback<void(bool)> reply);

  void EnableLocalLoggingInternal(const base::FilePath& base_path,
                                  size_t max_file_size_bytes,
                                  base::OnceCallback<void(bool)> reply);
  void DisableLocalLoggingInternal(base::OnceCallback<void(bool)> reply);

  void OnWebRtcEventLogWriteInternal(
      PeerConnectionKey key,
      bool remote_logging_allowed,
      const std::string& message,
      base::OnceCallback<void(std::pair<bool, bool>)> reply);

  void StartRemoteLoggingInternal(
      int render_process_id,
      BrowserContextId browser_context_id,
      const std::string& peer_connection_id,
      const base::FilePath& browser_context_dir,
      size_t max_file_size_bytes,
      const std::string& metadata,
      base::OnceCallback<void(bool, const std::string&)> reply);

  void ClearCacheForBrowserContextInternal(BrowserContextId browser_context_id,
                                           const base::Time& delete_begin,
                                           const base::Time& delete_end);

  void RenderProcessExitedInternal(int render_process_id);

  void SetLocalLogsObserverInternal(WebRtcLocalEventLogsObserver* observer,
                                    base::OnceClosure reply);

  void SetRemoteLogsObserverInternal(WebRtcRemoteEventLogsObserver* observer,
                                     base::OnceClosure reply);

  // Non-empty replies get posted to BrowserThread::UI.
  void MaybeReply(base::OnceClosure reply);
  void MaybeReply(base::OnceCallback<void(bool)> reply, bool value);
  void MaybeReply(base::OnceCallback<void(bool, const std::string&)> reply,
                  bool bool_val,
                  const std::string& str_val);
  void MaybeReply(base::OnceCallback<void(std::pair<bool, bool>)> reply,
                  bool first,
                  bool second);

  // Injects a fake clock, to be used by tests. For example, this could be
  // used to inject a frozen clock, thereby allowing unit tests to know what a
  // local log's filename would end up being.
  void SetClockForTesting(base::Clock* clock,
                          base::OnceClosure reply = base::OnceClosure());

  // Injects a PeerConnectionTrackerProxy for testing. The normal tracker proxy
  // is used to communicate back to WebRTC whether event logging is desired for
  // a given peer connection. Using this function, those indications can be
  // intercepted by a unit test.
  void SetPeerConnectionTrackerProxyForTesting(
      std::unique_ptr<PeerConnectionTrackerProxy> pc_tracker_proxy,
      base::OnceClosure reply = base::OnceClosure());

  // Injects a fake uploader, to be used by unit tests.
  void SetWebRtcEventLogUploaderFactoryForTesting(
      std::unique_ptr<WebRtcEventLogUploader::Factory> uploader_factory,
      base::OnceClosure reply = base::OnceClosure());

  // This allows unit tests that do not wish to change the task runner to still
  // check when certain operations are finished.
  // TODO(crbug.com/775415): Remove this and use PostNullTaskForTesting instead.
  scoped_refptr<base::SequencedTaskRunner>& GetTaskRunnerForTesting();

  void PostNullTaskForTesting(base::OnceClosure reply) override;

  static WebRtcEventLogManager* g_webrtc_event_log_manager;

  // Observer which will be informed whenever a local log file is started or
  // stopped. Its callbacks are called synchronously from |task_runner_|,
  // so the observer needs to be able to either run from any (sequenced) runner.
  WebRtcLocalEventLogsObserver* local_logs_observer_;

  // Observer which will be informed whenever a remote log file is started or
  // stopped. Its callbacks are called synchronously from |task_runner_|,
  // so the observer needs to be able to either run from any (sequenced) runner.
  WebRtcRemoteEventLogsObserver* remote_logs_observer_;

  // Manages local-bound logs - logs stored on the local filesystem when
  // logging has been explicitly enabled by the user.
  WebRtcLocalEventLogManager local_logs_manager_;

  // Manages remote-bound logs - logs which will be sent to a remote server.
  // This is only possible when a command line flag is present.
  // TODO(eladalon): Remove the command-line flag and the unique_ptr.
  // https://crbug.com/775415
  std::unique_ptr<WebRtcRemoteEventLogManager> remote_logs_manager_;

  // This keeps track of which peer connections have event logging turned on
  // in WebRTC, and for which client(s).
  std::map<PeerConnectionKey, LoggingTargetBitmap>
      peer_connections_with_event_logging_enabled_;

  // The set of RenderProcessHosts with which the manager is registered for
  // observation. Allows us to register for each RPH only once, and get notified
  // when it exits (cleanly or due to a crash).
  // This object is only to be accessed on the UI thread.
  base::flat_set<content::RenderProcessHost*> observed_render_process_hosts_;

  // In production, this holds a small object that just tells WebRTC (via
  // PeerConnectionTracker) to start/stop producing event logs for a specific
  // peer connection. In (relevant) unit tests, a mock will be injected.
  std::unique_ptr<PeerConnectionTrackerProxy> pc_tracker_proxy_;

  // The main logic will run sequentially on this runner, on which blocking
  // tasks are allowed.
  scoped_refptr<base::SequencedTaskRunner> task_runner_;

  DISALLOW_COPY_AND_ASSIGN(WebRtcEventLogManager);
};

#endif  // CHROME_BROWSER_MEDIA_WEBRTC_WEBRTC_EVENT_LOG_MANAGER_H_
