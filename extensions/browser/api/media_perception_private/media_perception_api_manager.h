// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_API_MEDIA_PERCEPTION_PRIVATE_MEDIA_PERCEPTION_API_MANAGER_H_
#define EXTENSIONS_BROWSER_API_MEDIA_PERCEPTION_PRIVATE_MEDIA_PERCEPTION_API_MANAGER_H_

#include <string>

#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "base/scoped_observer.h"
#include "chromeos/dbus/media_analytics_client.h"
#include "chromeos/media_perception/media_perception.pb.h"
#include "extensions/browser/browser_context_keyed_api_factory.h"
#include "extensions/common/api/media_perception_private.h"

namespace extensions {

class MediaPerceptionAPIManager
    : public BrowserContextKeyedAPI,
      public chromeos::MediaAnalyticsClient::Observer {
 public:
  using APISetAnalyticsComponentCallback = base::OnceCallback<void(
      extensions::api::media_perception_private::ComponentState
          component_state)>;

  using APIStateCallback = base::Callback<void(
      extensions::api::media_perception_private::State state)>;

  using APIGetDiagnosticsCallback = base::Callback<void(
      extensions::api::media_perception_private::Diagnostics diagnostics)>;

  explicit MediaPerceptionAPIManager(content::BrowserContext* context);
  ~MediaPerceptionAPIManager() override;

  // Convenience method to get the MediaPeceptionAPIManager for a
  // BrowserContext.
  static MediaPerceptionAPIManager* Get(content::BrowserContext* context);

  // BrowserContextKeyedAPI implementation.
  static BrowserContextKeyedAPIFactory<MediaPerceptionAPIManager>*
  GetFactoryInstance();

  // Public functions for MediaPerceptionPrivateAPI implementation.
  void SetAnalyticsComponent(
      const extensions::api::media_perception_private::Component& component,
      APISetAnalyticsComponentCallback callback);
  void GetState(const APIStateCallback& callback);
  void SetState(const extensions::api::media_perception_private::State& state,
                const APIStateCallback& callback);
  void GetDiagnostics(const APIGetDiagnosticsCallback& callback);

 private:
  friend class BrowserContextKeyedAPIFactory<MediaPerceptionAPIManager>;

  // BrowserContextKeyedAPI:
  static const char* service_name() { return "MediaPerceptionAPIManager"; }

  enum class AnalyticsProcessState {
    // The process is not running.
    IDLE,
    // The process has been launched via Upstart, but waiting for callback to
    // confirm.
    CHANGING_PROCESS_STATE,
    // The process is running.
    RUNNING,
    // The process state is unknown, e.g. when a Upstart Stop request fails.
    UNKNOWN
  };

  // Sets the state of the analytics process.
  void SetStateInternal(const APIStateCallback& callback,
                        const mri::State& state);

  // MediaAnalyticsClient::Observer overrides.
  void OnDetectionSignal(const mri::MediaPerception& media_perception) override;

  // Callback for State D-Bus method calls to the media analytics process.
  void StateCallback(const APIStateCallback& callback,
                     base::Optional<mri::State> state);

  // Callback for GetDiagnostics D-Bus method calls to the media analytics
  // process.
  void GetDiagnosticsCallback(const APIGetDiagnosticsCallback& callback,
                              base::Optional<mri::Diagnostics> diagnostics);

  // Callback for Upstart command to start media analytics process.
  void UpstartStartCallback(const APIStateCallback& callback,
                            const mri::State& state,
                            bool succeeded);

  // Callback for Upstart command to restart media analytics process.
  void UpstartRestartCallback(const APIStateCallback& callback, bool succeeded);

  // Callback for Upstart command to stop media analytics process.
  void UpstartStopCallback(const APIStateCallback& callback, bool succeeded);

  // Callback with the mount point for a loaded component.
  void LoadComponentCallback(APISetAnalyticsComponentCallback callback,
                             bool success,
                             const base::FilePath& mount_point);

  bool ComponentIsLoaded();

  content::BrowserContext* const browser_context_;

  // Keeps track of whether the analytics process is running so that it can be
  // started with an Upstart D-Bus method call if necessary.
  AnalyticsProcessState analytics_process_state_;

  // Keeps track of the mount point for the current media analytics process
  // component from component updater. If this string is not set, no component
  // is set.
  std::string mount_point_;

  ScopedObserver<chromeos::MediaAnalyticsClient, MediaPerceptionAPIManager>
      scoped_observer_;
  base::WeakPtrFactory<MediaPerceptionAPIManager> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(MediaPerceptionAPIManager);
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_API_MEDIA_PERCEPTION_PRIVATE_MEDIA_PERCEPTION_API_MANAGER_H_
